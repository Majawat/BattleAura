#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include "../config/Configuration.h"
#include "../hardware/LedController.h"

namespace BattleAura {

class WebServer {
public:
    WebServer(Configuration& config, LedController& ledController);
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
    void handleGetStatus(AsyncWebServerRequest* request);
    
    // Utility
    void sendCORSHeaders(AsyncWebServerRequest* request);
    void sendJSONResponse(AsyncWebServerRequest* request, int code, const String& json);
};

} // namespace BattleAura