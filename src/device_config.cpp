#include <Arduino.h>
#include "config.h"
#include "effects.h"

// Global device configuration
DeviceConfig deviceConfig;

// Load default configuration values
void loadDefaultConfig() {
  deviceConfig.deviceName = "BattleAura Device";
  
  // Hardware defaults
  deviceConfig.hasAudio = true;
  deviceConfig.hasLEDs = true;
  
  // Behavior defaults  
  deviceConfig.autoStartIdle = true;
  deviceConfig.defaultVolume = 20;
  deviceConfig.defaultBrightness = 100;
  
  // Power management defaults
  deviceConfig.idleTimeoutMs = IDLE_TIMEOUT_MS;
  deviceConfig.enableSleep = true;
  
  // Default background effects (Tank configuration)
  deviceConfig.backgroundEffects[0] = EffectMapping(D0, "candle", "Candle 1", true);
  deviceConfig.backgroundEffects[1] = EffectMapping(D1, "candle", "Candle 2", true);
  deviceConfig.backgroundEffects[2] = EffectMapping(D2, "candle", "Brazier", true);
  deviceConfig.backgroundEffects[3] = EffectMapping(D3, "console", "Console", true);
  deviceConfig.backgroundEffects[4] = EffectMapping(D4, "off", "Unused", false);
  deviceConfig.backgroundEffects[5] = EffectMapping(D5, "off", "Unused", false);
  deviceConfig.backgroundEffects[6] = EffectMapping(D8, "pulse", "Engine 1", true);
  deviceConfig.backgroundEffects[7] = EffectMapping(D9, "pulse", "Engine 2", true);
  
  // Default trigger effects (Tank configuration)
  deviceConfig.triggerEffects[0] = TriggerMapping("machine-gun", D4, AUDIO_WEAPON_FIRE_1, "Machine Gun", true);
  deviceConfig.triggerEffects[1] = TriggerMapping("flamethrower", D5, AUDIO_WEAPON_FIRE_2, "Flamethrower", true);
  deviceConfig.triggerEffects[2] = TriggerMapping("engine-rev", D8, AUDIO_ENGINE_REV, "Engine Rev", true);
  deviceConfig.triggerEffects[3] = TriggerMapping("rocket", D0, AUDIO_LIMITED_WEAPON, "Rocket", true);
  deviceConfig.triggerEffects[4] = TriggerMapping("taking-hits", D3, AUDIO_TAKING_HITS, "Taking Hits", true);
  deviceConfig.triggerEffects[5] = TriggerMapping("destroyed", 0, AUDIO_DESTROYED, "Destroyed", true);
  deviceConfig.triggerEffects[6] = TriggerMapping("unit-kill", D3, AUDIO_UNIT_KILL, "Unit Kill", true);
  deviceConfig.triggerEffects[7] = TriggerMapping(); // Empty slot
  
  Serial.println("Loaded default device configuration");
  Serial.println("Device: " + deviceConfig.deviceName);
  Serial.println("Background effects configured: 8");
  Serial.println("Trigger effects configured: 7");
}

// Load device-specific configuration (placeholder for future EEPROM/JSON loading)
void loadDeviceConfig() {
  // For now, just load defaults
  // Future: Load from EEPROM, SPIFFS, or web configuration
  loadDefaultConfig();
  
  // Device-specific overrides could go here
  // Example: if (deviceType == "Artillery") { ledCount = 12; }
}

// Save device configuration (placeholder for future EEPROM/JSON saving)
void saveDeviceConfig() {
  // Future: Save to EEPROM or SPIFFS
  Serial.println("Device configuration saved (placeholder)");
}

// Run all enabled background effects based on configuration
void runBackgroundEffects() {
  for (int i = 0; i < 8; i++) {
    if (deviceConfig.backgroundEffects[i].enabled) {
      runEffect(deviceConfig.backgroundEffects[i].ledPin, 
                deviceConfig.backgroundEffects[i].effectType);
    }
  }
}

// Execute a specific effect on a specific pin
void runEffect(int ledPin, String effectType) {
  if (effectType == "candle") {
    candleFlicker(ledPin);
  } else if (effectType == "pulse") {
    enginePulseSmooth(ledPin, 0);
  } else if (effectType == "console") {
    consoleDataStream(ledPin);
  } else if (effectType == "static") {
    setLED(ledPin, 255); // Full brightness static
  } else if (effectType == "off") {
    setLED(ledPin, 0); // Off
  }
  // Add more effect types as needed
}