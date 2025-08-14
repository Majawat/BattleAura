#include "config.h"
#include <Arduino.h>

// Global device configuration
DeviceConfig deviceConfig;

// Load default configuration values
void loadDefaultConfig() {
  deviceConfig.deviceName = "BattleAura Tank";
  deviceConfig.deviceType = "Tank";
  
  // Hardware defaults
  deviceConfig.hasAudio = true;
  deviceConfig.hasLEDs = true;
  deviceConfig.ledCount = 8;
  
  // Behavior defaults  
  deviceConfig.autoStartIdle = true;
  deviceConfig.defaultVolume = 20;
  deviceConfig.defaultBrightness = 100;
  
  // Power management defaults
  deviceConfig.idleTimeoutMs = IDLE_TIMEOUT_MS;
  deviceConfig.enableSleep = true;
  
  Serial.println("Loaded default device configuration");
  Serial.println("Device: " + deviceConfig.deviceName);
  Serial.println("Type: " + deviceConfig.deviceType);
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