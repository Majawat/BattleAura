#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <SPIFFS.h>
#include "config.h"
#include "effects.h"
#include "audio.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void setupWiFi();
void setupWebServer();
void setupOTA();
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println(F("BattleAura Starting..."));
  Serial.print(F("Version: ")); Serial.println(F(BATTLEARUA_VERSION));
  Serial.print(F("Build: ")); Serial.println(F(BATTLEARUA_BUILD_DATE));
  
  if (!ConfigManager::getInstance().begin()) {
    Serial.println(F("Config init failed"));
    return;
  }
  
  EffectManager::getInstance().begin();
  
  if (!AudioManager::getInstance().begin()) {
    Serial.println(F("Audio init failed"));
  }
  
  setupWiFi();
  setupWebServer();
  setupOTA();
  
  Serial.println(F("Ready!"));
  SystemConfig& config = ConfigManager::getInstance().getConfig();
  Serial.print(F("Device: ")); Serial.println(config.deviceName);
  Serial.print(F("IP: ")); Serial.println(WiFi.localIP());
}

void loop() {
  ArduinoOTA.handle();
  AudioManager::getInstance().update();
  EffectManager::getInstance().update();
  ws.cleanupClients();
  
  delay(10);
}

void setupWiFi() {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  if (!config.wifiEnabled || config.wifiSSID.length() == 0) {
    Serial.println("Starting AP mode for configuration");
    WiFi.softAP(config.deviceName.c_str(), "battlesync");
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    return;
  }
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSSID.c_str(), config.wifiPassword.c_str());
  
  Serial.printf("Connecting to %s", config.wifiSSID.c_str());
  
  uint32_t startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nConnected to WiFi: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nFailed to connect, starting AP mode");
    WiFi.softAP(config.deviceName.c_str(), "battlesync");
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  }
}

void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });
  
  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/app.js", "application/javascript");
  });
  
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    SystemConfig& config = ConfigManager::getInstance().getConfig();
    
    JsonDocument doc;
    doc["version"] = BATTLEARUA_VERSION;
    doc["buildDate"] = BATTLEARUA_BUILD_DATE;
    doc["deviceName"] = config.deviceName;
    doc["wifiSSID"] = config.wifiSSID;
    doc["wifiEnabled"] = config.wifiEnabled;
    doc["volume"] = config.volume;
    doc["audioEnabled"] = config.audioEnabled;
    doc["activePins"] = config.activePins;
    
    JsonArray pinsArray = doc["pins"].to<JsonArray>();
    for (int i = 0; i < MAX_PINS; i++) {
      JsonObject pinObj = pinsArray.add<JsonObject>();
      pinObj["pin"] = config.pins[i].pin;
      pinObj["type"] = static_cast<int>(config.pins[i].type);
      pinObj["effect"] = static_cast<int>(config.pins[i].effect);
      pinObj["name"] = config.pins[i].name;
      pinObj["audioFile"] = config.pins[i].audioFile;
      pinObj["enabled"] = config.pins[i].enabled;
      pinObj["brightness"] = config.pins[i].brightness;
      pinObj["color"] = config.pins[i].color;
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Handle config updates
    request->send(200, "text/plain", "Config updated");
  });
  
  server.on("/trigger", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("pin") && request->hasParam("effect")) {
      uint8_t pin = request->getParam("pin")->value().toInt();
      uint8_t effectType = request->getParam("effect")->value().toInt();
      uint32_t duration = request->hasParam("duration") ? 
                         request->getParam("duration")->value().toInt() : 0;
      
      EffectManager::getInstance().triggerEffect(pin, static_cast<EffectType>(effectType), duration);
      
      if (request->hasParam("audio")) {
        uint8_t audioFile = request->getParam("audio")->value().toInt();
        AudioManager::getInstance().playFile(audioFile);
      }
      
      request->send(200, "text/plain", "Effect triggered");
    } else {
      request->send(400, "text/plain", "Missing parameters");
    }
  });
  
  server.on("/audio", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("action")) {
      String action = request->getParam("action")->value();
      
      if (action == "play" && request->hasParam("file")) {
        uint8_t file = request->getParam("file")->value().toInt();
        bool loop = request->hasParam("loop") && request->getParam("loop")->value() == "true";
        AudioManager::getInstance().playFile(file, loop);
      } else if (action == "stop") {
        AudioManager::getInstance().stopPlaying();
      } else if (action == "pause") {
        AudioManager::getInstance().pausePlayback();
      } else if (action == "resume") {
        AudioManager::getInstance().resumePlayback();
      } else if (action == "volume" && request->hasParam("level")) {
        uint8_t volume = request->getParam("level")->value().toInt();
        AudioManager::getInstance().setVolume(volume);
      }
      
      request->send(200, "text/plain", "Audio command processed");
    } else {
      request->send(400, "text/plain", "Missing action");
    }
  });
  
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.begin();
  
  Serial.println("Web server started");
}

void setupOTA() {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  ArduinoOTA.setHostname(config.deviceName.c_str());
  ArduinoOTA.setPassword("battlesync");
  
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("Start updating " + type);
    EffectManager::getInstance().stopAllEffects();
    AudioManager::getInstance().stopPlaying();
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
      Serial.println("Failed to parse WebSocket message");
      return;
    }
    
    String command = doc["command"];
    
    if (command == "trigger_effect") {
      uint8_t pin = doc["pin"];
      uint8_t effect = doc["effect"];
      uint32_t duration = doc["duration"] | 0;
      
      EffectManager::getInstance().triggerEffect(pin, static_cast<EffectType>(effect), duration);
      
      if (doc["audio"].is<uint8_t>()) {
        uint8_t audioFile = doc["audio"];
        AudioManager::getInstance().playFile(audioFile);
      }
    } else if (command == "stop_effect") {
      uint8_t pin = doc["pin"];
      EffectManager::getInstance().stopEffect(pin);
    } else if (command == "play_audio") {
      uint8_t file = doc["file"];
      bool loop = doc["loop"] | false;
      AudioManager::getInstance().playFile(file, loop);
    } else if (command == "stop_audio") {
      AudioManager::getInstance().stopPlaying();
    }
  }
}