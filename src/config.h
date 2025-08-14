#ifndef CONFIG_H
#define CONFIG_H

// Firmware version
#define FIRMWARE_VERSION "0.24.0"
#define VERSION_FEATURE "Add device configuration system and centralized constants"
#define BUILD_DATE __DATE__ " " __TIME__

// Audio file mappings
#define AUDIO_IDLE        2
#define AUDIO_WEAPON_FIRE_1    3
#define AUDIO_WEAPON_FIRE_2    4
#define AUDIO_TAKING_HITS      5
#define AUDIO_ENGINE_REV       6
#define AUDIO_DESTROYED        7
#define AUDIO_LIMITED_WEAPON   8
#define AUDIO_UNIT_KILL        9

// Configuration constants
#define IDLE_TIMEOUT_MS   30000  // 30 seconds
#define STATUS_CHECK_INTERVAL_MS  10000  // 10 seconds
#define LED_UPDATE_DELAY_MS  50   // 50ms LED update cycle

// Device configuration structure
struct DeviceConfig {
  // Device identification
  String deviceName;
  String deviceType;
  
  // Hardware configuration
  bool hasAudio;
  bool hasLEDs;
  int ledCount;
  
  // Behavior settings
  bool autoStartIdle;
  int defaultVolume;
  int defaultBrightness;
  
  // Power management
  int idleTimeoutMs;
  bool enableSleep;
};

// Default device configuration
extern DeviceConfig deviceConfig;

// Configuration functions
void loadDefaultConfig();
void loadDeviceConfig();
void saveDeviceConfig();

#endif