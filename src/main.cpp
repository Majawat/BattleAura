#include "Arduino.h"
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <Update.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "effects.h"
#include "dfplayer.h"
#include "web_interface.h"

// RTOS Task handles
TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t webTaskHandle = NULL;
TaskHandle_t ledTaskHandle = NULL;

SoftwareSerial audioSerial(D7, D6); // RX=D7(GPIO20), TX=D6(GPIO21)
DFRobotDFPlayerMini dfPlayer;

// Firmware version constants now in config.h

// WiFi manager
WiFiManager wifiManager;

// Audio file constants now in config.h

// Function declarations
void printDetail(uint8_t type, int value);
void checkDFPlayerStatus();
void setupWiFi();
void setupWebServer();
void resumeIdleAudio();
int compareVersions(String current, String remote);
bool checkForUpdates(String &newVersion, String &downloadUrl, String &changelog);

// RTOS Task Functions
void audioTask(void* parameter);
void webTask(void* parameter);
void ledTask(void* parameter);
void printMemoryUsage();

// Audio and Effects Task - handles all audio and LED effects
void audioTask(void* parameter) {
  Serial.println("Audio task started on core " + String(xPortGetCoreID()));
  
  for(;;) {
    // DFPlayer message processing
    if (dfPlayer.available()) {
      printDetail(dfPlayer.readType(), dfPlayer.read());
    }
    
    // Periodic DFPlayer status check (every 10 seconds)
    checkDFPlayerStatus();
    
    // Check if idle audio should timeout to save battery
    checkIdleTimeout();
    
    // Handle actual idle audio stopping
    if (!idleAudioActive && dfPlayerConnected && dfPlayerPlaying && currentTrack == AUDIO_IDLE) {
      dfPlayer.stop();
      dfPlayerPlaying = false;
      dfPlayerStatus = "Idle Timeout - Stopped";
      Serial.println("Idle audio stopped - device now in low-power mode");
    }
    
    // Update non-blocking weapon effects
    updateWeaponEffects(&dfPlayer);
    
    // Update non-blocking battle effects
    updateBattleEffect(&dfPlayer);
    
    // Run at 20Hz for smooth effects
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// Web Server Task - handles async operations
void webTask(void* parameter) {
  Serial.println("Web task started on core " + String(xPortGetCoreID()));
  
  for(;;) {
    // ESPAsyncWebServer handles requests automatically
    // Minimal task for web server maintenance
    
    // Run at low frequency
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// LED Background Effects Task - handles candle flicker, etc.
void ledTask(void* parameter) {
  Serial.println("LED task started on core " + String(xPortGetCoreID()));
  
  unsigned long lastMemoryPrint = 0;
  
  for(;;) {
    // Run all configured background effects dynamically
    runBackgroundEffects();
    
    // Print memory usage every 30 seconds
    if (millis() - lastMemoryPrint > 30000) {
      printMemoryUsage();
      lastMemoryPrint = millis();
    }
    
    // Run at 20Hz for smooth LED effects
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// Memory monitoring function
void printMemoryUsage() {
  Serial.printf("Free heap: %d bytes, Largest block: %d bytes, Min free: %d bytes\n", 
                ESP.getFreeHeap(), ESP.getMaxAllocHeap(), ESP.getMinFreeHeap());
}

void setup() {
  Serial.begin(9600);
  // Remove initial delay - not needed
  Serial.println("BattleAura Init");
  
  // Load device configuration
  loadDeviceConfig();
  
  // Apply device configuration
  globalBrightness = deviceConfig.defaultBrightness;
  ledsEnabled = deviceConfig.hasLEDs;
  
  // Initialize pins based on configuration
  for (int i = 0; i < 11; i++) {
    if (deviceConfig.pins[i].enabled) {
      if (deviceConfig.pins[i].type == "led") {
        pinMode(deviceConfig.pins[i].pin, OUTPUT);
        Serial.println("Initialized LED pin " + String(deviceConfig.pins[i].pin) + " (" + deviceConfig.pins[i].label + ")");
      } else if (deviceConfig.pins[i].type == "rgb") {
        // RGB pins are handled by the NeoPixel library, no pinMode needed
        Serial.println("Configured RGB pin " + String(deviceConfig.pins[i].pin) + " (" + deviceConfig.pins[i].label + ")");
      }
    }
  }
  
  // Initialize RGB LED strip for any RGB pins
  int rgbPin = getRGBPin();
  if (rgbPin != -1) {
    rgbStrip.begin();
    rgbStrip.show(); // Initialize all pixels to 'off'
    Serial.println("RGB LED strip initialized on pin " + String(rgbPin));
  }
  
  // Initialize DFPlayer
  audioSerial.begin(9600);
  
  if (dfPlayer.begin(audioSerial, false, false)) {
    Serial.println("DFPlayer serial initialized, testing communication...");
    
    // Test actual communication by trying to read current state
    delay(1000);  // CRITICAL: DFPlayer initialization - keep this one
    
    // Try to set and verify volume to test bidirectional communication  
    currentVolume = deviceConfig.defaultVolume;
    dfPlayer.volume(currentVolume);
    delay(500); // CRITICAL: DFPlayer volume setting - keep this one
    
    int readVolume = dfPlayer.readVolume();
    delay(100); // CRITICAL: DFPlayer communication - keep this one
    
    if (readVolume > 0 && readVolume <= 30) {
      Serial.print("DFPlayer connected! Volume: ");
      Serial.println(readVolume);
      currentVolume = readVolume;
      dfPlayerConnected = true;
      dfPlayerStatus = "Connected";
      
      // Start looping idle sound
      dfPlayer.loop(AUDIO_IDLE);
      currentTrack = AUDIO_IDLE;
      dfPlayerPlaying = true;
      idleAudioActive = true;
      triggerActivity();
      
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
  
  // Setup callback for resuming idle audio after weapon effects
  setResumeIdleCallback(resumeIdleAudio);
  
  // Setup WiFi and web server
  setupWiFi();
  setupWebServer();
  
  // Print initial memory usage
  printMemoryUsage();
  
  // Create RTOS tasks for better performance and responsiveness
  Serial.println("Creating RTOS tasks...");
  
  // Audio and effects task on core 0 (same as main loop by default)
  xTaskCreatePinnedToCore(
    audioTask,           // Task function
    "AudioTask",         // Task name
    8192,               // Stack size (8KB)
    NULL,               // Parameters
    2,                  // Priority (higher than default)
    &audioTaskHandle,   // Task handle
    0                   // Core 0
  );
  
  // Web server task on core 1 for maximum responsiveness
  xTaskCreatePinnedToCore(
    webTask,            // Task function  
    "WebTask",          // Task name
    4096,               // Stack size (4KB)
    NULL,               // Parameters
    3,                  // Priority (highest)
    &webTaskHandle,     // Task handle
    1                   // Core 1
  );
  
  // LED background effects task on core 0
  xTaskCreatePinnedToCore(
    ledTask,            // Task function
    "LEDTask",          // Task name  
    4096,               // Stack size (4KB)
    NULL,               // Parameters
    1,                  // Priority (lower than audio)
    &ledTaskHandle,     // Task handle
    0                   // Core 0
  );
  
  Serial.println("RTOS tasks created successfully!");
  Serial.printf("Main loop running on core %d\n", xPortGetCoreID());
}

void loop() {
  // Main loop is now minimal - most work handled by RTOS tasks
  // Just handle WiFi connection monitoring
  static unsigned long lastWiFiCheck = 0;
  
  if (millis() - lastWiFiCheck > 30000) { // Check every 30 seconds
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected, attempting reconnection...");
      WiFi.reconnect();
    }
    lastWiFiCheck = millis();
  }
  
  // Small delay to prevent watchdog issues and allow other tasks to run
  delay(100);
}

// Check DFPlayer status periodically
void checkDFPlayerStatus() {
  // Check every 10 seconds
  if (millis() - lastStatusCheck < STATUS_CHECK_INTERVAL_MS) {
    return;
  }
  lastStatusCheck = millis();
  
  // Only check if we think it's connected, or if it was disconnected and we want to retry
  if (dfPlayerConnected || dfPlayerStatus == "No Response" || dfPlayerStatus == "Serial Init Failed") {
    int volume = dfPlayer.readVolume();
    delay(100); // CRITICAL: DFPlayer communication - keep this one
    
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

// WiFi setup with captive portal and performance optimizations
void setupWiFi() {
  Serial.println(F("Setting up WiFi..."));
  
  // WiFi performance optimizations
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false); // Reduce flash wear
  WiFi.setSleep(WIFI_PS_NONE); // Disable power saving for responsiveness
  
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

// Initialize modern web interface
void setupWebServer() {
  initWebInterface();
}

// Resume idle audio after weapon effects finish
void resumeIdleAudio() {
  if (dfPlayerConnected) {
    dfPlayer.loop(AUDIO_IDLE);
    currentTrack = AUDIO_IDLE;
    dfPlayerPlaying = true;
    idleAudioActive = true;
    triggerActivity();
    Serial.println("Auto-resumed idle audio after weapon effect");
  }
}

// Compare two version strings (returns 1 if remote > current, 0 if equal, -1 if current > remote)
int compareVersions(String current, String remote) {
  // Remove 'v' prefix if present
  if (current.startsWith("v")) current = current.substring(1);
  if (remote.startsWith("v")) remote = remote.substring(1);
  
  // Simple string comparison works for semantic versioning like 0.16.6
  if (remote > current) return 1;
  if (remote == current) return 0;
  return -1;
}

// Check for firmware updates from battlesync.me
bool checkForUpdates(String &newVersion, String &downloadUrl, String &changelog) {
  HTTPClient http;
  http.begin("https://battlesync.me/api/battleaura/firmware/latest");
  http.addHeader("User-Agent", "BattleAura/" + String(FIRMWARE_VERSION));
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Update check response: " + payload);
    
    JsonDocument doc;
    deserializeJson(doc, payload);
    
    newVersion = doc["version"].as<String>();
    downloadUrl = doc["download_url"].as<String>();
    changelog = doc["changelog"].as<String>();
    
    http.end();
    
    // Compare versions
    return compareVersions(FIRMWARE_VERSION, newVersion) > 0;
  } else {
    Serial.println("Update check failed: " + String(httpCode));
    http.end();
    return false;
  }
}

