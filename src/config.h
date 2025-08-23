#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "DFRobotDFPlayerMini.h"

// Firmware version
#define FIRMWARE_VERSION "0.32.0"
#define VERSION_FEATURE "Modern web interface with JSON API and real-time WebSocket updates"
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

// Modular pin configuration
struct PinConfig {
  int pin;                    // Physical pin number (D0-D10)
  String type;               // "led", "rgb", "audio", "unused"
  String label;              // User-friendly name like "Front Weapon", "Engine Glow"
  bool enabled;              // Is this pin active
  
  // LED-specific settings
  String effectType;         // "candle", "pulse", "console", "static", "off", "rgb_*"
  int brightness;            // Default brightness 0-255
  
  // RGB-specific settings (if type="rgb")
  int defaultR, defaultG, defaultB;  // Default RGB values
  
  // Constructor
  PinConfig() : pin(0), type("unused"), label(""), enabled(false), 
                effectType("off"), brightness(255), defaultR(255), defaultG(255), defaultB(255) {}
  PinConfig(int p, String t, String lbl) : pin(p), type(t), label(lbl), enabled(true),
                effectType("off"), brightness(255), defaultR(255), defaultG(255), defaultB(255) {}
};

// Audio track configuration
struct AudioTrack {
  int id;                    // Track number (1-99)
  String filename;           // MP3 filename on SD card
  String label;              // User-friendly name like "Machine Gun", "Engine Idle"
  String category;           // "weapon", "ambient", "damage", "victory"
  bool loops;                // Should this track loop?
  int volume;                // Track-specific volume (0-30)
  bool enabled;              // Is this track available
  
  // Constructor
  AudioTrack() : id(0), filename(""), label(""), category("other"), 
                 loops(false), volume(20), enabled(false) {}
  AudioTrack(int trackId, String file, String lbl, String cat) : 
             id(trackId), filename(file), label(lbl), category(cat),
             loops(false), volume(20), enabled(true) {}
};

// Trigger effect mapping - combines pins and audio
struct TriggerEffect {
  String effectId;           // "weapon1", "weapon2", "damage", "victory", etc.
  String label;              // User-friendly name
  String description;        // What this effect does
  bool enabled;              // Is this effect available
  
  // Pin assignments
  int primaryPin;            // Main LED pin for this effect
  int secondaryPin;          // Optional second LED pin (-1 if unused)
  
  // Audio assignment
  int audioTrack;            // Which audio track to play (0 = none)
  
  // Effect parameters
  String effectPattern;      // "flash", "pulse", "fade", "strobe", "custom"
  int duration;              // Effect duration in ms (0 = until audio ends)
  int intensity;             // Effect intensity 0-100%
  
  // Constructor
  TriggerEffect() : effectId(""), label(""), description(""), enabled(false),
                    primaryPin(-1), secondaryPin(-1), audioTrack(0),
                    effectPattern("flash"), duration(0), intensity(100) {}
};

// Device configuration structure
struct DeviceConfig {
  // Device identification
  String deviceName;
  String deviceDescription;
  
  // Global settings
  int defaultVolume;           // Global volume setting
  int defaultBrightness;       // Global brightness setting
  int idleTimeoutMs;           // Idle audio timeout
  bool enableSleep;            // Power management
  bool hasLEDs;                // LED system enabled
  
  // Modular configuration arrays
  PinConfig pins[11];          // D0-D10 pin configurations
  AudioTrack audioTracks[20];  // Up to 20 audio tracks
  TriggerEffect effects[12];   // Up to 12 custom effects
  
  // Active configuration counts
  int activePins;
  int activeAudioTracks;  
  int activeEffects;
};

// Default device configuration
extern DeviceConfig deviceConfig;

// Configuration functions
void loadDefaultConfig();
void loadDeviceConfig();
void saveDeviceConfig();

// Helper functions for dynamic pin management
int getPinByLabel(const String& label);
int getPinByType(const String& type);
int getRGBPin();
bool isRGBPin(int pin);
TriggerEffect* getEffectById(const String& effectId);

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