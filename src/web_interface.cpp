#include "web_interface.h"
#include "effects.h"
#include "dfplayer.h"

// Global instances
AsyncWebServer server(80);

void initWebInterface() {
  // Initialize SPIFFS for serving static files
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  setupWebRoutes();
  
  server.begin();
  Serial.println("Web interface started on port 80");
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
        request->send(200, "application/json", "{\"status\":\"success\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Invalid volume\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing volume\"}");
    }
  });
  
  server.on("/api/config/brightness", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("brightness", true)) {
      int brightness = request->getParam("brightness", true)->value().toInt();
      if (brightness >= 0 && brightness <= 100) {
        deviceConfig.defaultBrightness = brightness;
        globalBrightness = brightness;
        saveDeviceConfig();
        request->send(200, "application/json", "{\"status\":\"success\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Invalid brightness\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing brightness\"}");
    }
  });
  
  server.on("/api/config/leds", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("enabled", true)) {
      String enabledStr = request->getParam("enabled", true)->value();
      bool enabled = (enabledStr == "true" || enabledStr == "1");
      deviceConfig.hasLEDs = enabled;
      ledsEnabled = enabled;
      saveDeviceConfig();
      request->send(200, "application/json", "{\"status\":\"success\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing enabled\"}");
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
  String response = "{\"deviceName\":\"" + deviceConfig.deviceName + 
                   "\",\"firmwareVersion\":\"" + String(FIRMWARE_VERSION) + 
                   "\",\"buildDate\":\"" + String(BUILD_DATE) + 
                   "\",\"uptime\":" + String(millis() / 1000) +
                   ",\"freeHeap\":" + String(ESP.getFreeHeap()) +
                   ",\"chipModel\":\"" + String(ESP.getChipModel()) +
                   "\",\"flashSize\":" + String(ESP.getFlashChipSize()) + "}";
  request->send(200, "application/json", response);
}

void handleCheckUpdates(AsyncWebServerRequest *request) {
  String response = "{\"updateAvailable\":false,\"currentVersion\":\"" + 
                   String(FIRMWARE_VERSION) + "\",\"message\":\"No updates\"}";
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


String serializeConfig() {
  return "{\"deviceName\":\"" + deviceConfig.deviceName + 
         "\",\"firmwareVersion\":\"" + String(FIRMWARE_VERSION) + 
         "\",\"defaultVolume\":" + String(deviceConfig.defaultVolume) + 
         ",\"defaultBrightness\":" + String(deviceConfig.defaultBrightness) + 
         ",\"hasLEDs\":" + String(deviceConfig.hasLEDs ? "true" : "false") + "}";
}