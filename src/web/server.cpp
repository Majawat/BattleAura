#include "server.h"

WebServerManager& WebServerManager::getInstance() {
    static WebServerManager instance;
    return instance;
}

bool WebServerManager::initialize(uint16_t port) {
    if (running) return true;
    
    server = new WebServer(port);
    if (!server) {
        Serial.println("Failed to create WebServer instance");
        return false;
    }
    
    setupRoutes();
    server->begin();
    running = true;
    
    Serial.printf("WebServer started on port %d\n", port);
    return true;
}

void WebServerManager::handleClient() {
    if (server && running) {
        server->handleClient();
    }
}

bool WebServerManager::startWiFiAP(const String& ssid, const String& password) {
    WiFi.mode(WIFI_AP);
    bool success = WiFi.softAP(ssid.c_str(), password.c_str());
    
    if (success) {
        Serial.printf("WiFi AP started: %s\n", ssid.c_str());
        Serial.printf("Password: %s\n", password.c_str());
    }
    
    return success;
}

bool WebServerManager::connectWiFiStation(const String& ssid, const String& password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        return true;
    } else {
        Serial.println(" Failed");
        return false;
    }
}

String WebServerManager::getIPAddress() const {
    if (WiFi.getMode() == WIFI_AP) {
        return WiFi.softAPIP().toString();
    } else {
        return WiFi.localIP().toString();
    }
}

void WebServerManager::setupRoutes() {
    setupStaticFileHandlers();
    setupAPIHandlers();
    
    server->onNotFound([this]() { this->handleNotFound(); });
}

void WebServerManager::setupStaticFileHandlers() {
    // Root handler
    server->on("/", [this]() { this->handleRoot(); });
    
    // Static file handlers  
    server->on("/style.css", [this]() { 
        this->serveStaticFile("/style.css", "text/css"); 
    });
    server->on("/app.js", [this]() { 
        this->serveStaticFile("/app.js", "application/javascript"); 
    });
    server->on("/config.html", [this]() { 
        this->serveStaticFile("/config.html", "text/html"); 
    });
}

void WebServerManager::setupAPIHandlers() {
    // Configuration API
    server->on("/api/config", HTTP_GET, [this]() { this->handleGetConfig(); });
    server->on("/api/status", HTTP_GET, [this]() { this->handleGetStatus(); });
    
    // Pin control API
    server->on("/api/pin/effect", HTTP_POST, [this]() { this->handlePinEffect(); });
    server->on("/api/pin/state", HTTP_POST, [this]() { this->handlePinState(); });
    
    // Audio API
    server->on("/api/audio/play", HTTP_POST, [this]() { this->handleAudioPlay(); });
    server->on("/api/audio/stop", HTTP_POST, [this]() { this->handleAudioStop(); });
    server->on("/api/audio/volume", HTTP_POST, [this]() { this->handleAudioVolume(); });
    
    // Global effects API
    server->on("/api/effects/global", HTTP_POST, [this]() { this->handleGlobalEffect(); });
    server->on("/api/effects/stop", HTTP_POST, [this]() { this->handleStopAllEffects(); });
    
    // System API
    server->on("/api/system/factory-reset", HTTP_POST, [this]() { this->handleFactoryReset(); });
}

void WebServerManager::handleRoot() {
    serveStaticFile("/index.html", "text/html");
}

void WebServerManager::handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server->uri();
    message += "\nMethod: ";
    message += (server->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server->args();
    message += "\n";
    
    for (uint8_t i = 0; i < server->args(); i++) {
        message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
    }
    
    server->send(404, "text/plain", message);
}

void WebServerManager::serveStaticFile(const String& path, const String& contentType) {
    if (!SPIFFS.exists(path)) {
        Serial.printf("File not found: %s\n", path.c_str());
        server->send(404, "text/plain", "File not found");
        return;
    }
    
    File file = SPIFFS.open(path, "r");
    if (!file) {
        Serial.printf("Failed to open file: %s\n", path.c_str());
        server->send(500, "text/plain", "Failed to open file");
        return;
    }
    
    server->streamFile(file, contentType);
    file.close();
}

void WebServerManager::handleGetConfig() {
    sendCorsHeaders();
    
    ConfigManager& configMgr = ConfigManager::getInstance();
    if (!configMgr.isInitialized()) {
        sendJsonResponse(500, "Configuration not initialized");
        return;
    }
    
    const SystemConfig& config = configMgr.getConfig();
    
    JsonDocument doc;
    doc["version"] = BATTLEARUA_VERSION;
    doc["deviceName"] = config.deviceName;
    doc["wifiSSID"] = config.wifiSSID;
    doc["wifiEnabled"] = config.wifiEnabled;
    doc["volume"] = config.volume;
    doc["audioEnabled"] = config.audioEnabled;
    doc["activePins"] = config.activePins;
    
    JsonArray pinsArray = doc["pins"].to<JsonArray>();
    for (uint8_t i = 0; i < 10; i++) {
        JsonObject pinObj = pinsArray.add<JsonObject>();
        pinObj["pin"] = config.pins[i].pin;
        pinObj["pinMode"] = static_cast<uint8_t>(config.pins[i].pinMode);
        pinObj["defaultEffect"] = static_cast<uint8_t>(config.pins[i].defaultEffect);
        pinObj["name"] = config.pins[i].name;
        pinObj["audioFile"] = config.pins[i].audioFile;
        pinObj["enabled"] = config.pins[i].enabled;
        pinObj["brightness"] = config.pins[i].brightness;
        pinObj["color"] = config.pins[i].color;
    }
    String response;
    serializeJson(doc, response);
    
    server->send(200, "application/json", response);
}

void WebServerManager::handleGetStatus() {
    sendCorsHeaders();
    
    JsonDocument doc;
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["chipModel"] = ESP.getChipModel();
    doc["flashSize"] = ESP.getFlashChipSize();
    doc["wifiMode"] = (WiFi.getMode() == WIFI_AP) ? "AP" : "STA";
    doc["ipAddress"] = getIPAddress();
    
    String response;
    serializeJson(doc, response);
    
    server->send(200, "application/json", response);
}

void WebServerManager::handlePinEffect() {
    sendCorsHeaders();
    
    if (!server->hasArg("plain")) {
        sendJsonResponse(400, "Missing request body");
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server->arg("plain"));
    
    if (error) {
        sendJsonResponse(400, "Invalid JSON");
        return;
    }
    
    if (!doc.containsKey("pin") || !doc.containsKey("effect")) {
        sendJsonResponse(400, "Missing pin or effect parameters");
        return;
    }
    
    uint8_t pin = doc["pin"];
    uint8_t effect = doc["effect"];
    uint32_t duration = doc["duration"] | 0;
    
    Serial.printf("Pin effect: GPIO %d, effect %d, duration %d\n", pin, effect, duration);
    
    sendJsonResponse(200, "Effect triggered");
}

void WebServerManager::handlePinState() {
    sendCorsHeaders();
    
    if (!server->hasArg("plain")) {
        sendJsonResponse(400, "Missing request body");
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server->arg("plain"));
    
    if (error) {
        sendJsonResponse(400, "Invalid JSON");
        return;
    }
    
    if (!doc.containsKey("pin") || !doc.containsKey("state")) {
        sendJsonResponse(400, "Missing pin or state parameters");
        return;
    }
    
    uint8_t pin = doc["pin"];
    bool state = doc["state"];
    
    GPIOManager& gpio = GPIOManager::getInstance();
    PinState pinState = state ? PinState::HIGH : PinState::LOW;
    
    if (gpio.digitalWrite(pin, pinState)) {
        Serial.printf("Pin state: GPIO %d = %s\n", pin, state ? "HIGH" : "LOW");
        sendJsonResponse(200, String("Pin ") + pin + (state ? " ON" : " OFF"));
    } else {
        sendJsonResponse(400, "Failed to set pin state");
    }
}

void WebServerManager::handleAudioPlay() {
    sendCorsHeaders();
    
    if (!server->hasArg("plain")) {
        sendJsonResponse(400, "Missing request body");
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server->arg("plain"));
    
    if (error) {
        sendJsonResponse(400, "Invalid JSON");
        return;
    }
    
    if (!doc.containsKey("file")) {
        sendJsonResponse(400, "Missing file parameter");
        return;
    }
    
    uint8_t file = doc["file"];
    bool loop = doc["loop"] | false;
    
    Serial.printf("Audio play: file %d, loop %s\n", file, loop ? "true" : "false");
    
    sendJsonResponse(200, String("Playing audio file ") + file);
}

void WebServerManager::handleAudioStop() {
    sendCorsHeaders();
    
    Serial.println("Audio stop requested");
    
    sendJsonResponse(200, "Audio stopped");
}

void WebServerManager::handleAudioVolume() {
    sendCorsHeaders();
    
    if (!server->hasArg("plain")) {
        sendJsonResponse(400, "Missing request body");
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server->arg("plain"));
    
    if (error) {
        sendJsonResponse(400, "Invalid JSON");
        return;
    }
    
    if (!doc.containsKey("volume")) {
        sendJsonResponse(400, "Missing volume parameter");
        return;
    }
    
    uint8_t volume = doc["volume"];
    volume = constrain(volume, 0, 30);
    
    ConfigManager::getInstance().setVolume(volume);
    
    Serial.printf("Volume set to: %d\n", volume);
    
    sendJsonResponse(200, String("Volume set to ") + volume);
}

void WebServerManager::handleGlobalEffect() {
    sendCorsHeaders();
    
    if (!server->hasArg("plain")) {
        sendJsonResponse(400, "Missing request body");
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server->arg("plain"));
    
    if (error) {
        sendJsonResponse(400, "Invalid JSON");
        return;
    }
    
    if (!doc.containsKey("effect")) {
        sendJsonResponse(400, "Missing effect parameter");
        return;
    }
    
    String effect = doc["effect"];
    
    Serial.printf("Global effect: %s\n", effect.c_str());
    
    sendJsonResponse(200, String("Global effect ") + effect + " triggered");
}

void WebServerManager::handleStopAllEffects() {
    sendCorsHeaders();
    
    Serial.println("Stop all effects requested");
    
    sendJsonResponse(200, "All effects stopped");
}

void WebServerManager::handleFactoryReset() {
    sendCorsHeaders();
    
    Serial.println("Factory reset requested");
    
    ConfigManager::getInstance().resetToDefaults();
    
    sendJsonResponse(200, "Factory reset completed");
}

void WebServerManager::sendJsonResponse(int code, const String& message) {
    JsonDocument doc;
    doc["status"] = (code == 200) ? "success" : "error";
    doc["message"] = message;
    doc["timestamp"] = millis();
    
    String response;
    serializeJson(doc, response);
    
    server->send(code, "application/json", response);
}

void WebServerManager::sendCorsHeaders() {
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

String WebServerManager::getContentType(const String& filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    return "text/plain";
}