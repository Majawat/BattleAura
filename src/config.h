#ifndef CONFIG_H
#define CONFIG_H

// Firmware version
#define FIRMWARE_VERSION "0.25.0"
#define VERSION_FEATURE "Universal pin-to-effect configuration system"
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

// Effect mapping for continuous background effects
struct EffectMapping {
  int ledPin;
  String effectType;  // "candle", "pulse", "console", "static", "off"
  String label;       // User-friendly name
  bool enabled;
};

// Trigger mapping for button-activated effects
struct TriggerMapping {
  String buttonId;    // "machine-gun", "flamethrower", etc.
  int ledPin;
  int audioTrack;
  String label;       // User-friendly name
  bool enabled;
};

// Device configuration structure
struct DeviceConfig {
  // Device identification
  String deviceName;
  
  // Hardware configuration
  bool hasAudio;
  bool hasLEDs;
  
  // Behavior settings
  bool autoStartIdle;
  int defaultVolume;
  int defaultBrightness;
  
  // Power management
  int idleTimeoutMs;
  bool enableSleep;
  
  // Effect configuration (8 pins max for now)
  EffectMapping backgroundEffects[8];
  TriggerMapping triggerEffects[8];
};

// Default device configuration
extern DeviceConfig deviceConfig;

// Configuration functions
void loadDefaultConfig();
void loadDeviceConfig();
void saveDeviceConfig();

// Effect execution functions
void runBackgroundEffects();
void runEffect(int ledPin, String effectType);

#endif