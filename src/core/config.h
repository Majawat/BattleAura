#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "../hal/gpio_manager.h"

#define BATTLEARUA_VERSION "1.1.0"
#define CONFIG_FILE "/config.json"
#define DEFAULT_VOLUME 15

enum class EffectType {
    NONE = 0,
    CANDLE_FLICKER = 1,
    ENGINE_PULSE = 2,
    MACHINE_GUN = 3,
    FLAMETHROWER = 4,
    ROCKET_LAUNCHER = 5,
    TAKING_DAMAGE = 6,
    EXPLOSION = 7,
    CONSOLE_RGB = 8,
    STATIC_ON = 9,
    STATIC_OFF = 10
};

struct PinConfig {
    uint8_t pin;
    PinMode pinMode;
    EffectType defaultEffect;
    String name;
    uint8_t audioFile;
    bool enabled;
    uint8_t brightness;
    uint32_t color;
    
    // Default constructor
    PinConfig() : pin(0), pinMode(PinMode::PIN_DISABLED), defaultEffect(EffectType::NONE),
                  name("Unused"), audioFile(0), enabled(false), brightness(255), color(0xFFFFFF) {}
};

struct SystemConfig {
    String deviceName;
    String wifiSSID;  
    String wifiPassword;
    bool wifiEnabled;
    uint8_t volume;
    bool audioEnabled;
    PinConfig pins[10]; // Max 10 configurable pins
    uint8_t activePins;
    
    // Default constructor
    SystemConfig() : deviceName(""), wifiSSID(""), wifiPassword(""), 
                     wifiEnabled(false), volume(DEFAULT_VOLUME), audioEnabled(true), activePins(0) {}
};

class ConfigManager {
public:
    static ConfigManager& getInstance();
    
    // Lifecycle
    bool initialize();
    bool isInitialized() const { return initialized; }
    
    // Configuration access
    const SystemConfig& getConfig() const { return config; }
    SystemConfig& getMutableConfig() { return config; }
    
    // Persistence
    bool loadFromFile();
    bool saveToFile();
    bool resetToDefaults();
    
    // Convenience methods
    bool setWiFiCredentials(const String& ssid, const String& password);
    bool setVolume(uint8_t volume);
    bool setPinConfig(uint8_t index, const PinConfig& pinConfig);
    const PinConfig* getPinByGPIO(uint8_t gpio) const;
    
private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    SystemConfig config;
    bool initialized = false;
    bool spiffsAvailable = false;
    
    void initializeDefaults();
    bool initializeSPIFFS();
    JsonDocument configToJson() const;
    bool jsonToConfig(const JsonDocument& doc);
};