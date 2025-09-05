#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <ESPmDNS.h>
#include "../config/Configuration.h"
#include "../hardware/LedController.h"
#include "../vfx/VFXManager.h"
#include "../audio/AudioController.h"

namespace BattleAura {

class WebServer {
public:
    WebServer(Configuration& config, LedController& ledController, VFXManager& vfxManager, AudioController& audioController);
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
    VFXManager& vfxManager;
    AudioController& audioController;
    AsyncWebServer server;
    bool wifiConnected;
    bool apMode;
    String currentIP;
    
    // Setup methods
    void setupRoutes();
    void setupOTA();
    void setupmDNS();
    
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
    void handleStopAllEffects(AsyncWebServerRequest* request);
    void handlePlayAudio(AsyncWebServerRequest* request);
    void handlePlayAudioBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleStopAudio(AsyncWebServerRequest* request);
    void handleSetVolume(AsyncWebServerRequest* request);
    void handleSetVolumeBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleGetAudioStatus(AsyncWebServerRequest* request);
    void handleRetryAudio(AsyncWebServerRequest* request);
    void handleGetAudioTracks(AsyncWebServerRequest* request);
    void handleAddAudioTrack(AsyncWebServerRequest* request);
    void handleAddAudioTrackBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleDeleteAudioTrack(AsyncWebServerRequest* request);
    void handleGetSceneConfigs(AsyncWebServerRequest* request);
    void handleAddSceneConfigBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleDeleteSceneConfigBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleDeviceConfigBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleSystemRestart(AsyncWebServerRequest* request);
    void handleFactoryReset(AsyncWebServerRequest* request);
    void handleGetGlobalBrightness(AsyncWebServerRequest* request);
    void handleSetGlobalBrightnessBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleWiFiConfig(AsyncWebServerRequest* request);
    void handleWiFiConfigBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleClearWiFi(AsyncWebServerRequest* request);
    void handleOTAUpload(AsyncWebServerRequest* request);
    void handleOTAUploadFile(AsyncWebServerRequest* request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    
    // Utility
    void sendCORSHeaders(AsyncWebServerRequest* request);
    void sendJSONResponse(AsyncWebServerRequest* request, int code, const String& json);
    String generateHostname(const String& deviceName);
    void parseJSONBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total, void (WebServer::*handler)(AsyncWebServerRequest*, JsonDocument&));
    
    // JSON processing handlers
    void processAddAudioTrack(AsyncWebServerRequest* request, JsonDocument& doc);
    void processAddSceneConfig(AsyncWebServerRequest* request, JsonDocument& doc);
    void processDeleteSceneConfig(AsyncWebServerRequest* request, JsonDocument& doc);
    void processDeviceConfig(AsyncWebServerRequest* request, JsonDocument& doc);
    void processSetGlobalBrightness(AsyncWebServerRequest* request, JsonDocument& doc);
    
    // JSON business logic handlers
    void processWiFiConfig(AsyncWebServerRequest* request, JsonDocument& doc);
    void processSetVolume(AsyncWebServerRequest* request, JsonDocument& doc);
};

} // namespace BattleAura