#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

class EffectManager {
public:
  static EffectManager& getInstance();
  void begin();
  void update();
  void triggerEffect(uint8_t pin, EffectType effect, uint32_t duration = 0);
  void stopEffect(uint8_t pin);
  void stopAllEffects();
  void setBrightness(uint8_t pin, uint8_t brightness);
  void setColor(uint8_t pin, uint32_t color);
  
private:
  EffectManager() = default;
  
  struct EffectState {
    bool active;
    EffectType type;
    uint32_t startTime;
    uint32_t duration;
    uint32_t lastUpdate;
    uint16_t stepCount;
    uint8_t brightness;
    uint32_t color;
    bool audioTriggered;
    
    union {
      struct {
        uint8_t intensity;
        uint32_t nextChange;
      } candle;
      
      struct {
        uint8_t phase;
        bool direction;
      } pulse;
      
      struct {
        uint8_t burstCount;
        uint32_t burstDelay;
        bool burstActive;
      } weapon;
      
      struct {
        uint8_t red, green, blue;
        uint8_t mode;
      } rgb;
    } params;
  };
  
  EffectState effects[MAX_PINS];
  Adafruit_NeoPixel* neoPixels[MAX_PINS];
  
  void initPin(uint8_t index, const PinConfig& pinConfig);
  void updateCandleFlicker(uint8_t index);
  void updateEnginePulse(uint8_t index);
  void updateMachineGun(uint8_t index);
  void updateFlamethrower(uint8_t index);
  void updateRocketLauncher(uint8_t index);
  void updateTakingDamage(uint8_t index);
  void updateExplosion(uint8_t index);
  void updateConsoleRGB(uint8_t index);
  void updateStaticOn(uint8_t index);
  void updateStaticOff(uint8_t index);
  
  void setStandardLED(uint8_t pin, uint8_t value);
  void setNeoPixel(uint8_t index, uint32_t color);
  uint32_t scaleColor(uint32_t color, uint8_t brightness);
  uint32_t blend(uint32_t color1, uint32_t color2, uint8_t amount);
};