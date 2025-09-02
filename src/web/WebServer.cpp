#include "WebServer.h"
#include "WebInterface.h"
#include <ArduinoJson.h>

namespace BattleAura {

WebServer::WebServer(Configuration& config, LedController& ledController) 
    : config(config), ledController(ledController), server(80), 
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
    
    String apName = deviceConfig.deviceName + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    
    Serial.printf("WebServer: Starting AP '%s'...\n", apName.c_str());
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), deviceConfig.apPassword.c_str());
    
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
        handleSetBrightness(request);
    });
    
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetStatus(request);
    });
    
    // Handle CORS preflight
    server.on("/api/brightness", HTTP_OPTIONS, [this](AsyncWebServerRequest* request) {
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
        zoneObj["currentBrightness"] = ledController.getZoneBrightness(zone->id);
    }
    
    String response;
    serializeJson(doc, response);
    sendJSONResponse(request, 200, response);
}

void WebServer::handleSetBrightness(AsyncWebServerRequest* request) {
    // Handle JSON body
    if (request->hasParam("body", true)) {
        String body = request->getParam("body", true)->value();
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            sendJSONResponse(request, 400, R"({"error":"Invalid JSON"})");
            return;
        }
        
        if (!doc["zoneId"].is<uint8_t>() || !doc["brightness"].is<uint8_t>()) {
            sendJSONResponse(request, 400, R"({"error":"Missing zoneId or brightness"})");
            return;
        }
        
        uint8_t zoneId = doc["zoneId"];
        uint8_t brightness = doc["brightness"];
        
        if (!ledController.isZoneConfigured(zoneId)) {
            sendJSONResponse(request, 404, R"({"error":"Zone not found"})");
            return;
        }
        
        ledController.setZoneBrightness(zoneId, brightness);
        ledController.update();
        
        sendJSONResponse(request, 200, R"({"success":true})");
        
        Serial.printf("WebServer: Set zone %d brightness to %d\n", zoneId, brightness);
    } else {
        sendJSONResponse(request, 400, R"({"error":"No body provided"})");
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

} // namespace BattleAura