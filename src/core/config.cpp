#include "config.h"

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::initialize() {
    if (initialized) return true;
    
    Serial.println("Initializing ConfigManager...");
    
    // Always initialize defaults first
    initializeDefaults();
    
    // Try to initialize SPIFFS
    spiffsAvailable = initializeSPIFFS();
    if (!spiffsAvailable) {
        Serial.println("SPIFFS not available - using defaults only");
    }
    
    // Try to load config from file
    if (spiffsAvailable) {
        if (!loadFromFile()) {
            Serial.println("Config file not found or invalid - using defaults");
        }
    }
    
    initialized = true;
    Serial.println("ConfigManager initialized successfully");
    return true;
}

bool ConfigManager::initializeSPIFFS() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed");
        return false;
    }
    
    Serial.println("SPIFFS initialized successfully");
    return true;
}

void ConfigManager::initializeDefaults() {
    config = SystemConfig(); // Use default constructor
    config.deviceName = "BattleAura-" + String(random(1000, 9999));
    
    // Set up one default pin configuration
    config.pins[0].pin = 2;
    config.pins[0].pinMode = PinMode::OUTPUT_PWM;
    config.pins[0].defaultEffect = EffectType::CANDLE_FLICKER;
    config.pins[0].name = "Test LED";
    config.pins[0].audioFile = 0;
    config.pins[0].enabled = true;
    config.pins[0].brightness = 200;
    config.pins[0].color = 0xFF6600;
    config.activePins = 1;
    
    Serial.println("Default configuration initialized");
}

bool ConfigManager::loadFromFile() {
    if (!spiffsAvailable) return false;
    
    if (!SPIFFS.exists(CONFIG_FILE)) {
        Serial.println("Config file does not exist");
        return false;
    }
    
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("Failed to open config file for reading");
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("Failed to parse config file: %s\n", error.c_str());
        return false;
    }
    
    bool success = jsonToConfig(doc);
    if (success) {
        Serial.println("Configuration loaded from file");
    } else {
        Serial.println("Failed to parse configuration data");
    }
    
    return success;
}

bool ConfigManager::saveToFile() {
    if (!spiffsAvailable) {
        Serial.println("Cannot save config - SPIFFS not available");
        return false;
    }
    
    JsonDocument doc = configToJson();
    
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("Failed to open config file for writing");
        return false;
    }
    
    size_t bytesWritten = serializeJson(doc, file);
    file.close();
    
    if (bytesWritten == 0) {
        Serial.println("Failed to write config file");
        return false;
    }
    
    Serial.printf("Configuration saved to file (%d bytes)\n", bytesWritten);
    return true;
}

bool ConfigManager::resetToDefaults() {
    initializeDefaults();
    return saveToFile();
}

JsonDocument ConfigManager::configToJson() const {
    JsonDocument doc;
    
    doc["version"] = BATTLEARUA_VERSION;
    doc["deviceName"] = config.deviceName;
    doc["wifiSSID"] = config.wifiSSID;
    doc["wifiPassword"] = config.wifiPassword;
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
    
    return doc;
}

bool ConfigManager::jsonToConfig(const JsonDocument& doc) {
    if (!doc["version"].is<const char*>()) {
        Serial.println("Config file missing version field");
        return false;
    }
    
    config.deviceName = doc["deviceName"] | config.deviceName;
    config.wifiSSID = doc["wifiSSID"] | "";
    config.wifiPassword = doc["wifiPassword"] | "";
    config.wifiEnabled = doc["wifiEnabled"] | false;
    config.volume = doc["volume"] | DEFAULT_VOLUME;
    config.audioEnabled = doc["audioEnabled"] | true;
    config.activePins = doc["activePins"] | 0;
    
    if (doc["pins"].is<JsonArray>()) {
        JsonArray pinsArray = doc["pins"].as<JsonArray>();
        for (uint8_t i = 0; i < 10 && i < pinsArray.size(); i++) {
            JsonObject pinObj = pinsArray[i];
            config.pins[i].pin = pinObj["pin"] | 0;
            config.pins[i].pinMode = static_cast<PinMode>(pinObj["pinMode"] | 0);
            config.pins[i].defaultEffect = static_cast<EffectType>(pinObj["defaultEffect"] | 0);
            config.pins[i].name = pinObj["name"] | "Unused";
            config.pins[i].audioFile = pinObj["audioFile"] | 0;
            config.pins[i].enabled = pinObj["enabled"] | false;
            config.pins[i].brightness = pinObj["brightness"] | 255;
            config.pins[i].color = pinObj["color"] | 0xFFFFFF;
        }
    }
    
    return true;
}

bool ConfigManager::setWiFiCredentials(const String& ssid, const String& password) {
    config.wifiSSID = ssid;
    config.wifiPassword = password;
    config.wifiEnabled = (ssid.length() > 0);
    return saveToFile();
}

bool ConfigManager::setVolume(uint8_t volume) {
    config.volume = constrain(volume, 0, 30);
    return saveToFile();
}

bool ConfigManager::setPinConfig(uint8_t index, const PinConfig& pinConfig) {
    if (index >= 10) return false;
    
    config.pins[index] = pinConfig;
    
    // Recalculate active pins
    config.activePins = 0;
    for (uint8_t i = 0; i < 10; i++) {
        if (config.pins[i].enabled) {
            config.activePins++;
        }
    }
    
    return saveToFile();
}

const PinConfig* ConfigManager::getPinByGPIO(uint8_t gpio) const {
    for (uint8_t i = 0; i < 10; i++) {
        if (config.pins[i].enabled && config.pins[i].pin == gpio) {
            return &config.pins[i];
        }
    }
    return nullptr;
}