#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "DFRobotDFPlayerMini.h"

// Firmware version
#define FIRMWARE_VERSION "0.29.0"
#define VERSION_FEATURE "Convert to fully non-blocking operation for responsive web interface"
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
  
  // Constructor for easy initialization
  EffectMapping() : ledPin(0), effectType("off"), label(""), enabled(false) {}
  EffectMapping(int pin, String type, String lbl, bool en) : 
    ledPin(pin), effectType(type), label(lbl), enabled(en) {}
};

// Trigger mapping for button-activated effects
struct TriggerMapping {
  String buttonId;    // "machine-gun", "flamethrower", etc.
  int ledPin;
  int audioTrack;
  String label;       // User-friendly name
  bool enabled;
  
  // Constructor for easy initialization
  TriggerMapping() : buttonId(""), ledPin(0), audioTrack(0), label(""), enabled(false) {}
  TriggerMapping(String id, int pin, int audio, String lbl, bool en) :
    buttonId(id), ledPin(pin), audioTrack(audio), label(lbl), enabled(en) {}
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

// Weapon effect state tracking
struct WeaponEffectState {
  bool active;
  unsigned long startTime;
  unsigned long lastUpdate;
  int currentStep;
  int ledPin;
  int audioTrack;
};

extern WeaponEffectState weaponEffects[4]; // Support up to 4 concurrent weapon effects

// Battle effect state tracking
struct BattleEffectState {
  bool active;
  String effectType; // "taking-hits", "destroyed", "rocket", "unit-kill"
  unsigned long startTime;
  unsigned long lastUpdate;
  int currentStep;
  int currentCycle;
  int audioTrack;
  bool audioStarted;
  int maxDuration; // Maximum effect duration in ms
};

extern BattleEffectState battleEffect; // Single battle effect (they don't overlap)

// Non-blocking weapon effects
void startWeaponEffect(int effectId, DFRobotDFPlayerMini* dfPlayer, int ledPin, int audioTrack, String effectType);
void updateWeaponEffects(DFRobotDFPlayerMini* dfPlayer);

// Non-blocking battle effects
void startBattleEffect(String effectType, DFRobotDFPlayerMini* dfPlayer, int audioTrack);
void updateBattleEffect(DFRobotDFPlayerMini* dfPlayer);

// Individual battle effect update functions
void updateTakingHitsEffect(unsigned long now, unsigned long elapsed);
void updateDestroyedEffect(unsigned long now, unsigned long elapsed);
void updateRocketEffect(unsigned long now, unsigned long elapsed);
void updateUnitKillEffect(unsigned long now, unsigned long elapsed);

#endif