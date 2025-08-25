#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#define BATTLEARUA_VERSION "1.0.0"
#define BATTLEARUA_BUILD_DATE __DATE__

#define MAX_PINS 10
#define CONFIG_FILE "/config.json"
#define DEFAULT_VOLUME 15
#define DFPLAYER_TX_PIN 21
#define DFPLAYER_RX_PIN 20

enum class PinType {
  PIN_DISABLED,
  STANDARD_LED,
  WS2812B,
  ANALOG_INPUT,
  DIGITAL_INPUT
};

enum class EffectType {
  NONE,
  CANDLE_FLICKER,
  ENGINE_PULSE,
  MACHINE_GUN,
  FLAMETHROWER,
  ROCKET_LAUNCHER,
  TAKING_DAMAGE,
  EXPLOSION,
  CONSOLE_RGB,
  STATIC_ON,
  STATIC_OFF
};

struct PinConfig {
  uint8_t pin;
  PinType type;
  EffectType effect;
  String name;
  String audioFile;
  bool enabled;
  uint8_t brightness;
  uint32_t color;
};

struct SystemConfig {
  String deviceName;
  String wifiSSID;
  String wifiPassword;
  bool wifiEnabled;
  uint8_t volume;
  bool audioEnabled;
  PinConfig pins[MAX_PINS];
  uint8_t activePins;
};

class ConfigManager {
public:
  static ConfigManager& getInstance();
  bool begin();
  bool loadConfig();
  bool saveConfig();
  void resetToDefaults();
  
  SystemConfig& getConfig() { return config; }
  PinConfig* getPinConfig(uint8_t pin);
  void setPinConfig(uint8_t index, const PinConfig& pinConfig);
  void setWiFiCredentials(const String& ssid, const String& password);
  void setVolume(uint8_t volume);
  
private:
  ConfigManager() = default;
  SystemConfig config;
  void initDefaultConfig();
};