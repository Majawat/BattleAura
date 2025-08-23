#include "web_interface.h"
#include "effects.h"
#include "dfplayer.h"

// Global instances
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void initWebInterface() {
  // Initialize SPIFFS for serving static files
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  setupWebSocket();
  setupWebRoutes();
  
  server.begin();
  Serial.println("Modern web interface started on port 80");
  Serial.println("Access at: http://battlearua.local or http://[device-ip]");
}

void setupWebSocket() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
}

void setupWebRoutes() {
  // Serve static files from SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  
  // API Routes
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/system/info", HTTP_GET, handleSystemInfo);
  server.on("/api/system/check-updates", HTTP_GET, handleCheckUpdates);
  server.on("/api/system/factory-reset", HTTP_POST, handleFactoryReset);
  
  // Simple POST routes with URL parameters
  server.on("/api/config/volume", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("volume", true)) {
      int volume = request->getParam("volume", true)->value().toInt();
      if (volume >= 0 && volume <= 30) {
        deviceConfig.defaultVolume = volume;
        dfPlayer.volume(volume);
        saveDeviceConfig();
        broadcastConfigUpdated();
        request->send(200, "application/json", "{\"status\":\"success\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Volume must be between 0 and 30\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing volume parameter\"}");
    }
  });
  
  server.on("/api/config/brightness", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("brightness", true)) {
      int brightness = request->getParam("brightness", true)->value().toInt();
      if (brightness >= 0 && brightness <= 100) {
        deviceConfig.defaultBrightness = brightness;
        globalBrightness = brightness;
        saveDeviceConfig();
        broadcastConfigUpdated();
        request->send(200, "application/json", "{\"status\":\"success\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Brightness must be between 0 and 100\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing brightness parameter\"}");
    }
  });
  
  server.on("/api/config/leds", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("enabled", true)) {
      String enabledStr = request->getParam("enabled", true)->value();
      bool enabled = (enabledStr == "true" || enabledStr == "1");
      deviceConfig.hasLEDs = enabled;
      ledsEnabled = enabled;
      saveDeviceConfig();
      broadcastConfigUpdated();
      request->send(200, "application/json", "{\"status\":\"success\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing enabled parameter\"}");
    }
  });
  
  // Trigger effect endpoints
  server.on("^\\/api\\/trigger\\/(.+)$", HTTP_POST, [](AsyncWebServerRequest *request) {
    String effectId = request->pathArg(0);
    bool success = false;
    
    if (effectId == "weapon1") {
      TriggerEffect* effect = getEffectById("weapon1");
      if (effect && effect->enabled) {
        startWeaponEffect(0, &dfPlayer, effect->primaryPin, effect->audioTrack, "machine-gun");
        success = true;
      }
    }
    else if (effectId == "weapon2") {
      TriggerEffect* effect = getEffectById("weapon2");
      if (effect && effect->enabled) {
        startWeaponEffect(1, &dfPlayer, effect->primaryPin, effect->audioTrack, "flamethrower");
        success = true;
      }
    }
    else if (effectId == "engine-rev") {
      TriggerEffect* effect = getEffectById("engine-rev");
      if (effect && effect->enabled) {
        startWeaponEffect(2, &dfPlayer, effect->primaryPin, effect->audioTrack, "engine-rev");
        success = true;
      }
    }
    else if (effectId == "rocket") {
      rocketEffect(&dfPlayer, AUDIO_LIMITED_WEAPON);
      success = true;
    }
    else if (effectId == "taking-hits") {
      takingHitsEffect(&dfPlayer, AUDIO_TAKING_HITS);
      success = true;
    }
    else if (effectId == "destroyed") {
      destroyedEffect(&dfPlayer, AUDIO_DESTROYED);
      success = true;
    }
    else if (effectId == "unit-kill") {
      unitKillEffect(&dfPlayer, AUDIO_UNIT_KILL);
      success = true;
    }
    
    if (success) {
      broadcastEffectStarted(effectId);
      request->send(200, "application/json", "{\"status\":\"success\"}");
    } else {
      request->send(404, "application/json", "{\"error\":\"Unknown effect\"}");
    }
  });

  // CORS headers
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
}

void handleGetConfig(AsyncWebServerRequest *request) {
  String configJson = serializeConfig();
  request->send(200, "application/json", configJson);
}

void handleSystemInfo(AsyncWebServerRequest *request) {
  JsonDocument doc;
  
  doc["deviceName"] = deviceConfig.deviceName;
  doc["firmwareVersion"] = FIRMWARE_VERSION;
  doc["versionFeature"] = VERSION_FEATURE;
  doc["buildDate"] = BUILD_DATE;
  doc["uptime"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["chipModel"] = ESP.getChipModel();
  doc["chipRevision"] = ESP.getChipRevision();
  doc["flashSize"] = ESP.getFlashChipSize();
  doc["activePins"] = deviceConfig.activePins;
  doc["activeAudioTracks"] = deviceConfig.activeAudioTracks;
  doc["activeEffects"] = deviceConfig.activeEffects;
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void handleCheckUpdates(AsyncWebServerRequest *request) {
  // Placeholder for update checking
  JsonDocument doc;
  doc["updateAvailable"] = false;
  doc["currentVersion"] = FIRMWARE_VERSION;
  doc["message"] = "Update checking not implemented yet";
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void handleFactoryReset(AsyncWebServerRequest *request) {
  request->send(200, "application/json", "{\"status\":\"Factory reset initiated\"}");
  
  // Delay to allow response to be sent
  delay(100);
  
  // Clear SPIFFS configuration
  SPIFFS.remove("/config.json");
  
  // Restart device
  ESP.restart();
}

// WebSocket event handler
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      // Send initial config to new client
      client->text(serializeConfig());
      break;
      
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
      
    case WS_EVT_DATA: {
      AwsFrameInfo *info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0; // Null terminate
        String message = (char*)data;
        Serial.println("WebSocket message: " + message);
        
        // Handle WebSocket commands here if needed
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error && doc["command"].is<String>()) {
          String command = doc["command"];
          if (command == "ping") {
            client->text("{\"type\":\"pong\"}");
          }
        }
      }
      break;
    }
      
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void broadcastToWebSocket(const String& message) {
  ws.textAll(message);
}

void broadcastEffectStarted(const String& effectId) {
  JsonDocument doc;
  doc["type"] = "effect-started";
  doc["data"]["effectId"] = effectId;
  doc["data"]["timestamp"] = millis();
  
  String message;
  serializeJson(doc, message);
  broadcastToWebSocket(message);
}

void broadcastConfigUpdated() {
  JsonDocument doc;
  doc["type"] = "config-updated";
  doc["timestamp"] = millis();
  
  String message;
  serializeJson(doc, message);
  broadcastToWebSocket(message);
}

String serializeConfig() {
  JsonDocument doc;
  
  // Only essential device info for simple UI
  doc["deviceName"] = deviceConfig.deviceName;
  doc["firmwareVersion"] = FIRMWARE_VERSION;
  doc["defaultVolume"] = deviceConfig.defaultVolume;
  doc["defaultBrightness"] = deviceConfig.defaultBrightness;
  doc["hasLEDs"] = deviceConfig.hasLEDs;
  
  String result;
  serializeJson(doc, result);
  return result;
}