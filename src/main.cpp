#include "Arduino.h"
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <Update.h>
#include <ESPmDNS.h>
#include "effects.h"
#include "dfplayer.h"

SoftwareSerial audioSerial(D7, D6); // RX=D7(GPIO20), TX=D6(GPIO21)
DFRobotDFPlayerMini dfPlayer;

// Firmware version
#define FIRMWARE_VERSION "0.14.0"
#define VERSION_FEATURE "Add console data stream effect for LED4 screen simulation"
#define BUILD_DATE __DATE__ " " __TIME__

// Web server and WiFi
WebServer server(80);
WiFiManager wifiManager;



// Audio files
#define AUDIO_IDLE        2
#define AUDIO_WEAPON_FIRE_1    3
#define AUDIO_WEAPON_FIRE_2    4
#define AUDIO_TAKING_HITS      5
#define AUDIO_ENGINE_REV       6
#define AUDIO_DESTROYED        7
#define AUDIO_LIMITED_WEAPON   8
#define AUDIO_UNIT_KILL        9

// Function declarations
void printDetail(uint8_t type, int value);
void checkDFPlayerStatus();
void setupWiFi();
void setupWebServer();
void handleRoot();
void handleUpload();
void handleUploadFile();
void handleWiFiReset();
void handleFactoryReset();

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("=== BattleAura ===");
  
  // Initialize LEDs
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED5, OUTPUT);
  pinMode(LED6, OUTPUT); 
  pinMode(LED7, OUTPUT);
  pinMode(LED8, OUTPUT);
  
  // Initialize DFPlayer
  audioSerial.begin(9600);
  
  if (dfPlayer.begin(audioSerial, false, false)) {
    Serial.println("DFPlayer serial initialized, testing communication...");
    
    // Test actual communication by trying to read current state
    delay(1000);  // Give DFPlayer time to initialize
    
    // Try to set and verify volume to test bidirectional communication
    dfPlayer.volume(20);
    delay(500);
    
    int currentVolume = dfPlayer.readVolume();
    delay(100);
    
    if (currentVolume > 0 && currentVolume <= 30) {
      Serial.print("DFPlayer connected! Volume: ");
      Serial.println(currentVolume);
      dfPlayerConnected = true;
      dfPlayerStatus = "Connected";
      
      // Start looping idle sound
      dfPlayer.loop(AUDIO_IDLE);
      currentTrack = AUDIO_IDLE;
      dfPlayerPlaying = true;
      
      Serial.println("Playing IDLE file in loop with candle flicker...");
    } else {
      Serial.println("DFPlayer communication test failed - no response");
      dfPlayerConnected = false;
      dfPlayerStatus = "No Response";
    }
  } else {
    Serial.println("DFPlayer serial initialization failed!");
    dfPlayerConnected = false;
    dfPlayerStatus = "Serial Init Failed";
  }
  
  // Setup WiFi and web server
  setupWiFi();
  setupWebServer();
}

void loop() {
  // Candle flicker effect for both LEDs
  candleFlicker(LED1); //candle fiber optics 1
  candleFlicker(LED2); //candle fiber optics 2
  candleFlicker(LED3); // brazier
  
  // Console screen data stream effect
  consoleDataStream(LED4); // console screen
  
  // Engine stack effects
  enginePulseSmooth(LED7, 0);    // First engine stack - smooth pulse
  enginePulseSmooth(LED8, 0);    // Second engine stack - smooth pulse
  
  // Small delay for smooth flicker
  delay(50);
  if (dfPlayer.available()) {
    printDetail(dfPlayer.readType(), dfPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
  
  // Periodic DFPlayer status check (every 10 seconds)
  checkDFPlayerStatus();
  
  // Handle web server requests
  server.handleClient();

}

// Check DFPlayer status periodically
void checkDFPlayerStatus() {
  // Check every 10 seconds
  if (millis() - lastStatusCheck < 10000) {
    return;
  }
  lastStatusCheck = millis();
  
  // Only check if we think it's connected, or if it was disconnected and we want to retry
  if (dfPlayerConnected || dfPlayerStatus == "No Response" || dfPlayerStatus == "Serial Init Failed") {
    int volume = dfPlayer.readVolume();
    delay(100);
    
    if (volume > 0 && volume <= 30) {
      if (!dfPlayerConnected) {
        Serial.println("DFPlayer reconnected!");
        dfPlayerConnected = true;
        dfPlayerStatus = "Connected";
        // Restart idle loop if it was disconnected
        if (!dfPlayerPlaying) {
          dfPlayer.loop(AUDIO_IDLE);
          currentTrack = AUDIO_IDLE;
          dfPlayerPlaying = true;
        }
      }
    } else {
      if (dfPlayerConnected) {
        Serial.println("DFPlayer connection lost!");
        dfPlayerConnected = false;
        dfPlayerStatus = "Connection Lost";
        dfPlayerPlaying = false;
      }
    }
  }
}

// WiFi setup with captive portal
void setupWiFi() {
  Serial.println(F("Setting up WiFi..."));
  
  // Set custom AP name and password
  wifiManager.setAPCallback([](WiFiManager *myWiFiManager) {
    Serial.println(F("Entered config mode"));
    Serial.print(F("AP: "));
    Serial.println(myWiFiManager->getConfigPortalSSID());
  });
  
  // Custom hostname parameter
  WiFiManagerParameter custom_hostname("hostname", "Device Hostname", "battletank", 20);
  wifiManager.addParameter(&custom_hostname);
  
  // Try to connect, if failed start captive portal
  if (!wifiManager.autoConnect("BattleAura-Setup")) {
    Serial.println(F("Failed to connect and hit timeout"));
    ESP.restart();
  }
  
  // Connected to WiFi
  Serial.println(F("WiFi connected!"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  
  // Start mDNS with custom hostname
  String hostname = custom_hostname.getValue();
  if (hostname.length() == 0) hostname = "battletank";
  
  if (MDNS.begin(hostname.c_str())) {
    Serial.print(F("mDNS started: "));
    Serial.print(hostname);
    Serial.println(F(".local"));
  }
}

// Web server setup
void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/upload", HTTP_POST, handleUpload, handleUploadFile);
  server.on("/wifi-reset", handleWiFiReset);
  server.on("/factory-reset", handleFactoryReset);
  
  server.begin();
  Serial.println(F("Web server started"));
}

// Main page with upload form
void handleRoot() {
  String html = F("<!DOCTYPE html><html><head><title>BattleAura OTA</title></head><body>");
  html += F("<h1>BattleAura - OTA Update</h1>");
  html += F("<p><strong>Firmware:</strong> ");
  html += FIRMWARE_VERSION;
  html += F("<br><strong>Built:</strong> ");
  html += BUILD_DATE;
  html += F("</p>");
  html += F("<p>");
  html += VERSION_FEATURE;
  html += F("</p>");
  
  html += F("<h2>System Status</h2>");
  html += F("<p><strong>System:</strong> Running<br>");
  html += F("<strong>DFPlayer:</strong> ");
  if (dfPlayerConnected) {
    html += F("Connected<br>");
    html += F("<strong>Audio Status:</strong> ");
    html += dfPlayerStatus;
    if (dfPlayerPlaying) {
      html += F(" (Track ");
      html += currentTrack;
      html += F(")");
    }
  } else {
    html += F("Disconnected");
  }
  html += F("</p>");
  
  html += F("<h2>Upload New Firmware</h2>");
  html += F("<form method='POST' action='/upload' enctype='multipart/form-data'>");
  html += F("<input type='file' name='firmware' accept='.bin'><br><br>");
  html += F("<input type='submit' value='Upload'>");
  html += F("</form>");
  
  html += F("<h2>System Controls</h2>");
  html += F("<p><strong>WiFi Reset:</strong> Clear WiFi settings and restart captive portal<br>");
  html += F("<button onclick=\"if(confirm('Reset WiFi settings? Device will restart.')) window.location='/wifi-reset'\" style='background:#ff6b35;color:white;padding:10px;border:none;border-radius:5px;margin:5px;'>Reset WiFi</button></p>");
  
  html += F("<p><strong>Factory Reset:</strong> Reset all settings to factory defaults<br>");
  html += F("<button onclick=\"if(confirm('Factory reset? This will erase all settings and restart the device.')) window.location='/factory-reset'\" style='background:#dc3545;color:white;padding:10px;border:none;border-radius:5px;margin:5px;'>Factory Reset</button></p>");
  
  html += F("</body></html>");
  
  server.send(200, "text/html", html);
}

// Handle upload completion
void handleUpload() {
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  ESP.restart();
}

// Handle actual file upload
void handleUploadFile() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update Start: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Update Success: %uB\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

// Handle WiFi reset request
void handleWiFiReset() {
  Serial.println("WiFi reset requested via web interface");
  server.send(200, "text/html", 
    F("<html><body><h1>WiFi Reset</h1><p>WiFi settings cleared. Device restarting...</p></body></html>"));
  
  delay(1000);
  
  // Clear WiFi settings
  wifiManager.resetSettings();
  
  Serial.println("WiFi settings reset, restarting...");
  ESP.restart();
}

// Handle factory reset request  
void handleFactoryReset() {
  Serial.println("Factory reset requested via web interface");
  server.send(200, "text/html",
    F("<html><body><h1>Factory Reset</h1><p>All settings cleared. Device restarting...</p></body></html>"));
  
  delay(1000);
  
  // Clear WiFi settings
  wifiManager.resetSettings();
  
  // Could add other factory reset actions here:
  // - Clear EEPROM/NVS settings if you store any
  // - Reset any saved configurations
  // - Reset DFPlayer settings
  
  Serial.println("Factory reset complete, restarting...");
  ESP.restart();
}
