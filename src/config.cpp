#include "config.h"

ConfigManager& ConfigManager::getInstance() {
  static ConfigManager instance;
  return instance;
}

bool ConfigManager::begin() {
  if (!SPIFFS.begin(true)) {
    Serial.println(F("SPIFFS Mount Failed"));
    return false;
  }
  
  initDefaultConfig();
  
  if (!loadConfig()) {
    Serial.println("Failed to load config, using defaults");
    saveConfig();
  }
  
  return true;
}

void ConfigManager::initDefaultConfig() {
  config.deviceName = "BattleAura-" + String(random(1000, 9999));
  config.wifiSSID = "";
  config.wifiPassword = "";
  config.wifiEnabled = false;
  config.volume = DEFAULT_VOLUME;
  config.audioEnabled = true;
  config.activePins = 0;
  
  for (int i = 0; i < MAX_PINS; i++) {
    config.pins[i] = {
      .pin = 0,
      .type = PinType::PIN_DISABLED,
      .effect = EffectType::NONE,
      .name = "Unused",
      .audioFile = "",
      .enabled = false,
      .brightness = 255,
      .color = 0xFFFFFF
    };
  }
  
  config.pins[0] = {
    .pin = 2,
    .type = PinType::STANDARD_LED,
    .effect = EffectType::CANDLE_FLICKER,
    .name = "Candle Fibers 1",
    .audioFile = "",
    .enabled = true,
    .brightness = 200,
    .color = 0xFF6600
  };
  config.activePins = 1;
}

bool ConfigManager::loadConfig() {
  File file = SPIFFS.open(CONFIG_FILE, "r");
  if (!file) {
    Serial.println("Config file not found");
    return false;
  }
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.printf("Config parse error: %s\n", error.c_str());
    return false;
  }
  
  config.deviceName = doc["deviceName"] | config.deviceName;
  config.wifiSSID = doc["wifiSSID"] | "";
  config.wifiPassword = doc["wifiPassword"] | "";
  config.wifiEnabled = doc["wifiEnabled"] | false;
  config.volume = doc["volume"] | DEFAULT_VOLUME;
  config.audioEnabled = doc["audioEnabled"] | true;
  config.activePins = doc["activePins"] | 0;
  
  JsonArray pinsArray = doc["pins"];
  if (pinsArray) {
    for (int i = 0; i < MAX_PINS && i < pinsArray.size(); i++) {
      JsonObject pinObj = pinsArray[i];
      config.pins[i] = {
        .pin = pinObj["pin"] | 0,
        .type = static_cast<PinType>(pinObj["type"] | 0),
        .effect = static_cast<EffectType>(pinObj["effect"] | 0),
        .name = pinObj["name"] | "Unused",
        .audioFile = pinObj["audioFile"] | "",
        .enabled = pinObj["enabled"] | false,
        .brightness = pinObj["brightness"] | 255,
        .color = pinObj["color"] | 0xFFFFFF
      };
    }
  }
  
  Serial.println("Config loaded successfully");
  return true;
}

bool ConfigManager::saveConfig() {
  JsonDocument doc;
  
  doc["deviceName"] = config.deviceName;
  doc["wifiSSID"] = config.wifiSSID;
  doc["wifiPassword"] = config.wifiPassword;
  doc["wifiEnabled"] = config.wifiEnabled;
  doc["volume"] = config.volume;
  doc["audioEnabled"] = config.audioEnabled;
  doc["activePins"] = config.activePins;
  
  JsonArray pinsArray = doc["pins"].to<JsonArray>();
  for (int i = 0; i < MAX_PINS; i++) {
    JsonObject pinObj = pinsArray.add<JsonObject>();
    pinObj["pin"] = config.pins[i].pin;
    pinObj["type"] = static_cast<int>(config.pins[i].type);
    pinObj["effect"] = static_cast<int>(config.pins[i].effect);
    pinObj["name"] = config.pins[i].name;
    pinObj["audioFile"] = config.pins[i].audioFile;
    pinObj["enabled"] = config.pins[i].enabled;
    pinObj["brightness"] = config.pins[i].brightness;
    pinObj["color"] = config.pins[i].color;
  }
  
  File file = SPIFFS.open(CONFIG_FILE, "w");
  if (!file) {
    Serial.println("Failed to create config file");
    return false;
  }
  
  serializeJson(doc, file);
  file.close();
  
  Serial.println("Config saved successfully");
  return true;
}

void ConfigManager::resetToDefaults() {
  initDefaultConfig();
  saveConfig();
}

PinConfig* ConfigManager::getPinConfig(uint8_t pin) {
  for (int i = 0; i < MAX_PINS; i++) {
    if (config.pins[i].pin == pin && config.pins[i].enabled) {
      return &config.pins[i];
    }
  }
  return nullptr;
}

void ConfigManager::setPinConfig(uint8_t index, const PinConfig& pinConfig) {
  if (index < MAX_PINS) {
    config.pins[index] = pinConfig;
    
    config.activePins = 0;
    for (int i = 0; i < MAX_PINS; i++) {
      if (config.pins[i].enabled) {
        config.activePins++;
      }
    }
  }
}

void ConfigManager::setWiFiCredentials(const String& ssid, const String& password) {
  config.wifiSSID = ssid;
  config.wifiPassword = password;
  config.wifiEnabled = (ssid.length() > 0);
}

void ConfigManager::setVolume(uint8_t volume) {
  config.volume = constrain(volume, 0, 30);
}