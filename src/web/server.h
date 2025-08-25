#pragma once

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include "../core/config.h"

class WebServerManager {
public:
    static WebServerManager& getInstance();
    
    bool initialize(uint16_t port = 80);
    void handleClient();
    bool isRunning() const { return running; }
    
    // WiFi Management
    bool startWiFiAP(const String& ssid, const String& password);
    bool connectWiFiStation(const String& ssid, const String& password);
    String getIPAddress() const;
    
private:
    WebServerManager() = default;
    ~WebServerManager() = default;
    WebServerManager(const WebServerManager&) = delete;
    WebServerManager& operator=(const WebServerManager&) = delete;
    
    WebServer* server = nullptr;
    bool running = false;
    
    void setupRoutes();
    void setupStaticFileHandlers();
    void setupAPIHandlers();
    
    // Static file handlers
    void handleRoot();
    void handleNotFound();
    void serveStaticFile(const String& path, const String& contentType);
    
    // API handlers
    void handleGetConfig();
    void handleGetStatus();
    void handlePinEffect();
    void handlePinState();
    void handleAudioPlay();
    void handleAudioStop();
    void handleAudioVolume();
    void handleGlobalEffect();
    void handleStopAllEffects();
    void handleFactoryReset();
    
    // Utility methods
    void sendJsonResponse(int code, const String& message);
    void sendCorsHeaders();
    String getContentType(const String& filename);
};