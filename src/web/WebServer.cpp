#include "WebServer.h"
#include "WebInterface.h"
#include <ArduinoJson.h>

namespace BattleAura {

WebServer::WebServer(Configuration& config, LedController& ledController, VFXManager& vfxManager, AudioController& audioController) 
    : config(config), ledController(ledController), vfxManager(vfxManager), audioController(audioController), server(80), 
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
        
        // Setup mDNS only in Station mode (connected to WiFi)
        // mDNS doesn't work in AP mode
        delay(500); // Small delay to ensure WiFi is fully established
        setupmDNS();
    } else {
        Serial.println("WebServer: WiFi failed, starting AP mode");
        startAccessPoint();
        Serial.println("WebServer: mDNS not available in AP mode");
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
    
    // Set hostname BEFORE WiFi connection for proper DHCP registration
    String hostname = generateHostname(deviceConfig.deviceName);
    Serial.printf("WebServer: Setting WiFi hostname to '%s'\n", hostname.c_str());
    WiFi.setHostname(hostname.c_str());
    
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
    
    // VFX control endpoints
    server.on("/api/vfx", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetVFX(request);
    });
    
    server.on("/api/vfx/trigger", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Response will be sent after body is processed
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleTriggerVFXBody(request, data, len, index, total);
    });
    
    server.on("/api/vfx/stop-all", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleStopAllVFX(request);
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
    
    // Audio tracks management endpoints
    server.on("/api/audio/tracks", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetAudioTracks(request);
    });
    
    server.on("/api/audio/tracks", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Response will be sent after body is processed
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleAddAudioTrackBody(request, data, len, index, total);
    });
    
    server.on("/api/audio/tracks", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        handleDeleteAudioTrack(request);
    });
    
    // WiFi configuration endpoints
    server.on("/api/wifi/config", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Response will be sent after body is processed
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleWiFiConfigBody(request, data, len, index, total);
    });
    
    server.on("/api/wifi/clear", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleClearWiFi(request);
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
    
    server.on("/api/vfx/trigger", HTTP_OPTIONS, [this](AsyncWebServerRequest* request) {
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
    
    // VFX configuration
    server.on("/api/scenes/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetSceneConfigs(request);
    });
    
    server.on("/api/scenes/config", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Will be handled by body handler
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleAddSceneConfigBody(request, data, len, index, total);
    });
    
    server.on("/api/scenes/config", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        // Will be handled by body handler
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleDeleteSceneConfigBody(request, data, len, index, total);
    });
    
    // Device configuration
    server.on("/api/device/config", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Will be handled by body handler
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleDeviceConfigBody(request, data, len, index, total);
    });
    
    // System management
    server.on("/api/system/restart", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleSystemRestart(request);
    });
    
    server.on("/api/system/factory-reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleFactoryReset(request);
    });
    
    // Global brightness settings
    server.on("/api/global/brightness", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetGlobalBrightness(request);
    });
    
    server.on("/api/global/brightness", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Will be handled by body handler
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleSetGlobalBrightnessBody(request, data, len, index, total);
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

void WebServer::setupmDNS() {
    const auto& deviceConfig = config.getDeviceConfig();
    
    String hostname = generateHostname(deviceConfig.deviceName);
    
    Serial.printf("WebServer: Initializing mDNS for '%s.local'\n", hostname.c_str());
    
    // Initialize mDNS (hostname already set during WiFi connection)
    if (MDNS.begin(hostname.c_str())) {
        Serial.printf("WebServer: mDNS responder started at %s.local\n", hostname.c_str());
        
        // Add HTTP web service
        MDNS.addService("http", "tcp", 80);
        MDNS.addServiceTxt("http", "tcp", "device", "BattleAura");
        MDNS.addServiceTxt("http", "tcp", "version", deviceConfig.firmwareVersion.c_str());
        MDNS.addServiceTxt("http", "tcp", "model", "ESP32-S3");
        MDNS.addServiceTxt("http", "tcp", "path", "/");
        
        // Add BattleAura-specific service for discovery
        MDNS.addService("battleaura", "tcp", 80);
        MDNS.addServiceTxt("battleaura", "tcp", "version", deviceConfig.firmwareVersion.c_str());
        MDNS.addServiceTxt("battleaura", "tcp", "name", deviceConfig.deviceName.c_str());
        
        Serial.printf("WebServer: mDNS services registered for discovery\n");
    } else {
        Serial.println("WebServer: Failed to start mDNS responder - check WiFi connection");
    }
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
    doc["hostname"] = generateHostname(deviceConfig.deviceName);
    doc["firmwareVersion"] = deviceConfig.firmwareVersion;
    doc["ip"] = currentIP;
    doc["wifiMode"] = apMode ? "AP" : "STA";
    doc["wifiConnected"] = wifiConnected;
    doc["wifiSSID"] = wifiConnected ? deviceConfig.wifiSSID : "";
    doc["deviceId"] = WiFi.macAddress().substring(12);  // Last 6 chars of MAC for AP SSID
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

// VFX management handlers
void WebServer::handleGetVFX(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonArray vfxArray = doc["vfx"].to<JsonArray>();
    
    auto vfxNames = vfxManager.getVFXNames();
    for (const String& name : vfxNames) {
        JsonObject vfxObj = vfxArray.add<JsonObject>();
        vfxObj["name"] = name;
        vfxObj["enabled"] = vfxManager.isVFXEnabled(name);
    }
    
    String response;
    serializeJson(doc, response);
    sendJSONResponse(request, 200, response);
}

void WebServer::handleTriggerVFX(AsyncWebServerRequest* request) {
    // This method is no longer used - body handler does the work
}

static String triggerVFXBody = "";

void WebServer::handleTriggerVFXBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Accumulate body data
    if (index == 0) {
        triggerVFXBody = "";
    }
    
    for (size_t i = 0; i < len; i++) {
        triggerVFXBody += (char)data[i];
    }
    
    // Process when complete
    if (index + len == total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, triggerVFXBody);
        
        if (error) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Invalid JSON"})");
            return;
        }
        
        if (!doc["vfxName"]) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Missing vfxName"})");
            return;
        }
        
        String vfxName = doc["vfxName"];
        uint32_t duration = doc["duration"] | 0; // Default to continuous
        
        if (vfxManager.triggerVFX(vfxName, duration)) {
            Serial.printf("WebServer: Triggered VFX '%s' for %dms\n", vfxName.c_str(), duration);
            
            JsonDocument responseDoc;
            responseDoc["success"] = true;
            responseDoc["message"] = String("Triggered VFX: ") + vfxName;
            
            String response;
            serializeJson(responseDoc, response);
            sendJSONResponse(request, 200, response);
        } else {
            sendJSONResponse(request, 404, R"({"success":false,"error":"VFX not found"})");
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
    parseJSONBody(request, data, len, index, total, &WebServer::processSetVolume);
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

void WebServer::handleWiFiConfig(AsyncWebServerRequest* request) {
    // This is handled by the body handler
    request->send(400, "application/json", R"({"success":false,"error":"Missing request body"})");
}

void WebServer::handleWiFiConfigBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
    parseJSONBody(request, data, len, index, total, &WebServer::processWiFiConfig);
}

void WebServer::handleClearWiFi(AsyncWebServerRequest* request) {
    Serial.println("WebServer: Clearing WiFi configuration");
    
    // Clear WiFi configuration
    auto& deviceConfig = config.getDeviceConfig();
    deviceConfig.wifiSSID = "";
    deviceConfig.wifiPassword = "";
    
    // Save configuration
    if (!config.save()) {
        sendJSONResponse(request, 500, R"({"success":false,"error":"Failed to save configuration"})");
        return;
    }
    
    Serial.println("WebServer: WiFi configuration cleared");
    sendJSONResponse(request, 200, R"({"success":true,"message":"WiFi configuration cleared. Device will remain in AP mode until reboot."})");
}

String WebServer::generateHostname(const String& deviceName) {
    String hostname = deviceName;
    hostname.toLowerCase();
    hostname.replace(" ", "");
    hostname.replace("_", "");
    hostname.replace("-", "");
    // Remove any non-alphanumeric characters for hostname compliance
    String cleanHostname = "";
    for (int i = 0; i < hostname.length(); i++) {
        char c = hostname.charAt(i);
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            cleanHostname += c;
        }
    }
    if (cleanHostname.length() == 0) {
        cleanHostname = "battleaura";
    }
    return cleanHostname;
}

void WebServer::parseJSONBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total, void (WebServer::*handler)(AsyncWebServerRequest*, JsonDocument&)) {
    // Only process when we have the complete body
    if (index + len == total) {
        String body = String((char*)data, len);
        JsonDocument doc;
        
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Invalid JSON"})");
            return;
        }
        
        // Call the specific handler with parsed JSON
        (this->*handler)(request, doc);
    }
}

void WebServer::processWiFiConfig(AsyncWebServerRequest* request, JsonDocument& doc) {
    if (!doc["ssid"].is<String>()) {
        sendJSONResponse(request, 400, R"({"success":false,"error":"Missing SSID"})");
        return;
    }
    
    String ssid = doc["ssid"];
    String password = doc["password"] | "";
    String deviceName = doc["deviceName"] | "";
    
    if (ssid.length() == 0 || ssid.length() > 32) {
        sendJSONResponse(request, 400, R"({"success":false,"error":"SSID must be 1-32 characters"})");
        return;
    }
    
    if (password.length() > 64) {
        sendJSONResponse(request, 400, R"({"success":false,"error":"Password must be 64 characters or less"})");
        return;
    }
    
    // Update configuration
    auto& deviceConfig = config.getDeviceConfig();
    deviceConfig.wifiSSID = ssid;
    deviceConfig.wifiPassword = password;
    
    // Update device name if provided
    if (deviceName.length() > 0) {
        if (deviceName.length() > 32) {
            sendJSONResponse(request, 400, R"({"success":false,"error":"Device name must be 32 characters or less"})");
            return;
        }
        deviceConfig.deviceName = deviceName;
        
        // Update hostname and mDNS with new device name (only works in Station mode)
        if (wifiConnected && !apMode) {
            String cleanHostname = generateHostname(deviceName);
            
            WiFi.setHostname(cleanHostname.c_str());
            MDNS.end();
            delay(100); // Small delay before restarting mDNS
            
            if (MDNS.begin(cleanHostname.c_str())) {
                Serial.printf("WebServer: mDNS updated to %s.local\n", cleanHostname.c_str());
                
                // Re-register services with new hostname
                MDNS.addService("http", "tcp", 80);
                MDNS.addServiceTxt("http", "tcp", "device", "BattleAura");
                MDNS.addServiceTxt("http", "tcp", "version", config.getDeviceConfig().firmwareVersion.c_str());
                MDNS.addServiceTxt("http", "tcp", "model", "ESP32-S3");
                MDNS.addServiceTxt("http", "tcp", "path", "/");
                
                MDNS.addService("battleaura", "tcp", 80);
                MDNS.addServiceTxt("battleaura", "tcp", "version", config.getDeviceConfig().firmwareVersion.c_str());
                MDNS.addServiceTxt("battleaura", "tcp", "name", deviceName.c_str());
            } else {
                Serial.println("WebServer: Failed to restart mDNS with new hostname");
            }
        } else {
            Serial.println("WebServer: mDNS hostname update skipped (not in Station mode)");
        }
    }
    
    // Save configuration
    if (!config.save()) {
        sendJSONResponse(request, 500, R"({"success":false,"error":"Failed to save configuration"})");
        return;
    }
    
    Serial.printf("WebServer: Configuration updated - Device: %s, SSID: %s\n", 
                 deviceConfig.deviceName.c_str(), ssid.c_str());
    
    // Send success response first
    JsonDocument responseDoc;
    responseDoc["success"] = true;
    responseDoc["message"] = deviceName.length() > 0 ? 
        "Device name and WiFi configuration saved. Attempting to connect..." :
        "WiFi configuration saved. Attempting to connect...";
    
    String response;
    serializeJson(responseDoc, response);
    sendJSONResponse(request, 200, response);
    
    // Attempt WiFi connection after a short delay
    Serial.println("WebServer: Attempting WiFi reconnection in 2 seconds...");
}

void WebServer::processSetVolume(AsyncWebServerRequest* request, JsonDocument& doc) {
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

void WebServer::handleGetAudioTracks(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonArray tracksArray = doc["tracks"].to<JsonArray>();
    
    auto tracks = config.getAllAudioTracks();
    for (AudioTrack* track : tracks) {
        JsonObject trackObj = tracksArray.add<JsonObject>();
        trackObj["fileNumber"] = track->fileNumber;
        trackObj["description"] = track->description;
        trackObj["isLoop"] = track->isLoop;
        trackObj["duration"] = track->duration;
    }
    
    String response;
    serializeJson(doc, response);
    sendJSONResponse(request, 200, response);
}

void WebServer::handleAddAudioTrack(AsyncWebServerRequest* request) {
    // This method is no longer used - body handler does the work
}

static String addAudioTrackBody = "";

void WebServer::handleAddAudioTrackBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
    parseJSONBody(request, data, len, index, total, &WebServer::processAddAudioTrack);
}

void WebServer::processAddAudioTrack(AsyncWebServerRequest* request, JsonDocument& doc) {
    if (!doc["fileNumber"] || !doc["description"]) {
        sendJSONResponse(request, 400, R"({"success":false,"error":"Missing required fields: fileNumber, description"})");
        return;
    }
    
    AudioTrack track;
    track.fileNumber = doc["fileNumber"];
    track.description = doc["description"].as<String>();
    track.isLoop = doc["isLoop"] | false;
    track.duration = doc["duration"] | 0;
    
    if (config.addAudioTrack(track)) {
        if (config.save()) {
            Serial.printf("WebServer: Added audio track %d: %s\n", track.fileNumber, track.description.c_str());
            
            JsonDocument responseDoc;
            responseDoc["success"] = true;
            responseDoc["message"] = "Audio track added successfully";
            
            String response;
            serializeJson(responseDoc, response);
            sendJSONResponse(request, 200, response);
        } else {
            sendJSONResponse(request, 500, R"({"success":false,"error":"Failed to save configuration"})");
        }
    } else {
        sendJSONResponse(request, 400, R"({"success":false,"error":"Failed to add audio track"})");
    }
}

void WebServer::handleDeleteAudioTrack(AsyncWebServerRequest* request) {
    if (!request->hasParam("fileNumber")) {
        sendJSONResponse(request, 400, R"({"success":false,"error":"Missing fileNumber parameter"})");
        return;
    }
    
    uint16_t fileNumber = request->getParam("fileNumber")->value().toInt();
    if (fileNumber == 0) {
        sendJSONResponse(request, 400, R"({"success":false,"error":"Invalid file number"})");
        return;
    }
    
    if (config.removeAudioTrack(fileNumber)) {
        if (config.save()) {
            Serial.printf("WebServer: Removed audio track %d\n", fileNumber);
            
            JsonDocument responseDoc;
            responseDoc["success"] = true;
            responseDoc["message"] = "Audio track removed successfully";
            
            String response;
            serializeJson(responseDoc, response);
            sendJSONResponse(request, 200, response);
        } else {
            sendJSONResponse(request, 500, R"({"success":false,"error":"Failed to save configuration"})");
        }
    } else {
        sendJSONResponse(request, 404, R"({"success":false,"error":"Audio track not found"})");
    }
}

void WebServer::handleGetSceneConfigs(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonArray configsArray = doc["configs"].to<JsonArray>();
    
    auto sceneConfigs = config.getAllSceneConfigs();
    for (const SceneConfig* sceneConfig : sceneConfigs) {
        if (sceneConfig) {
            JsonObject configObj = configsArray.add<JsonObject>();
            configObj["name"] = sceneConfig->name;
            configObj["audioFile"] = sceneConfig->audioFile;
            configObj["duration"] = sceneConfig->duration;
            configObj["audioTimeout"] = sceneConfig->audioTimeout;
            configObj["enabled"] = sceneConfig->enabled;
            
            // Convert type enum to string
            switch (sceneConfig->type) {
                case SceneType::AMBIENT:
                    configObj["type"] = "AMBIENT";
                    break;
                case SceneType::ACTIVE:
                    configObj["type"] = "ACTIVE";
                    break;
                case SceneType::GLOBAL:
                    configObj["type"] = "GLOBAL";
                    break;
                default:
                    configObj["type"] = "AMBIENT";
            }
            
            JsonArray groupsArray = configObj["targetGroups"].to<JsonArray>();
            for (const String& group : sceneConfig->targetGroups) {
                groupsArray.add(group);
            }
        }
    }
    
    String response;
    serializeJson(doc, response);
    sendJSONResponse(request, 200, response);
}

void WebServer::handleAddSceneConfigBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
    parseJSONBody(request, data, len, index, total, &WebServer::processAddSceneConfig);
}

void WebServer::processAddSceneConfig(AsyncWebServerRequest* request, JsonDocument& doc) {
    String sceneName = doc["name"] | "";
    String sceneType = doc["type"] | "AMBIENT";
    JsonArray groupsArray = doc["groups"];
    uint16_t audioFile = doc["audioFile"] | 0;
    uint32_t duration = doc["duration"] | 0;
    uint32_t audioTimeout = doc["audioTimeout"] | 0;
    bool enabled = doc["enabled"] | true;
    
    if (sceneName.isEmpty()) {
        sendJSONResponse(request, 400, R"({"success":false,"error":"VFX name is required"})");
        return;
    }
    
    SceneConfig sceneConfig;
    sceneConfig.name = sceneName;
    sceneConfig.audioFile = audioFile;
    sceneConfig.duration = duration;
    sceneConfig.audioTimeout = audioTimeout;
    sceneConfig.enabled = enabled;
    
    // Set scene type
    if (sceneType == "AMBIENT") {
        sceneConfig.type = SceneType::AMBIENT;
    } else if (sceneType == "ACTIVE") {
        sceneConfig.type = SceneType::ACTIVE;
    } else if (sceneType == "GLOBAL") {
        sceneConfig.type = SceneType::GLOBAL;
    } else {
        sceneConfig.type = SceneType::AMBIENT; // Default
    }
    
    // Add target groups
    for (JsonVariant group : groupsArray) {
        String groupName = group.as<String>();
        if (!groupName.isEmpty()) {
            sceneConfig.addTargetGroup(groupName);
        }
    }
    
    if (config.addSceneConfig(sceneConfig)) {
        if (config.save()) {
            Serial.printf("WebServer: Added scene config '%s' with %d groups\n", 
                         sceneName.c_str(), sceneConfig.targetGroups.size());
            
            JsonDocument responseDoc;
            responseDoc["success"] = true;
            responseDoc["message"] = "Scene configuration saved successfully";
            
            String response;
            serializeJson(responseDoc, response);
            sendJSONResponse(request, 200, response);
        } else {
            sendJSONResponse(request, 500, R"({"success":false,"error":"Failed to save configuration"})");
        }
    } else {
        sendJSONResponse(request, 500, R"({"success":false,"error":"Failed to add scene configuration"})");
    }
}

void WebServer::handleDeleteSceneConfigBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
    parseJSONBody(request, data, len, index, total, &WebServer::processDeleteSceneConfig);
}

void WebServer::processDeleteSceneConfig(AsyncWebServerRequest* request, JsonDocument& doc) {
    String sceneName = doc["name"] | "";
    
    if (sceneName.isEmpty()) {
        sendJSONResponse(request, 400, R"({"success":false,"error":"VFX name is required"})");
        return;
    }
    
    if (config.removeSceneConfig(sceneName)) {
        if (config.save()) {
            Serial.printf("WebServer: Removed scene config '%s'\n", sceneName.c_str());
            
            JsonDocument responseDoc;
            responseDoc["success"] = true;
            responseDoc["message"] = "Scene configuration removed successfully";
            
            String response;
            serializeJson(responseDoc, response);
            sendJSONResponse(request, 200, response);
        } else {
            sendJSONResponse(request, 500, R"({"success":false,"error":"Failed to save configuration"})");
        }
    } else {
        sendJSONResponse(request, 404, R"({"success":false,"error":"Scene configuration not found"})");
    }
}

void WebServer::handleDeviceConfigBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
    parseJSONBody(request, data, len, index, total, &WebServer::processDeviceConfig);
}

void WebServer::processDeviceConfig(AsyncWebServerRequest* request, JsonDocument& doc) {
    String deviceName = doc["deviceName"] | "";
    bool audioEnabled = doc["audioEnabled"] | true;
    
    if (!deviceName.isEmpty()) {
        config.getDeviceConfig().deviceName = deviceName;
    }
    config.getDeviceConfig().audioEnabled = audioEnabled;
    
    if (config.save()) {
        Serial.printf("WebServer: Updated device config - Name: %s, Audio: %s\n", 
                     deviceName.c_str(), audioEnabled ? "enabled" : "disabled");
        
        JsonDocument responseDoc;
        responseDoc["success"] = true;
        responseDoc["message"] = "Device configuration saved successfully";
        
        String response;
        serializeJson(responseDoc, response);
        sendJSONResponse(request, 200, response);
    } else {
        sendJSONResponse(request, 500, R"({"success":false,"error":"Failed to save device configuration"})");
    }
}

void WebServer::handleSystemRestart(AsyncWebServerRequest* request) {
    JsonDocument responseDoc;
    responseDoc["success"] = true;
    responseDoc["message"] = "Device is restarting...";
    
    String response;
    serializeJson(responseDoc, response);
    sendJSONResponse(request, 200, response);
    
    Serial.println("WebServer: System restart requested");
    
    // Delay restart to allow response to be sent
    delay(1000);
    ESP.restart();
}

void WebServer::handleFactoryReset(AsyncWebServerRequest* request) {
    if (config.factoryReset()) {
        Serial.println("WebServer: Factory reset completed");
        
        JsonDocument responseDoc;
        responseDoc["success"] = true;
        responseDoc["message"] = "Factory reset completed. Device is restarting...";
        
        String response;
        serializeJson(responseDoc, response);
        sendJSONResponse(request, 200, response);
        
        // Delay restart to allow response to be sent
        delay(1000);
        ESP.restart();
    } else {
        sendJSONResponse(request, 500, R"({"success":false,"error":"Failed to perform factory reset"})");
    }
}

void WebServer::handleStopAllVFX(AsyncWebServerRequest* request) {
    Serial.println("WebServer: Stopping all VFX");
    
    // Stop all VFX via the VFX manager
    vfxManager.stopAllVFX();
    
    JsonDocument responseDoc;
    responseDoc["success"] = true;
    responseDoc["message"] = "All VFX stopped";
    
    String response;
    serializeJson(responseDoc, response);
    sendJSONResponse(request, 200, response);
}

void WebServer::handleGetGlobalBrightness(AsyncWebServerRequest* request) {
    JsonDocument responseDoc;
    responseDoc["brightness"] = config.getDeviceConfig().globalBrightness;
    
    String response;
    serializeJson(responseDoc, response);
    sendJSONResponse(request, 200, response);
}

void WebServer::handleSetGlobalBrightnessBody(AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
    parseJSONBody(request, data, len, index, total, &WebServer::processSetGlobalBrightness);
}

void WebServer::processSetGlobalBrightness(AsyncWebServerRequest* request, JsonDocument& doc) {
    uint8_t brightness = doc["brightness"] | 255;
    
    Serial.printf("WebServer: Setting global brightness to %d\n", brightness);
    
    // Store global brightness in configuration
    auto& deviceConfig = config.getDeviceConfig();
    deviceConfig.globalBrightness = brightness;
    config.save();
    
    // Apply global brightness to all zones
    auto zones = config.getAllZones();
    for (Zone* zone : zones) {
        if (zone) {
            // Calculate proportional brightness based on zone's max brightness
            uint8_t zoneBrightness = (brightness * zone->brightness) / 255;
            ledController.setZoneBrightness(zone->id, zoneBrightness);
        }
    }
    
    JsonDocument responseDoc;
    responseDoc["success"] = true;
    responseDoc["message"] = "Global brightness applied to all zones";
    responseDoc["brightness"] = brightness;
    
    String response;
    serializeJson(responseDoc, response);
    sendJSONResponse(request, 200, response);
}

} // namespace BattleAura