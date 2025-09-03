#include "WebServer.h"
#include "WebInterface.h"
#include <ArduinoJson.h>

namespace BattleAura {

WebServer::WebServer(Configuration& config, LedController& ledController, EffectManager& effectManager, AudioController& audioController) 
    : config(config), ledController(ledController), effectManager(effectManager), audioController(audioController), server(80), 
      wifiConnected(false), apMode(false) {
}

WebServer::~WebServer() {
    server.end();
}

bool WebServer::begin() {
    Serial.println("WebServer: Starting...");
    
    // Try to connect to WiFi first
    if (connectToWiFi()) {
        Serial.printf("WebServer: Connected to WiFi, IP: %s\n", currentIP.c_str());
    } else {
        Serial.println("WebServer: WiFi failed, starting AP mode");
        startAccessPoint();
    }
    
    // Setup web routes
    setupRoutes();
    
    // Setup OTA updates
    setupOTA();
    
    // Start the server
    server.begin();
    
    Serial.println("WebServer: Ready");
    return true;
}

void WebServer::handle() {
    // Handle OTA updates
    ArduinoOTA.handle();
}

bool WebServer::connectToWiFi() {
    const auto& deviceConfig = config.getDeviceConfig();
    
    // Skip if no WiFi credentials
    if (deviceConfig.wifiSSID.isEmpty()) {
        return false;
    }
    
    Serial.printf("WebServer: Connecting to WiFi '%s'...\n", deviceConfig.wifiSSID.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(deviceConfig.wifiSSID.c_str(), deviceConfig.wifiPassword.c_str());
    
    // Wait up to 10 seconds for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        apMode = false;
        currentIP = WiFi.localIP().toString();
        return true;
    }
    
    return false;
}

void WebServer::startAccessPoint() {
    const auto& deviceConfig = config.getDeviceConfig();
    
    // Ensure clean WiFi state
    WiFi.disconnect(true);
    delay(100);
    
    String apName = deviceConfig.deviceName + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    
    Serial.printf("WebServer: Starting AP '%s' with password '%s'...\n", 
                 apName.c_str(), deviceConfig.apPassword.c_str());
    
    WiFi.mode(WIFI_AP);
    delay(100);
    WiFi.softAP(apName.c_str(), deviceConfig.apPassword.c_str());
    
    // Wait for AP to initialize
    delay(100);
    int attempts = 0;
    while (WiFi.softAPIP().toString() == "0.0.0.0" && attempts < 10) {
        delay(100);
        attempts++;
    }
    
    wifiConnected = false;
    apMode = true;
    currentIP = WiFi.softAPIP().toString();
    
    Serial.printf("WebServer: AP started, IP: %s\n", currentIP.c_str());
}

bool WebServer::isWiFiConnected() const {
    return wifiConnected;
}

String WebServer::getIPAddress() const {
    return currentIP;
}

void WebServer::printStatus() const {
    Serial.println("=== WebServer Status ===");
    Serial.printf("Mode: %s\n", apMode ? "Access Point" : "WiFi Station");
    Serial.printf("IP Address: %s\n", currentIP.c_str());
    Serial.printf("Connected: %s\n", wifiConnected ? "Yes" : "No");
    
    if (apMode) {
        Serial.printf("AP Name: %s\n", WiFi.softAPSSID().c_str());
        Serial.printf("Connected clients: %d\n", WiFi.softAPgetStationNum());
    }
}

void WebServer::setupRoutes() {
    // Enable CORS for all routes
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Main page
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleRoot(request);
    });
    
    // API routes
    server.on("/api/zones", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetZones(request);
    });
    
    server.on("/api/brightness", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Response will be sent after body is processed
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleSetBrightnessBody(request, data, len, index, total);
    });
    
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetStatus(request);
    });
    
    // Zone management endpoints
    server.on("/api/zones", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Response will be sent after body is processed
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleAddZoneBody(request, data, len, index, total);
    });
    
    server.on("/api/zones", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        handleDeleteZone(request);
    });
    
    server.on("/api/zones/clear", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleClearZones(request);
    });
    
    // Effect control endpoints
    server.on("/api/effects", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetEffects(request);
    });
    
    server.on("/api/effects/trigger", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Response will be sent after body is processed
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleTriggerEffectBody(request, data, len, index, total);
    });
    
    // Audio control endpoints
    server.on("/api/audio/play", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Response will be sent after body is processed
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
        handlePlayAudioBody(request, data, len, index, total);
    });
    
    server.on("/api/audio/stop", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleStopAudio(request);
    });
    
    server.on("/api/audio/volume", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Response will be sent after body is processed
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleSetVolumeBody(request, data, len, index, total);
    });
    
    server.on("/api/audio/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetAudioStatus(request);
    });
    
    server.on("/api/audio/retry", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleRetryAudio(request);
    });
    
    // OTA firmware update endpoint
    server.on("/update", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleOTAUpload(request);
    }, [this](AsyncWebServerRequest* request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        handleOTAUploadFile(request, filename, index, data, len, final);
    });
    
    // Handle CORS preflight
    server.on("/api/brightness", HTTP_OPTIONS, [this](AsyncWebServerRequest* request) {
        sendCORSHeaders(request);
        request->send(200);
    });
    
    server.on("/api/zones", HTTP_OPTIONS, [this](AsyncWebServerRequest* request) {
        sendCORSHeaders(request);
        request->send(200);
    });
    
    server.on("/api/effects/trigger", HTTP_OPTIONS, [this](AsyncWebServerRequest* request) {
        sendCORSHeaders(request);
        request->send(200);
    });
    
    server.on("/api/audio/play", HTTP_OPTIONS, [this](AsyncWebServerRequest* request) {
        sendCORSHeaders(request);
        request->send(200);
    });
    
    server.on("/api/audio/volume", HTTP_OPTIONS, [this](AsyncWebServerRequest* request) {
        sendCORSHeaders(request);
        request->send(200);
    });
    
    // 404 handler
    server.onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "Not found");
    });
}

void WebServer::setupOTA() {
    const auto& deviceConfig = config.getDeviceConfig();
    
    ArduinoOTA.setPassword(deviceConfig.otaPassword.c_str());
    ArduinoOTA.setHostname(deviceConfig.deviceName.c_str());
    
    ArduinoOTA.onStart([]() {
        Serial.println("OTA: Update started");
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("OTA: Update completed");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA: Progress %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA: Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    
    ArduinoOTA.begin();
}

void WebServer::handleRoot(AsyncWebServerRequest* request) {
    request->send(200, "text/html", MAIN_HTML);
}

void WebServer::handleGetZones(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonArray zonesArray = doc["zones"].to<JsonArray>();
    
    auto zones = config.getAllZones();
    for (Zone* zone : zones) {
        if (!zone->enabled) continue;
        
        JsonObject zoneObj = zonesArray.add<JsonObject>();
        zoneObj["id"] = zone->id;
        zoneObj["name"] = zone->name;
        zoneObj["gpio"] = zone->gpio;
        zoneObj["type"] = (zone->type == ZoneType::PWM) ? "PWM" : "WS2812B";
        zoneObj["groupName"] = zone->groupName;
        zoneObj["brightness"] = zone->brightness;
        zoneObj["currentBrightness"] = ledController.getUserBrightness(zone->id);
    }
    
    String response;
    serializeJson(doc, response);
    sendJSONResponse(request, 200, response);
}

void WebServer::handleSetBrightness(AsyncWebServerRequest* request) {
    // This method is no longer used - body handler does the work
}

static String setBrightnessBody = "";

void WebServer::handleSetBrightnessBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Accumulate body data
    if (index == 0) {
        setBrightnessBody = "";
    }
    
    for (size_t i = 0; i < len; i++) {
        setBrightnessBody += (char)data[i];
    }
    
    // Process when complete
    if (index + len == total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, setBrightnessBody);
        
        if (error) {
            Serial.printf("WebServer: Brightness JSON error: %s\n", error.c_str());
            sendJSONResponse(request, 400, R"({"error":"Invalid JSON"})");
            return;
        }
        
        if (!doc["zoneId"] || !doc["brightness"]) {
            Serial.println("WebServer: Missing zoneId or brightness in request");
            sendJSONResponse(request, 400, R"({"error":"Missing zoneId or brightness"})");
            return;
        }
        
        uint8_t zoneId = doc["zoneId"];
        uint8_t brightness = doc["brightness"];
        
        Serial.printf("WebServer: Setting zone %d brightness to %d\n", zoneId, brightness);
        
        if (!ledController.isZoneConfigured(zoneId)) {
            Serial.printf("WebServer: Zone %d not configured\n", zoneId);
            sendJSONResponse(request, 404, R"({"error":"Zone not found"})");
            return;
        }
        
        ledController.setUserBrightness(zoneId, brightness);
        ledController.update();
        
        sendJSONResponse(request, 200, R"({"success":true})");
        
        Serial.printf("WebServer: Successfully set zone %d brightness to %d\n", zoneId, brightness);
    }
}

void WebServer::handleGetStatus(AsyncWebServerRequest* request) {
    const auto& deviceConfig = config.getDeviceConfig();
    
    JsonDocument doc;
    doc["deviceName"] = deviceConfig.deviceName;
    doc["firmwareVersion"] = deviceConfig.firmwareVersion;
    doc["ip"] = currentIP;
    doc["wifiMode"] = apMode ? "AP" : "STA";
    doc["uptime"] = millis();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["totalHeap"] = ESP.getHeapSize();
    
    String response;
    serializeJson(doc, response);
    sendJSONResponse(request, 200, response);
}

void WebServer::sendCORSHeaders(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    request->send(response);
}

void WebServer::sendJSONResponse(AsyncWebServerRequest* request, int code, const String& json) {
    AsyncWebServerResponse* response = request->beginResponse(code, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void WebServer::handleOTAUpload(AsyncWebServerRequest* request) {
    bool shouldReboot = !Update.hasError();
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", 
        shouldReboot ? R"({"success":true,"message":"Upload successful, rebooting..."})" 
                     : R"({"success":false,"message":"Upload failed"})");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    
    if (shouldReboot) {
        delay(100);
        ESP.restart();
    }
}

void WebServer::handleOTAUploadFile(AsyncWebServerRequest* request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
        Serial.printf("OTA: Starting update - %s\n", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
            return;
        }
    }
    
    if (len) {
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
            return;
        }
    }
    
    if (final) {
        if (Update.end(true)) {
            Serial.printf("OTA: Update complete - %uB\n", index + len);
        } else {
            Update.printError(Serial);
        }
    }
}

// Zone management handlers
void WebServer::handleAddZone(AsyncWebServerRequest* request) {
    // This method is no longer used - body handler does the work
}

static String addZoneBody = "";

void WebServer::handleAddZoneBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Accumulate body data
    if (index == 0) {
        addZoneBody = "";
    }
    
    for (size_t i = 0; i < len; i++) {
        addZoneBody += (char)data[i];
    }
    
    // Process when complete
    if (index + len == total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, addZoneBody);
        
        if (error) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Invalid JSON"})");
            return;
        }
        
        // Validate required fields
        if (!doc["name"] || !doc["gpio"] || !doc["type"]) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Missing required fields: name, gpio, type"})");
            return;
        }
        
        // Parse zone data
        String name = doc["name"];
        uint8_t gpio = doc["gpio"];
        String typeStr = doc["type"];
        uint8_t ledCount = doc["ledCount"] | 1;
        String groupName = doc["groupName"] | "Default";
        uint8_t brightness = doc["brightness"] | 255;
        
        // Validate GPIO
        if (!config.isValidGPIO(gpio)) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Invalid GPIO pin"})");
            return;
        }
        
        if (config.isGPIOInUse(gpio)) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"GPIO pin already in use"})");
            return;
        }
        
        // Parse zone type
        ZoneType zoneType;
        if (typeStr == "PWM") {
            zoneType = ZoneType::PWM;
            ledCount = 1; // PWM zones always have 1 LED
        } else if (typeStr == "WS2812B") {
            zoneType = ZoneType::WS2812B;
            if (ledCount < 1 || ledCount > 100) {
                sendJSONResponse(request, 400, R"({"success":false,"error":"LED count must be 1-100 for RGB zones"})");
                return;
            }
        } else {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Invalid zone type. Use PWM or WS2812B"})");
            return;
        }
        
        // Create zone
        uint8_t zoneId = config.getNextZoneId();
        Zone zone(zoneId, name, gpio, zoneType, ledCount, groupName, brightness);
        
        if (config.addZone(zone)) {
            // Add zone to LED controller
            ledController.addZone(zone);
            
            // Save configuration
            config.save();
            
            Serial.printf("WebServer: Added zone %d '%s' on GPIO %d\n", zoneId, name.c_str(), gpio);
            
            JsonDocument responseDoc;
            responseDoc["success"] = true;
            responseDoc["zoneId"] = zoneId;
            responseDoc["message"] = "Zone added successfully";
            
            String response;
            serializeJson(responseDoc, response);
            sendJSONResponse(request, 201, response);
        } else {
            sendJSONResponse(request, 500, R"({"success":false,"error":"Failed to add zone"})");
        }
    }
}

void WebServer::handleDeleteZone(AsyncWebServerRequest* request) {
    if (!request->hasParam("zoneId")) {
        sendJSONResponse(request, 400, R"({"success":false,"error":"Missing zoneId parameter"})");
        return;
    }
    
    uint8_t zoneId = request->getParam("zoneId")->value().toInt();
    
    if (config.removeZone(zoneId)) {
        // Remove from LED controller
        ledController.removeZone(zoneId);
        
        // Save configuration
        config.save();
        
        Serial.printf("WebServer: Removed zone %d\n", zoneId);
        sendJSONResponse(request, 200, R"({"success":true,"message":"Zone removed successfully"})");
    } else {
        sendJSONResponse(request, 404, R"({"success":false,"error":"Zone not found"})");
    }
}

void WebServer::handleClearZones(AsyncWebServerRequest* request) {
    // Get all zones and remove them
    auto zones = config.getAllZones();
    for (Zone* zone : zones) {
        ledController.removeZone(zone->id);
        config.removeZone(zone->id);
    }
    
    // Save configuration
    config.save();
    
    Serial.printf("WebServer: Cleared all zones (%d removed)\n", zones.size());
    
    JsonDocument responseDoc;
    responseDoc["success"] = true;
    responseDoc["message"] = String("Cleared ") + String(zones.size()) + String(" zones");
    
    String response;
    serializeJson(responseDoc, response);
    sendJSONResponse(request, 200, response);
}

// Effect management handlers
void WebServer::handleGetEffects(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonArray effectsArray = doc["effects"].to<JsonArray>();
    
    auto effectNames = effectManager.getEffectNames();
    for (const String& name : effectNames) {
        JsonObject effectObj = effectsArray.add<JsonObject>();
        effectObj["name"] = name;
        effectObj["enabled"] = effectManager.isEffectEnabled(name);
    }
    
    String response;
    serializeJson(doc, response);
    sendJSONResponse(request, 200, response);
}

void WebServer::handleTriggerEffect(AsyncWebServerRequest* request) {
    // This method is no longer used - body handler does the work
}

static String triggerEffectBody = "";

void WebServer::handleTriggerEffectBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Accumulate body data
    if (index == 0) {
        triggerEffectBody = "";
    }
    
    for (size_t i = 0; i < len; i++) {
        triggerEffectBody += (char)data[i];
    }
    
    // Process when complete
    if (index + len == total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, triggerEffectBody);
        
        if (error) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Invalid JSON"})");
            return;
        }
        
        if (!doc["effectName"]) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Missing effectName"})");
            return;
        }
        
        String effectName = doc["effectName"];
        uint32_t duration = doc["duration"] | 0; // Default to continuous
        
        if (effectManager.triggerEffect(effectName, duration)) {
            Serial.printf("WebServer: Triggered effect '%s' for %dms\n", effectName.c_str(), duration);
            
            JsonDocument responseDoc;
            responseDoc["success"] = true;
            responseDoc["message"] = String("Triggered effect: ") + effectName;
            
            String response;
            serializeJson(responseDoc, response);
            sendJSONResponse(request, 200, response);
        } else {
            sendJSONResponse(request, 404, R"({"success":false,"error":"Effect not found"})");
        }
    }
}

// Audio handler methods
void WebServer::handlePlayAudio(AsyncWebServerRequest* request) {
    // This is handled by the body handler
    request->send(400, "application/json", R"({"success":false,"error":"Missing request body"})");
}

void WebServer::handlePlayAudioBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Only process when we have the complete body
    if (index + len == total) {
        String body = String((char*)data, len);
        JsonDocument doc;
        
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Invalid JSON"})");
            return;
        }
        
        if (!doc["trackNumber"]) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Missing trackNumber"})");
            return;
        }
        
        uint16_t trackNumber = doc["trackNumber"];
        bool loop = doc["loop"] | false;
        
        if (audioController.isAvailable()) {
            if (audioController.play(trackNumber, loop)) {
                Serial.printf("WebServer: Playing audio track %d (loop: %s)\n", trackNumber, loop ? "yes" : "no");
                
                JsonDocument responseDoc;
                responseDoc["success"] = true;
                responseDoc["message"] = String("Playing track ") + trackNumber;
                responseDoc["track"] = trackNumber;
                responseDoc["loop"] = loop;
                
                String response;
                serializeJson(responseDoc, response);
                sendJSONResponse(request, 200, response);
            } else {
                sendJSONResponse(request, 400, R"({"success":false,"error":"Failed to play audio track"})");
            }
        } else {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Audio hardware not available"})");
        }
    }
}

void WebServer::handleStopAudio(AsyncWebServerRequest* request) {
    if (audioController.isAvailable()) {
        if (audioController.stop()) {
            Serial.println("WebServer: Stopped audio playback");
            sendJSONResponse(request, 200, R"({"success":true,"message":"Audio stopped"})");
        } else {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Failed to stop audio"})");
        }
    } else {
        sendJSONResponse(request, 400, R"({"success":false,"error":"Audio hardware not available"})");
    }
}

void WebServer::handleSetVolume(AsyncWebServerRequest* request) {
    // This is handled by the body handler
    request->send(400, "application/json", R"({"success":false,"error":"Missing request body"})");
}

void WebServer::handleSetVolumeBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Only process when we have the complete body
    if (index + len == total) {
        String body = String((char*)data, len);
        JsonDocument doc;
        
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Invalid JSON"})");
            return;
        }
        
        if (!doc["volume"].is<uint8_t>()) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Missing volume"})");
            return;
        }
        
        uint8_t volume = doc["volume"];
        if (volume > 30) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Volume must be 0-30"})");
            return;
        }
        
        if (audioController.isAvailable()) {
            if (audioController.setVolume(volume)) {
                Serial.printf("WebServer: Set audio volume to %d\n", volume);
                
                JsonDocument responseDoc;
                responseDoc["success"] = true;
                responseDoc["message"] = String("Volume set to ") + volume;
                responseDoc["volume"] = volume;
                
                String response;
                serializeJson(responseDoc, response);
                sendJSONResponse(request, 200, response);
            } else {
                sendJSONResponse(request, 400, R"({"success":false,"error":"Failed to set volume"})");
            }
        } else {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Audio hardware not available"})");
        }
    }
}

void WebServer::handleGetAudioStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    
    doc["success"] = true;
    doc["available"] = audioController.isAvailable();
    doc["playing"] = audioController.isPlaying();
    doc["currentTrack"] = audioController.getCurrentTrack();
    doc["volume"] = audioController.getVolume();
    
    // Status string for UI
    switch (audioController.getStatus()) {
        case AudioStatus::STOPPED:
            doc["status"] = "stopped";
            break;
        case AudioStatus::PLAYING:
            doc["status"] = "playing";
            break;
        case AudioStatus::PAUSED:
            doc["status"] = "paused";
            break;
        case AudioStatus::ERROR:
            doc["status"] = "error";
            break;
        default:
            doc["status"] = "unknown";
            break;
    }
    
    String response;
    serializeJson(doc, response);
    sendJSONResponse(request, 200, response);
}

void WebServer::handleRetryAudio(AsyncWebServerRequest* request) {
    Serial.println("WebServer: Manual audio retry requested");
    
    if (audioController.retryInitialization()) {
        Serial.println("WebServer: Audio retry successful");
        sendJSONResponse(request, 200, R"({"success":true,"message":"Audio hardware initialized successfully"})");
    } else {
        Serial.println("WebServer: Audio retry failed");
        sendJSONResponse(request, 400, R"({"success":false,"error":"Audio hardware initialization failed - check connections"})");
    }
}

} // namespace BattleAura