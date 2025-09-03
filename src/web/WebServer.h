#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include "../config/Configuration.h"
#include "../hardware/LedController.h"
#include "../effects/EffectManager.h"

namespace BattleAura {

class WebServer {
public:
    WebServer(Configuration& config, LedController& ledController, EffectManager& effectManager);
    ~WebServer();
    
    // Initialization
    bool begin();
    void handle();
    
    // WiFi management
    bool connectToWiFi();
    void startAccessPoint();
    bool isWiFiConnected() const;
    String getIPAddress() const;
    
    // Status
    void printStatus() const;

private:
    Configuration& config;
    LedController& ledController;
    EffectManager& effectManager;
    AsyncWebServer server;
    bool wifiConnected;
    bool apMode;
    String currentIP;
    
    // Setup methods
    void setupRoutes();
    void setupOTA();
    
    // Route handlers
    void handleRoot(AsyncWebServerRequest* request);
    void handleGetZones(AsyncWebServerRequest* request);
    void handleSetBrightness(AsyncWebServerRequest* request);
    void handleSetBrightnessBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleAddZone(AsyncWebServerRequest* request);
    void handleAddZoneBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleDeleteZone(AsyncWebServerRequest* request);
    void handleClearZones(AsyncWebServerRequest* request);
    void handleGetEffects(AsyncWebServerRequest* request);
    void handleTriggerEffect(AsyncWebServerRequest* request);
    void handleTriggerEffectBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleOTAUpload(AsyncWebServerRequest* request);
    void handleOTAUploadFile(AsyncWebServerRequest* request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    
    // Utility
    void sendCORSHeaders(AsyncWebServerRequest* request);
    void sendJSONResponse(AsyncWebServerRequest* request, int code, const String& json);
};

} // namespace BattleAura