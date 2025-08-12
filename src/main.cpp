#include "Arduino.h"
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <Update.h>
#include <ESPmDNS.h>

SoftwareSerial audioSerial(D7, D6); // RX=D7(GPIO20), TX=D6(GPIO21)
DFRobotDFPlayerMini dfPlayer;

// Firmware version
#define FIRMWARE_VERSION "0.7.0"
#define BUILD_DATE __DATE__ " " __TIME__

// Web server and WiFi
WebServer server(80);
WiFiManager wifiManager;

// LED pins for candle effect
#define LED1 D0 // Candle fiber optics 1
#define LED2 D1 // Candle fiber optics 2
#define LED3 D2 // Brazier
#define LED4 D3 // Console screen
#define LED5 D4 // Weapon 1 - machine gun
#define LED6 D5 // Weapon 2 - flamethrower
#define LED7 D8 // Engine stack 1
#define LED8 D9 // Engine stack 2
#define LED9 D10 // Unused currently

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
void candleFlicker(int ledPin);
void printDetail(uint8_t type, int value);
void setupWiFi();
void setupWebServer();
void handleRoot();
void handleUpload();
void handleUploadFile();

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
    Serial.println("DFPlayer connected!");
    
    // Set volume and start looping
    dfPlayer.volume(20); // Volume 0-30
    delay(500);
    dfPlayer.loop(AUDIO_IDLE);
    
    Serial.println("Playing IDLE file in loop with candle flicker...");
  } else {
    Serial.println("DFPlayer connection failed!");
  }
  
  // Setup WiFi and web server
  setupWiFi();
  setupWebServer();
}

void loop() {
  // Candle flicker effect for both LEDs
  candleFlicker(LED1);
  candleFlicker(LED2);
  candleFlicker(LED3);
  
  // Small delay for smooth flicker
  delay(50);
  if (dfPlayer.available()) {
    printDetail(dfPlayer.readType(), dfPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
  
  // Handle web server requests
  server.handleClient();

}

// Candle flicker effect
void candleFlicker(int ledPin) {
  static unsigned long lastBaseUpdate = 0;
  static int baseIntensity = 60;  // Starting base
  
  // Update base intensity every 200-500ms (randomly)
  if (millis() - lastBaseUpdate > random(200, 500)) {
    baseIntensity = random(40, 80);
    lastBaseUpdate = millis();
  }
  
  // Add flicker every call
  int flicker = random(-20, 30);
  int brightness = constrain(baseIntensity + flicker, 5, 120);
  
  analogWrite(ledPin, brightness);
}

// Engine breathing effect
void enginePulse(int ledPin, int minBright, int maxBright, int speed) {
  static unsigned long lastUpdate = 0;
  static int brightness = minBright;
  static int direction = 1;

  if (millis() - lastUpdate > speed) {
    lastUpdate = millis();
    brightness += direction;
    if (brightness >= maxBright || brightness <= minBright) {
      direction = -direction;
    }
    analogWrite(ledPin, brightness);
  }
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println(F("USB Inserted!"));
      break;
    case DFPlayerUSBRemoved:
      Serial.println(F("USB Removed!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
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
  html += F("<p>Current Status: Running</p>");
  html += F("<h2>Upload New Firmware</h2>");
  html += F("<form method='POST' action='/upload' enctype='multipart/form-data'>");
  html += F("<input type='file' name='firmware' accept='.bin'><br><br>");
  html += F("<input type='submit' value='Upload'>");
  html += F("</form></body></html>");
  
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
