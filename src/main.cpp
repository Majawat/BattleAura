#include "Arduino.h"
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <Update.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "effects.h"
#include "dfplayer.h"

SoftwareSerial audioSerial(D7, D6); // RX=D7(GPIO20), TX=D6(GPIO21)
DFRobotDFPlayerMini dfPlayer;

// Firmware version constants now in config.h

// Web server and WiFi
WebServer server(80);
WiFiManager wifiManager;

// Audio file constants now in config.h

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
void handleMachineGun();
void handleFlamethrower();
void handleEngineRev();
void handleReconnectDFPlayer();
void handleReboot();
void resumeIdleAudio();
void handleCheckUpdates();
void handlePerformUpdate();
void handleVolumeUp();
void handleVolumeDown();
void handleAudioPause();
void handleAudioPlay();
void handleBrightnessUp();
void handleBrightnessDown();
void handleLedsToggle();
void handleTakingHits();
void handleDestroyed();
void handleRocket();
void handleUnitKill();
void handleConfig();
void handleConfigSave();
int compareVersions(String current, String remote);
bool checkForUpdates(String &newVersion, String &downloadUrl, String &changelog);

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("=== BattleAura ===");
  
  // Load device configuration
  loadDeviceConfig();
  
  // Apply device configuration
  globalBrightness = deviceConfig.defaultBrightness;
  ledsEnabled = deviceConfig.hasLEDs;
  
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
    currentVolume = deviceConfig.defaultVolume;
    dfPlayer.volume(currentVolume);
    delay(500);
    
    int readVolume = dfPlayer.readVolume();
    delay(100);
    
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
}

void loop() {
  // Run all configured background effects dynamically
  runBackgroundEffects();
  
  // Small delay for smooth flicker
  delay(LED_UPDATE_DELAY_MS);
  if (dfPlayer.available()) {
    printDetail(dfPlayer.readType(), dfPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
  
  // Periodic DFPlayer status check (every 10 seconds)
  checkDFPlayerStatus();
  
  // Check if idle audio should timeout to save battery
  checkIdleTimeout();
  
  // Handle actual idle audio stopping (dfPlayer is only accessible here)
  if (!idleAudioActive && dfPlayerConnected && dfPlayerPlaying && currentTrack == AUDIO_IDLE) {
    dfPlayer.stop();
    dfPlayerPlaying = false;
    dfPlayerStatus = "Idle Timeout - Stopped";
    Serial.println("Idle audio stopped - device now in low-power mode");
  }
  
  // Handle web server requests
  server.handleClient();

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
  server.on("/machine-gun", handleMachineGun);
  server.on("/flamethrower", handleFlamethrower);
  server.on("/engine-rev", handleEngineRev);
  server.on("/reconnect-dfplayer", handleReconnectDFPlayer);
  server.on("/reboot", handleReboot);
  server.on("/check-updates", handleCheckUpdates);
  server.on("/perform-update", handlePerformUpdate);
  server.on("/volume-up", handleVolumeUp);
  server.on("/volume-down", handleVolumeDown);
  server.on("/audio-pause", handleAudioPause);
  server.on("/audio-play", handleAudioPlay);
  server.on("/brightness-up", handleBrightnessUp);
  server.on("/brightness-down", handleBrightnessDown);
  server.on("/leds-toggle", handleLedsToggle);
  server.on("/taking-hits", handleTakingHits);
  server.on("/destroyed", handleDestroyed);
  server.on("/rocket", handleRocket);
  server.on("/unit-kill", handleUnitKill);
  server.on("/config", handleConfig);
  server.on("/config-save", HTTP_POST, handleConfigSave);
  
  server.begin();
  Serial.println(F("Web server started"));
}

// Main page with upload form
void handleRoot() {
  String html = F("<!DOCTYPE html><html><head><title>BattleAura</title>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<style>");
  html += F("body{background:#222;color:#fff;font-family:Arial;padding:20px}");
  html += F("h1{color:#5af}h2{color:#a5f;margin-top:20px}");
  html += F("button{background:#4a4;color:#fff;border:none;padding:10px 20px;margin:5px;border-radius:4px}");
  html += F(".warn{background:#fa0}.danger{background:#f44}");
  html += F(".connected{background:#4a4;padding:3px 6px;border-radius:3px}");
  html += F(".disconnected{background:#f44;padding:3px 6px;border-radius:3px}");
  html += F("input[type=file]{width:100%;padding:8px;margin:10px 0}");
  html += F("</style></head><body>");
  
  html += F("<h1>BattleAura</h1>");
  
  html += F("<h2>Audio Controls</h2>");
  html += F("<button onclick=\"window.location='/volume-down'\">Vol -</button>");
  html += F("<button onclick=\"window.location='/volume-up'\">Vol +</button>");
  html += F("<button onclick=\"window.location='/audio-pause'\">Pause</button>");
  html += F("<button onclick=\"window.location='/audio-play'\">Play</button>");
  
  html += F("<h2>Lighting Controls</h2>");
  html += F("<button onclick=\"window.location='/leds-toggle'\">");
  if (ledsEnabled) {
    html += F("LEDs ON");
  } else {
    html += F("LEDs OFF"); 
  }
  html += F("</button>");
  html += F("<button onclick=\"window.location='/brightness-down'\">Bright -</button>");
  html += F("<button onclick=\"window.location='/brightness-up'\">Bright +</button>");
  
  html += F("<h2>Battle Effects</h2>");
  html += F("<button onclick=\"window.location='/machine-gun'\">Machine Gun</button>");
  html += F("<button onclick=\"window.location='/flamethrower'\">Flamethrower</button>");
  html += F("<button onclick=\"window.location='/engine-rev'\">Engine Rev</button>");
  html += F("<button onclick=\"window.location='/rocket'\">Rocket</button>");
  
  html += F("<h2>Battle Events</h2>");
  html += F("<button onclick=\"window.location='/taking-hits'\">Taking Hits</button>");
  html += F("<button class='warn' onclick=\"if(confirm('Simulate destruction?')) window.location='/destroyed'\">Destroyed</button>");
  html += F("<button onclick=\"window.location='/unit-kill'\">Unit Kill</button>");
  
  html += F("<h2>System Status</h2>");
  html += F("<b>DFPlayer:</b> ");
  if (dfPlayerConnected) {
    html += F("<span class='connected'>Connected</span>");
    if (dfPlayerPlaying) {
      html += F(" (Track ");
      html += currentTrack;
      html += F(")");
    }
    html += F("<br><b>Volume:</b> ");
    html += currentVolume;
    html += F("/30");
  } else {
    html += F("<span class='disconnected'>Disconnected</span>");
    html += F(" <button onclick=\"window.location='/reconnect-dfplayer'\">Reconnect</button>");
  }
  
  html += F("<br><b>LEDs:</b> ");
  if (ledsEnabled) {
    html += F("<span class='connected'>ON</span>");
    html += F(" <b>Brightness:</b> ");
    html += globalBrightness;
    html += F("%");
  } else {
    html += F("<span class='disconnected'>OFF</span>");
  }
  
  html += F("<h2>Firmware</h2>");
  html += F("<b>Version:</b> ");
  html += FIRMWARE_VERSION;
  html += F("<br><b>Built:</b> ");
  html += BUILD_DATE;
  html += F("<br><br><button onclick=\"window.location='/check-updates'\">Check for Updates</button>");
  html += F("<br><br><b>Manual Upload:</b>");
  html += F("<form method='POST' action='/upload' enctype='multipart/form-data'>");
  html += F("<input type='file' name='firmware' accept='.bin' required>");
  html += F("<input type='submit' value='Upload'>");
  html += F("</form>");
  
  html += F("<h2>Configuration</h2>");
  html += F("<button onclick=\"window.location='/config'\">Configure Effects</button>");
  
  html += F("<h2>System Controls</h2>");
  html += F("<button onclick=\"if(confirm('Reboot device?')) window.location='/reboot'\">Reboot</button>");
  html += F("<button class='warn' onclick=\"if(confirm('Reset WiFi?')) window.location='/wifi-reset'\">Reset WiFi</button>");
  html += F("<button class='danger' onclick=\"if(confirm('Factory reset?')) window.location='/factory-reset'\">Factory Reset</button>");
  
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

// Handle machine gun effect request
void handleMachineGun() {
  Serial.println("Machine gun triggered via web interface");
  triggerActivity();
  machineGunEffect(&dfPlayer, LED5, AUDIO_WEAPON_FIRE_1);
  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle flamethrower effect request
void handleFlamethrower() {
  Serial.println("Flamethrower triggered via web interface");
  triggerActivity();
  flamethrowerEffect(&dfPlayer, LED6, AUDIO_WEAPON_FIRE_2);
  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle engine rev effect request
void handleEngineRev() {
  Serial.println("Engine rev triggered via web interface");
  triggerActivity();
  engineRevEffect(&dfPlayer, LED7, LED8, AUDIO_ENGINE_REV);
  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle DFPlayer reconnect request
void handleReconnectDFPlayer() {
  Serial.println("DFPlayer reconnect requested via web interface");
  triggerActivity();
  
  // Attempt to reconnect DFPlayer
  dfPlayer.begin(audioSerial, false, false);
  delay(1000);
  
  // Test communication
  dfPlayer.volume(currentVolume);
  delay(500);
  
  int readVolume = dfPlayer.readVolume();
  delay(100);
  
  if (readVolume > 0 && readVolume <= 30) {
    Serial.println("DFPlayer reconnected successfully!");
    currentVolume = readVolume;
    dfPlayerConnected = true;
    dfPlayerStatus = "Reconnected";
    // Start idle audio
    dfPlayer.loop(AUDIO_IDLE);
    currentTrack = AUDIO_IDLE;
    dfPlayerPlaying = true;
    idleAudioActive = true;
    triggerActivity();
  } else {
    Serial.println("DFPlayer reconnection failed");
    dfPlayerConnected = false;
    dfPlayerStatus = "Reconnection Failed";
  }
  
  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle device reboot request
void handleReboot() {
  Serial.println("Device reboot requested via web interface");
  server.send(200, "text/html", 
    F("<html><body style='background:#222;color:#fff;font-family:Arial;padding:20px;text-align:center'>"
      "<h1>Rebooting...</h1><p>Device is restarting</p>"
      "<script>setTimeout(function(){window.location.href='/';}, 10000);</script>"
      "</body></html>"));
  
  delay(1000);
  Serial.println("Rebooting device...");
  ESP.restart();
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

// Handle check for updates request
void handleCheckUpdates() {
  Serial.println("Checking for firmware updates...");
  
  String newVersion, downloadUrl, changelog;
  bool updateAvailable = checkForUpdates(newVersion, downloadUrl, changelog);
  
  String html = F("<!DOCTYPE html><html><head><title>Updates</title>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<style>body{background:#222;color:#fff;font-family:Arial;padding:20px}");
  html += F("button{background:#4a4;color:#fff;border:none;padding:10px 20px;margin:5px;border-radius:4px}");
  html += F(".update{background:#44a}</style></head><body>");
  
  html += F("<h1>Update Check</h1>");
  html += F("<b>Current:</b> ");
  html += String(FIRMWARE_VERSION);
  html += F("<br>");
  
  if (updateAvailable) {
    html += F("<b>New Version:</b> ");
    html += newVersion;
    html += F("<br><b>Changes:</b> ");
    html += changelog;
    html += F("<br><br>");
    html += F("<button class='update' onclick=\"if(confirm('Install ");
    html += newVersion;
    html += F("?')) window.location='/perform-update?url=");
    html += downloadUrl;
    html += F("'\">Install Update</button>");
  } else {
    html += F("<b>Status:</b> Latest version!<br>");
  }
  
  html += F("<br><button onclick=\"window.location='/'\">Back</button>");
  html += F("</body></html>");
  
  server.send(200, "text/html", html);
}

// Handle firmware update download and installation
void handlePerformUpdate() {
  String updateUrl = server.arg("url");
  
  if (updateUrl.length() == 0) {
    server.send(400, "text/html", F("<html><body><h1>Error</h1><p>No update URL provided</p></body></html>"));
    return;
  }
  
  Serial.println("Starting firmware update from: " + updateUrl);
  
  // Get current host for reconnection
  String currentHost = server.hostHeader();
  
  // Send immediate response with auto-reconnect
  String updateHtml = F("<!DOCTYPE html><html><head>");
  updateHtml += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  updateHtml += F("<style>body{background:#222;color:#fff;font-family:Arial;padding:20px;text-align:center}</style>");
  updateHtml += F("<script>");
  updateHtml += F("let countdown = 30;");
  updateHtml += F("function updateCountdown() {");
  updateHtml += F("  document.getElementById('countdown').textContent = countdown;");
  updateHtml += F("  if (countdown <= 0) {");
  updateHtml += F("    document.getElementById('status').innerHTML = 'Reconnecting...';");
  updateHtml += F("    window.location.href = 'http://");
  updateHtml += currentHost;
  updateHtml += F("';");
  updateHtml += F("  } else {");
  updateHtml += F("    countdown--;");
  updateHtml += F("    setTimeout(updateCountdown, 1000);");
  updateHtml += F("  }");
  updateHtml += F("}");
  updateHtml += F("setTimeout(updateCountdown, 1000);");
  updateHtml += F("</script>");
  updateHtml += F("</head><body>");
  updateHtml += F("<h1>Updating...</h1>");
  updateHtml += F("<p>Installing firmware update</p>");
  updateHtml += F("<p>Device will restart when complete</p>");
  updateHtml += F("<div id='status'>Auto-reconnecting in <span id='countdown'>30</span> seconds</div>");
  updateHtml += F("<p><a href='http://");
  updateHtml += currentHost;
  updateHtml += F("'>Manual reconnect</a></p>");
  updateHtml += F("</body></html>");
  
  server.send(200, "text/html", updateHtml);
  
  // Give time for response to send
  delay(1000);
  
  // Start the actual update process
  HTTPClient http;
  http.begin(updateUrl);
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    
    if (contentLength > 0) {
      bool canBegin = Update.begin(contentLength);
      
      if (canBegin) {
        Serial.println("Starting OTA update...");
        WiFiClient * client = http.getStreamPtr();
        
        size_t written = Update.writeStream(*client);
        
        if (written == contentLength) {
          Serial.println("OTA update written successfully");
        } else {
          Serial.println("OTA update write failed");
        }
        
        if (Update.end()) {
          if (Update.isFinished()) {
            Serial.println("OTA update completed successfully! Restarting...");
            ESP.restart();
          } else {
            Serial.println("OTA update not finished");
          }
        } else {
          Serial.println("OTA update end failed");
        }
      } else {
        Serial.println("OTA update begin failed");
      }
    }
  } else {
    Serial.println("HTTP GET failed: " + String(httpCode));
  }
  
  http.end();
}

// Handle volume up request
void handleVolumeUp() {
  if (dfPlayerConnected && currentVolume < 30) {
    currentVolume++;
    dfPlayer.volume(currentVolume);
    Serial.println("Volume up: " + String(currentVolume));
  }
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle volume down request  
void handleVolumeDown() {
  if (dfPlayerConnected && currentVolume > 0) {
    currentVolume--;
    dfPlayer.volume(currentVolume);
    Serial.println("Volume down: " + String(currentVolume));
  }
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle audio pause request
void handleAudioPause() {
  if (dfPlayerConnected) {
    dfPlayer.pause();
    dfPlayerPlaying = false;
    dfPlayerStatus = "Paused";
    Serial.println("Audio paused");
  }
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle audio play request
void handleAudioPlay() {
  triggerActivity();
  if (dfPlayerConnected) {
    if (!dfPlayerPlaying) {
      dfPlayer.start();
      dfPlayerPlaying = true;
      dfPlayerStatus = "Playing";
      Serial.println("Audio resumed");
    } else {
      // Start idle loop if nothing is playing
      dfPlayer.loop(AUDIO_IDLE);
      currentTrack = AUDIO_IDLE;
      dfPlayerPlaying = true;
      idleAudioActive = true;
      dfPlayerStatus = "Playing Idle";
      Serial.println("Playing idle audio");
    }
  }
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle brightness up request
void handleBrightnessUp() {
  if (globalBrightness < 100) {
    globalBrightness += 10;
    if (globalBrightness > 100) globalBrightness = 100;
    Serial.println("Brightness up: " + String(globalBrightness) + "%");
  }
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle brightness down request  
void handleBrightnessDown() {
  if (globalBrightness > 0) {
    globalBrightness -= 10;
    if (globalBrightness < 0) globalBrightness = 0;
    Serial.println("Brightness down: " + String(globalBrightness) + "%");
  }
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle LEDs toggle request
void handleLedsToggle() {
  ledsEnabled = !ledsEnabled;
  Serial.println("LEDs " + String(ledsEnabled ? "enabled" : "disabled"));
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle taking hits request
void handleTakingHits() {
  Serial.println("Taking hits triggered via web interface");
  triggerActivity();
  takingHitsEffect(&dfPlayer, AUDIO_TAKING_HITS);
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle destroyed request
void handleDestroyed() {
  Serial.println("CRITICAL: Destroyed effect triggered via web interface");
  triggerActivity();
  destroyedEffect(&dfPlayer, AUDIO_DESTROYED);
  // Note: This function will shutdown the device, so no redirect needed
}

// Handle rocket request  
void handleRocket() {
  Serial.println("Rocket fired via web interface");
  triggerActivity();
  rocketEffect(&dfPlayer, AUDIO_LIMITED_WEAPON);
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle unit kill request
void handleUnitKill() {
  Serial.println("Unit kill confirmed via web interface");
  triggerActivity();
  unitKillEffect(&dfPlayer, AUDIO_UNIT_KILL);
  server.sendHeader("Location", "/");
  server.send(302);
}

// Handle configuration page request
void handleConfig() {
  String html = F("<!DOCTYPE html><html><head><title>BattleAura Configuration</title>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<style>");
  html += F("body{background:#222;color:#fff;font-family:Arial;padding:20px}");
  html += F("h1{color:#5af}h2{color:#a5f;margin-top:20px}");
  html += F("table{width:100%;border-collapse:collapse;margin:10px 0}");
  html += F("th,td{border:1px solid #666;padding:8px;text-align:left}");
  html += F("th{background:#333}");
  html += F("select,input{padding:5px;margin:2px;background:#333;color:#fff;border:1px solid #666}");
  html += F("button{background:#4a4;color:#fff;border:none;padding:10px 20px;margin:5px;border-radius:4px}");
  html += F(".save{background:#44a;font-size:16px;padding:15px 30px}");
  html += F("</style></head><body>");
  
  html += F("<h1>Effect Configuration</h1>");
  html += F("<form method='POST' action='/config-save'>");
  
  html += F("<h2>Background Effects</h2>");
  html += F("<table><tr><th>Pin</th><th>Effect Type</th><th>Label</th><th>Enabled</th></tr>");
  
  // Generate background effects table
  const char* effectTypes[] = {"off", "candle", "pulse", "console", "static"};
  const char* pinNames[] = {"D0", "D1", "D2", "D3", "D4", "D5", "D8", "D9"};
  
  for (int i = 0; i < 8; i++) {
    html += F("<tr><td>");
    html += pinNames[i];
    html += F("</td><td><select name='bg_effect_");
    html += String(i);
    html += F("'>");
    
    for (int j = 0; j < 5; j++) {
      html += F("<option value='");
      html += effectTypes[j];
      html += F("'");
      if (deviceConfig.backgroundEffects[i].effectType == effectTypes[j]) {
        html += F(" selected");
      }
      html += F(">");
      html += effectTypes[j];
      html += F("</option>");
    }
    
    html += F("</select></td><td><input type='text' name='bg_label_");
    html += String(i);
    html += F("' value='");
    html += deviceConfig.backgroundEffects[i].label;
    html += F("' placeholder='Label'></td><td><input type='checkbox' name='bg_enabled_");
    html += String(i);
    html += F("'");
    if (deviceConfig.backgroundEffects[i].enabled) {
      html += F(" checked");
    }
    html += F("></td></tr>");
  }
  html += F("</table>");
  
  html += F("<br><button type='submit' class='save'>Save Configuration</button>");
  html += F("<button type='button' onclick=\"window.location='/'\">Back to Main</button>");
  html += F("</form></body></html>");
  
  server.send(200, "text/html", html);
}

// Handle configuration save request
void handleConfigSave() {
  Serial.println("Saving configuration from web interface");
  
  // Update background effects from form data
  for (int i = 0; i < 8; i++) {
    String effectParam = "bg_effect_" + String(i);
    String labelParam = "bg_label_" + String(i);
    String enabledParam = "bg_enabled_" + String(i);
    
    if (server.hasArg(effectParam)) {
      deviceConfig.backgroundEffects[i].effectType = server.arg(effectParam);
    }
    if (server.hasArg(labelParam)) {
      deviceConfig.backgroundEffects[i].label = server.arg(labelParam);
    }
    deviceConfig.backgroundEffects[i].enabled = server.hasArg(enabledParam);
    
    Serial.println("Pin " + String(i) + ": " + deviceConfig.backgroundEffects[i].effectType + 
                   " (" + deviceConfig.backgroundEffects[i].label + ") " + 
                   (deviceConfig.backgroundEffects[i].enabled ? "ON" : "OFF"));
  }
  
  // Save configuration (placeholder for now)
  saveDeviceConfig();
  
  // Redirect back to main page with success message
  String html = F("<!DOCTYPE html><html><head>");
  html += F("<meta http-equiv='refresh' content='2;url=/'>");
  html += F("<style>body{background:#222;color:#fff;font-family:Arial;padding:20px;text-align:center}</style>");
  html += F("</head><body><h1>Configuration Saved!</h1>");
  html += F("<p>Effects will be active on next loop cycle</p>");
  html += F("<p>Redirecting...</p></body></html>");
  
  server.send(200, "text/html", html);
}
