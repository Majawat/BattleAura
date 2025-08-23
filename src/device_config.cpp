#include <Arduino.h>
#include "config.h"
#include "effects.h"
#include "dfplayer.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// Global device configuration
DeviceConfig deviceConfig;

// Weapon effect states for non-blocking effects
WeaponEffectState weaponEffects[4] = {
  {false, 0, 0, 0, 0, 0},
  {false, 0, 0, 0, 0, 0},
  {false, 0, 0, 0, 0, 0},
  {false, 0, 0, 0, 0, 0}
};

// Battle effect state for non-blocking effects
BattleEffectState battleEffect = {false, "", 0, 0, 0, 0, 0, false, 0};

// Load default configuration values
void loadDefaultConfig() {
  deviceConfig.deviceName = "BattleAura Device";
  deviceConfig.deviceDescription = "Modular LED and Audio Controller";
  
  // Global settings defaults
  deviceConfig.defaultVolume = 20;
  deviceConfig.defaultBrightness = 100;
  deviceConfig.idleTimeoutMs = IDLE_TIMEOUT_MS;
  deviceConfig.enableSleep = true;
  deviceConfig.hasLEDs = true;
  
  // Initialize pin configurations (D0-D10)
  deviceConfig.pins[0] = PinConfig(D0, "led", "Candle 1");
  deviceConfig.pins[0].effectType = "candle";
  deviceConfig.pins[0].brightness = 120;
  
  deviceConfig.pins[1] = PinConfig(D1, "led", "Candle 2");
  deviceConfig.pins[1].effectType = "candle";
  deviceConfig.pins[1].brightness = 120;
  
  deviceConfig.pins[2] = PinConfig(D2, "led", "Brazier");
  deviceConfig.pins[2].effectType = "candle";
  deviceConfig.pins[2].brightness = 150;
  
  deviceConfig.pins[3] = PinConfig(D3, "rgb", "Console Screen");
  deviceConfig.pins[3].effectType = "console";
  deviceConfig.pins[3].defaultR = 0;
  deviceConfig.pins[3].defaultG = 100;
  deviceConfig.pins[3].defaultB = 200;
  
  deviceConfig.pins[4] = PinConfig(D4, "led", "Weapon 1");
  deviceConfig.pins[4].effectType = "off";
  deviceConfig.pins[4].enabled = false;
  
  deviceConfig.pins[5] = PinConfig(D5, "led", "Weapon 2");
  deviceConfig.pins[5].effectType = "off";
  deviceConfig.pins[5].enabled = false;
  
  deviceConfig.pins[6] = PinConfig(); // D6 reserved for DFPlayer TX
  deviceConfig.pins[6].type = "unused";
  deviceConfig.pins[6].enabled = false;
  
  deviceConfig.pins[7] = PinConfig(); // D7 reserved for DFPlayer RX
  deviceConfig.pins[7].type = "unused";
  deviceConfig.pins[7].enabled = false;
  
  deviceConfig.pins[8] = PinConfig(D8, "led", "Engine 1");
  deviceConfig.pins[8].effectType = "pulse";
  deviceConfig.pins[8].brightness = 100;
  
  deviceConfig.pins[9] = PinConfig(D9, "led", "Engine 2");
  deviceConfig.pins[9].effectType = "pulse";
  deviceConfig.pins[9].brightness = 100;
  
  deviceConfig.pins[10] = PinConfig(D10, "led", "Spare");
  deviceConfig.pins[10].effectType = "off";
  deviceConfig.pins[10].enabled = false;
  
  // Initialize audio tracks with numbered filenames for DFPlayer
  deviceConfig.audioTracks[0] = AudioTrack(1, "0001.mp3", "Reserved", "system");
  deviceConfig.audioTracks[0].enabled = false;
  
  deviceConfig.audioTracks[1] = AudioTrack(AUDIO_IDLE, "0002.mp3", "Engine Idle", "ambient");
  deviceConfig.audioTracks[1].loops = true;
  deviceConfig.audioTracks[1].volume = 15;
  
  deviceConfig.audioTracks[2] = AudioTrack(AUDIO_WEAPON_FIRE_1, "0003.mp3", "Machine Gun", "weapon");
  deviceConfig.audioTracks[2].volume = 25;
  
  deviceConfig.audioTracks[3] = AudioTrack(AUDIO_WEAPON_FIRE_2, "0004.mp3", "Flamethrower", "weapon");
  deviceConfig.audioTracks[3].volume = 22;
  
  deviceConfig.audioTracks[4] = AudioTrack(AUDIO_TAKING_HITS, "0005.mp3", "Taking Damage", "damage");
  deviceConfig.audioTracks[4].volume = 28;
  
  deviceConfig.audioTracks[5] = AudioTrack(AUDIO_ENGINE_REV, "0006.mp3", "Engine Rev", "ambient");
  deviceConfig.audioTracks[5].volume = 20;
  
  deviceConfig.audioTracks[6] = AudioTrack(AUDIO_DESTROYED, "0007.mp3", "Destruction", "damage");
  deviceConfig.audioTracks[6].volume = 30;
  
  deviceConfig.audioTracks[7] = AudioTrack(AUDIO_LIMITED_WEAPON, "0008.mp3", "Rocket Fire", "weapon");
  deviceConfig.audioTracks[7].volume = 27;
  
  deviceConfig.audioTracks[8] = AudioTrack(AUDIO_UNIT_KILL, "0009.mp3", "Victory", "victory");
  deviceConfig.audioTracks[8].volume = 25;
  
  // Initialize trigger effects
  deviceConfig.effects[0] = TriggerEffect();
  deviceConfig.effects[0].effectId = "weapon1";
  deviceConfig.effects[0].label = "Machine Gun";
  deviceConfig.effects[0].primaryPin = D4;
  deviceConfig.effects[0].audioTrack = AUDIO_WEAPON_FIRE_1;
  deviceConfig.effects[0].effectPattern = "flash";
  deviceConfig.effects[0].duration = 1500;
  deviceConfig.effects[0].enabled = true;
  
  deviceConfig.effects[1] = TriggerEffect();
  deviceConfig.effects[1].effectId = "weapon2";
  deviceConfig.effects[1].label = "Flamethrower";
  deviceConfig.effects[1].primaryPin = D5;
  deviceConfig.effects[1].audioTrack = AUDIO_WEAPON_FIRE_2;
  deviceConfig.effects[1].effectPattern = "pulse";
  deviceConfig.effects[1].duration = 2000;
  deviceConfig.effects[1].enabled = true;
  
  deviceConfig.effects[2] = TriggerEffect();
  deviceConfig.effects[2].effectId = "engine-rev";
  deviceConfig.effects[2].label = "Engine Rev";
  deviceConfig.effects[2].primaryPin = D8;
  deviceConfig.effects[2].secondaryPin = D9;
  deviceConfig.effects[2].audioTrack = AUDIO_ENGINE_REV;
  deviceConfig.effects[2].effectPattern = "pulse";
  deviceConfig.effects[2].duration = 3000;
  deviceConfig.effects[2].enabled = true;
  
  // Set active counts
  deviceConfig.activePins = 6; // D0, D1, D2, D3, D8, D9
  deviceConfig.activeAudioTracks = 8;
  deviceConfig.activeEffects = 3;
  
  Serial.println("Loaded default device configuration");
  Serial.println("Device: " + deviceConfig.deviceName);
  Serial.println("Active pins: " + String(deviceConfig.activePins));
  Serial.println("Active audio tracks: " + String(deviceConfig.activeAudioTracks));
  Serial.println("Active effects: " + String(deviceConfig.activeEffects));
}

// Load device-specific configuration from SPIFFS
void loadDeviceConfig() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed, using defaults");
    loadDefaultConfig();
    return;
  }
  
  if (!SPIFFS.exists("/config.json")) {
    Serial.println("No saved configuration found, creating defaults");
    loadDefaultConfig();
    saveDeviceConfig(); // Save defaults for next time
    return;
  }
  
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file, using defaults");
    loadDefaultConfig();
    return;
  }
  
  DynamicJsonDocument doc(8192); // 8KB should be enough for our config
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.println("Failed to parse config JSON, using defaults");
    Serial.println("JSON Error: " + String(error.c_str()));
    loadDefaultConfig();
    return;
  }
  
  // Load basic settings
  deviceConfig.deviceName = doc["deviceName"] | "BattleAura Device";
  deviceConfig.deviceDescription = doc["deviceDescription"] | "Modular LED and Audio Controller";
  deviceConfig.defaultVolume = doc["defaultVolume"] | 20;
  deviceConfig.defaultBrightness = doc["defaultBrightness"] | 100;
  deviceConfig.idleTimeoutMs = doc["idleTimeoutMs"] | IDLE_TIMEOUT_MS;
  deviceConfig.enableSleep = doc["enableSleep"] | true;
  deviceConfig.hasLEDs = doc["hasLEDs"] | true;
  
  // Load pin configurations
  JsonArray pinsArray = doc["pins"];
  for (int i = 0; i < 11 && i < pinsArray.size(); i++) {
    JsonObject pin = pinsArray[i];
    deviceConfig.pins[i].pin = pin["pin"] | (D0 + i);
    deviceConfig.pins[i].type = pin["type"] | "unused";
    deviceConfig.pins[i].label = pin["label"] | "";
    deviceConfig.pins[i].enabled = pin["enabled"] | false;
    deviceConfig.pins[i].effectType = pin["effectType"] | "off";
    deviceConfig.pins[i].brightness = pin["brightness"] | 255;
    deviceConfig.pins[i].defaultR = pin["defaultR"] | 255;
    deviceConfig.pins[i].defaultG = pin["defaultG"] | 255;
    deviceConfig.pins[i].defaultB = pin["defaultB"] | 255;
  }
  
  // Load audio tracks
  JsonArray audioArray = doc["audioTracks"];
  for (int i = 0; i < 20 && i < audioArray.size(); i++) {
    JsonObject track = audioArray[i];
    deviceConfig.audioTracks[i].id = track["id"] | 0;
    deviceConfig.audioTracks[i].filename = track["filename"] | "";
    deviceConfig.audioTracks[i].label = track["label"] | "";
    deviceConfig.audioTracks[i].category = track["category"] | "other";
    deviceConfig.audioTracks[i].loops = track["loops"] | false;
    deviceConfig.audioTracks[i].volume = track["volume"] | 20;
    deviceConfig.audioTracks[i].enabled = track["enabled"] | false;
  }
  
  // Load trigger effects
  JsonArray effectsArray = doc["effects"];
  for (int i = 0; i < 12 && i < effectsArray.size(); i++) {
    JsonObject effect = effectsArray[i];
    deviceConfig.effects[i].effectId = effect["effectId"] | "";
    deviceConfig.effects[i].label = effect["label"] | "";
    deviceConfig.effects[i].description = effect["description"] | "";
    deviceConfig.effects[i].enabled = effect["enabled"] | false;
    deviceConfig.effects[i].primaryPin = effect["primaryPin"] | -1;
    deviceConfig.effects[i].secondaryPin = effect["secondaryPin"] | -1;
    deviceConfig.effects[i].audioTrack = effect["audioTrack"] | 0;
    deviceConfig.effects[i].effectPattern = effect["effectPattern"] | "flash";
    deviceConfig.effects[i].duration = effect["duration"] | 0;
    deviceConfig.effects[i].intensity = effect["intensity"] | 100;
  }
  
  // Count active items
  deviceConfig.activePins = 0;
  for (int i = 0; i < 11; i++) {
    if (deviceConfig.pins[i].enabled) deviceConfig.activePins++;
  }
  
  deviceConfig.activeAudioTracks = 0;
  for (int i = 0; i < 20; i++) {
    if (deviceConfig.audioTracks[i].enabled) deviceConfig.activeAudioTracks++;
  }
  
  deviceConfig.activeEffects = 0;
  for (int i = 0; i < 12; i++) {
    if (deviceConfig.effects[i].enabled) deviceConfig.activeEffects++;
  }
  
  Serial.println("Configuration loaded from SPIFFS");
  Serial.println("Device: " + deviceConfig.deviceName);
  Serial.println("Active pins: " + String(deviceConfig.activePins));
  Serial.println("Active audio tracks: " + String(deviceConfig.activeAudioTracks));
  Serial.println("Active effects: " + String(deviceConfig.activeEffects));
}

// Save device configuration to SPIFFS
void saveDeviceConfig() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed, cannot save configuration");
    return;
  }
  
  DynamicJsonDocument doc(8192); // 8KB JSON document
  
  // Save basic settings
  doc["deviceName"] = deviceConfig.deviceName;
  doc["deviceDescription"] = deviceConfig.deviceDescription;
  doc["defaultVolume"] = deviceConfig.defaultVolume;
  doc["defaultBrightness"] = deviceConfig.defaultBrightness;
  doc["idleTimeoutMs"] = deviceConfig.idleTimeoutMs;
  doc["enableSleep"] = deviceConfig.enableSleep;
  doc["hasLEDs"] = deviceConfig.hasLEDs;
  
  // Save pin configurations
  JsonArray pinsArray = doc.createNestedArray("pins");
  for (int i = 0; i < 11; i++) {
    JsonObject pin = pinsArray.createNestedObject();
    pin["pin"] = deviceConfig.pins[i].pin;
    pin["type"] = deviceConfig.pins[i].type;
    pin["label"] = deviceConfig.pins[i].label;
    pin["enabled"] = deviceConfig.pins[i].enabled;
    pin["effectType"] = deviceConfig.pins[i].effectType;
    pin["brightness"] = deviceConfig.pins[i].brightness;
    pin["defaultR"] = deviceConfig.pins[i].defaultR;
    pin["defaultG"] = deviceConfig.pins[i].defaultG;
    pin["defaultB"] = deviceConfig.pins[i].defaultB;
  }
  
  // Save audio tracks
  JsonArray audioArray = doc.createNestedArray("audioTracks");
  for (int i = 0; i < 20; i++) {
    JsonObject track = audioArray.createNestedObject();
    track["id"] = deviceConfig.audioTracks[i].id;
    track["filename"] = deviceConfig.audioTracks[i].filename;
    track["label"] = deviceConfig.audioTracks[i].label;
    track["category"] = deviceConfig.audioTracks[i].category;
    track["loops"] = deviceConfig.audioTracks[i].loops;
    track["volume"] = deviceConfig.audioTracks[i].volume;
    track["enabled"] = deviceConfig.audioTracks[i].enabled;
  }
  
  // Save trigger effects
  JsonArray effectsArray = doc.createNestedArray("effects");
  for (int i = 0; i < 12; i++) {
    JsonObject effect = effectsArray.createNestedObject();
    effect["effectId"] = deviceConfig.effects[i].effectId;
    effect["label"] = deviceConfig.effects[i].label;
    effect["description"] = deviceConfig.effects[i].description;
    effect["enabled"] = deviceConfig.effects[i].enabled;
    effect["primaryPin"] = deviceConfig.effects[i].primaryPin;
    effect["secondaryPin"] = deviceConfig.effects[i].secondaryPin;
    effect["audioTrack"] = deviceConfig.effects[i].audioTrack;
    effect["effectPattern"] = deviceConfig.effects[i].effectPattern;
    effect["duration"] = deviceConfig.effects[i].duration;
    effect["intensity"] = deviceConfig.effects[i].intensity;
  }
  
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }
  
  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Failed to write config to file");
  } else {
    Serial.println("Configuration saved to SPIFFS (" + String(configFile.size()) + " bytes)");
  }
  
  configFile.close();
}

// Helper functions for dynamic pin management
int getPinByLabel(const String& label) {
  for (int i = 0; i < 11; i++) {
    if (deviceConfig.pins[i].enabled && deviceConfig.pins[i].label == label) {
      return deviceConfig.pins[i].pin;
    }
  }
  return -1; // Not found
}

int getPinByType(const String& type) {
  for (int i = 0; i < 11; i++) {
    if (deviceConfig.pins[i].enabled && deviceConfig.pins[i].type == type) {
      return deviceConfig.pins[i].pin;
    }
  }
  return -1; // Not found
}

int getRGBPin() {
  return getPinByType("rgb");
}

bool isRGBPin(int pin) {
  for (int i = 0; i < 11; i++) {
    if (deviceConfig.pins[i].enabled && 
        deviceConfig.pins[i].pin == pin && 
        deviceConfig.pins[i].type == "rgb") {
      return true;
    }
  }
  return false;
}

TriggerEffect* getEffectById(const String& effectId) {
  for (int i = 0; i < 12; i++) {
    if (deviceConfig.effects[i].enabled && 
        deviceConfig.effects[i].effectId == effectId) {
      return &deviceConfig.effects[i];
    }
  }
  return nullptr;
}

// Run all enabled background effects based on configuration
void runBackgroundEffects() {
  for (int i = 0; i < 11; i++) {
    if (deviceConfig.pins[i].enabled) {
      runEffect(deviceConfig.pins[i].pin, deviceConfig.pins[i].effectType);
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
    // Check if this is an RGB pin dynamically
    if (isRGBPin(ledPin)) {
      consoleDataStreamRGB(ledPin);
    } else {
      consoleDataStream(ledPin);
    }
  } else if (effectType == "rgb_blue") {
    if (isRGBPin(ledPin)) {
      rgbStaticColor(ledPin, 0, 0, 255);
    } else {
      setLED(ledPin, 255); // Fallback to white for non-RGB pins
    }
  } else if (effectType == "rgb_red") {
    if (isRGBPin(ledPin)) {
      rgbStaticColor(ledPin, 255, 0, 0);
    } else {
      setLED(ledPin, 255); // Fallback to white for non-RGB pins
    }
  } else if (effectType == "rgb_green") {
    if (isRGBPin(ledPin)) {
      rgbStaticColor(ledPin, 0, 255, 0);
    } else {
      setLED(ledPin, 255); // Fallback to white for non-RGB pins
    }
  } else if (effectType == "rgb_pulse_blue") {
    if (isRGBPin(ledPin)) {
      rgbPulse(ledPin, 0, 0, 255);
    } else {
      enginePulseSmooth(ledPin, 0); // Fallback to regular pulse
    }
  } else if (effectType == "rgb_pulse_red") {
    if (isRGBPin(ledPin)) {
      rgbPulse(ledPin, 255, 0, 0);
    } else {
      enginePulseSmooth(ledPin, 0); // Fallback to regular pulse
    }
  } else if (effectType == "static") {
    setLED(ledPin, 255); // Full brightness static
  } else if (effectType == "off") {
    if (isRGBPin(ledPin)) {
      setRGB(ledPin, 0, 0, 0); // Turn off RGB
    } else {
      setLED(ledPin, 0); // Turn off regular LED
    }
  }
  // Add more effect types as needed
}

// Start a non-blocking weapon effect
void startWeaponEffect(int effectId, DFRobotDFPlayerMini* dfPlayer, int ledPin, int audioTrack, String effectType) {
  if (effectId < 0 || effectId >= 4) return;
  
  // Initialize the effect state
  weaponEffects[effectId].active = true;
  weaponEffects[effectId].startTime = millis();
  weaponEffects[effectId].lastUpdate = millis();
  weaponEffects[effectId].currentStep = 0;
  weaponEffects[effectId].ledPin = ledPin;
  weaponEffects[effectId].audioTrack = audioTrack;
  
  // Start the audio
  if (dfPlayer && dfPlayerConnected) {
    dfPlayer->stop();
    delay(100); // CRITICAL: DFPlayer stop/play transition - keep this one
    dfPlayer->play(audioTrack);
    dfPlayerPlaying = true;
    currentTrack = audioTrack;
  }
  
  Serial.println("Started non-blocking " + effectType + " effect on pin " + String(ledPin));
}

// Update all active weapon effects (call from main loop)
void updateWeaponEffects(DFRobotDFPlayerMini* dfPlayer) {
  for (int i = 0; i < 4; i++) {
    if (!weaponEffects[i].active) continue;
    
    WeaponEffectState& effect = weaponEffects[i];
    unsigned long now = millis();
    unsigned long elapsed = now - effect.startTime;
    
    // Generic weapon effect pattern: flash for duration of audio
    if (elapsed < 5000) { // 5 second max duration
      // Check if it's time for next flash
      if (now - effect.lastUpdate >= 130) { // 130ms cycle (50ms on, 80ms off)
        effect.lastUpdate = now;
        
        if (effect.currentStep % 2 == 0) {
          setLED(effect.ledPin, 255); // Flash on
        } else {
          setLED(effect.ledPin, 0);   // Flash off
        }
        effect.currentStep++;
      }
      
      // Check if audio is still playing
      if (dfPlayer && dfPlayer->available()) {
        // Process DFPlayer messages but don't block
        uint8_t type = dfPlayer->readType();
        int value = dfPlayer->read();
        if (type == DFPlayerPlayFinished && value == effect.audioTrack) {
          // This effect's audio finished
          effect.active = false;
          setLED(effect.ledPin, 0); // Ensure LED is off
          Serial.println("Weapon effect " + String(i) + " finished");
        }
      }
    } else {
      // Timeout - end the effect
      effect.active = false;
      setLED(effect.ledPin, 0);
      Serial.println("Weapon effect " + String(i) + " timed out");
    }
  }
}

// Start a non-blocking battle effect
void startBattleEffect(String effectType, DFRobotDFPlayerMini* dfPlayer, int audioTrack) {
  battleEffect.active = true;
  battleEffect.effectType = effectType;
  battleEffect.startTime = millis();
  battleEffect.lastUpdate = millis();
  battleEffect.currentStep = 0;
  battleEffect.currentCycle = 0;
  battleEffect.audioTrack = audioTrack;
  battleEffect.audioStarted = false;
  
  // Set max duration based on effect type
  if (effectType == "taking-hits") {
    battleEffect.maxDuration = 8000; // 8 seconds
  } else if (effectType == "destroyed") {
    battleEffect.maxDuration = 15000; // 15 seconds before final shutdown
  } else if (effectType == "rocket") {
    battleEffect.maxDuration = 6000; // 6 seconds
  } else if (effectType == "unit-kill") {
    battleEffect.maxDuration = 5000; // 5 seconds
  } else {
    battleEffect.maxDuration = 5000; // Default 5 seconds
  }
  
  Serial.println("Started non-blocking battle effect: " + effectType);
}

// Update battle effect (call from main loop)
void updateBattleEffect(DFRobotDFPlayerMini* dfPlayer) {
  if (!battleEffect.active) return;
  
  unsigned long now = millis();
  unsigned long elapsed = now - battleEffect.startTime;
  
  // Start audio if not already started
  if (!battleEffect.audioStarted && dfPlayer && dfPlayerConnected) {
    dfPlayer->stop();
    // CRITICAL: DFPlayer stop/play transition - this is essential for hardware
    delay(100);
    dfPlayer->play(battleEffect.audioTrack);
    dfPlayerPlaying = true;
    currentTrack = battleEffect.audioTrack;
    battleEffect.audioStarted = true;
    Serial.println("Battle effect audio started");
  }
  
  // Handle different battle effect types
  if (battleEffect.effectType == "taking-hits") {
    updateTakingHitsEffect(now, elapsed);
  } else if (battleEffect.effectType == "destroyed") {
    updateDestroyedEffect(now, elapsed);
  } else if (battleEffect.effectType == "rocket") {
    updateRocketEffect(now, elapsed);
  } else if (battleEffect.effectType == "unit-kill") {
    updateUnitKillEffect(now, elapsed);
  }
  
  // Check for timeout
  if (elapsed > battleEffect.maxDuration) {
    battleEffect.active = false;
    Serial.println("Battle effect finished: " + battleEffect.effectType);
  }
}

// Taking hits effect - non-blocking implementation
void updateTakingHitsEffect(unsigned long now, unsigned long elapsed) {
  // State machine with different phases
  if (elapsed < 2000) {
    // Phase 1: Console critical alerts (0-2s)
    if (now - battleEffect.lastUpdate >= 220) { // 80ms on + 60ms off + 80ms on + 60ms off + pause
      battleEffect.lastUpdate = now;
      
      int step = battleEffect.currentStep % 7;
      int consolePin = getRGBPin();
      if (consolePin != -1) {
        if (step == 0 || step == 2 || step == 4) {
          setRGB(consolePin, 255, 0, 0);  // Console RED alert
        } else {
          setRGB(consolePin, 0, 0, 0);    // Off
        }
      }
      
      if (step == 6) {
        battleEffect.currentCycle++;
        battleEffect.currentStep = 0;
      } else {
        battleEffect.currentStep++;
      }
    }
  } else if (elapsed < 4000) {
    // Phase 2: Power fluctuation (2-4s)
    if (now - battleEffect.lastUpdate >= 400) {
      battleEffect.lastUpdate = now;
      
      if (battleEffect.currentStep % 2 == 0) {
        globalBrightness = 30; // Dim to 30%
      } else {
        globalBrightness = deviceConfig.defaultBrightness; // Restore
      }
      battleEffect.currentStep++;
    }
  } else {
    // Phase 3: Engine strain (4s+)
    if (now - battleEffect.lastUpdate >= 400) {
      battleEffect.lastUpdate = now;
      
      // Find engine pins by labels
      int engine1Pin = getPinByLabel("Engine 1");
      int engine2Pin = getPinByLabel("Engine 2");
      
      if (battleEffect.currentStep % 2 == 0) {
        if (engine1Pin != -1) setLED(engine1Pin, 50);  // Engines dim
        if (engine2Pin != -1) setLED(engine2Pin, 50);
      } else {
        if (engine1Pin != -1) setLED(engine1Pin, 120); // Engines recover
        if (engine2Pin != -1) setLED(engine2Pin, 120);
      }
      battleEffect.currentStep++;
    }
  }
}

// Destroyed effect - non-blocking implementation
void updateDestroyedEffect(unsigned long now, unsigned long elapsed) {
  if (elapsed < 10000) {
    // Phase 1: System failure (0-10s)
    if (now - battleEffect.lastUpdate >= 200) {
      battleEffect.lastUpdate = now;
      
      // Console critical failure - random warning colors
      int consolePin = getRGBPin();
      if (consolePin != -1) {
        int r = random(100, 255);
        int g = random(0, 100);  // Mostly red/yellow warning colors
        setRGB(consolePin, r, g, 0);
      }
      
      // Candles dying - erratic flicker getting weaker over time
      int maxIntensity = 100 - (elapsed / 100); // Gradually dim
      int candle1Pin = getPinByLabel("Candle 1");
      int candle2Pin = getPinByLabel("Candle 2");
      int brazierPin = getPinByLabel("Brazier");
      
      if (candle1Pin != -1) setLED(candle1Pin, random(20, maxIntensity));
      if (candle2Pin != -1) setLED(candle2Pin, random(20, maxIntensity));
      if (brazierPin != -1) setLED(brazierPin, random(20, maxIntensity));
      
      // Engines violent death throes
      int engine1Pin = getPinByLabel("Engine 1");
      int engine2Pin = getPinByLabel("Engine 2");
      if (engine1Pin != -1) setLED(engine1Pin, random(0, 200));
      if (engine2Pin != -1) setLED(engine2Pin, random(0, 200));
    }
  } else {
    // Phase 2: Final shutdown (10s+)
    if (now - battleEffect.lastUpdate >= 200) {
      battleEffect.lastUpdate = now;
      
      int fade = 100 - ((elapsed - 10000) / 50); // Fade over 5 seconds
      if (fade > 0) {
        globalBrightness = fade;
      } else {
        // Complete shutdown
        globalBrightness = 0;
        battleEffect.active = false;
        
        // Turn off all LEDs explicitly
        for (int i = 0; i < 11; i++) {
          if (deviceConfig.pins[i].enabled) {
            if (deviceConfig.pins[i].type == "rgb") {
              setRGB(deviceConfig.pins[i].pin, 0, 0, 0);
            } else if (deviceConfig.pins[i].type == "led") {
              setLED(deviceConfig.pins[i].pin, 0);
            }
          }
        }
        
        Serial.println("FINAL SHUTDOWN: All systems offline");
        Serial.println("Unit destroyed. Entering deep sleep...");
        
        // Complete shutdown - device will need physical reset to wake
        ESP.deepSleep(0);
      }
    }
  }
}

// Rocket effect - non-blocking implementation
void updateRocketEffect(unsigned long now, unsigned long elapsed) {
  if (elapsed < 1000) {
    // Phase 1: Massive explosion backlight (0-1s)
    int candle1Pin = getPinByLabel("Candle 1");
    int candle2Pin = getPinByLabel("Candle 2");
    int brazierPin = getPinByLabel("Brazier");
    
    if (candle1Pin != -1) setLED(candle1Pin, 255);
    if (candle2Pin != -1) setLED(candle2Pin, 255);
    if (brazierPin != -1) setLED(brazierPin, 255);
  } else if (elapsed < 2500) {
    // Phase 2: Console overload flash (1-2.5s)
    if (now - battleEffect.lastUpdate >= 200) {
      battleEffect.lastUpdate = now;
      
      int consolePin = getRGBPin();
      if (consolePin != -1) {
        int step = battleEffect.currentStep % 4;
        if (step == 0) {
          setRGB(consolePin, 255, 255, 255);  // Bright white flash
        } else if (step == 2) {
          setRGB(consolePin, 200, 200, 255);  // Blue explosion glow
        } else {
          setRGB(consolePin, 0, 0, 0);
        }
      }
      battleEffect.currentStep++;
    }
  } else {
    // Phase 3: Candles settle to intense glow (2.5s+)
    if (now - battleEffect.lastUpdate >= 100) {
      battleEffect.lastUpdate = now;
      
      int candle1Pin = getPinByLabel("Candle 1");
      int candle2Pin = getPinByLabel("Candle 2");
      int brazierPin = getPinByLabel("Brazier");
      int consolePin = getRGBPin();
      
      if (candle1Pin != -1) setLED(candle1Pin, random(180, 220));
      if (candle2Pin != -1) setLED(candle2Pin, random(180, 220));
      if (brazierPin != -1) setLED(brazierPin, random(180, 220));
      if (consolePin != -1) setRGB(consolePin, random(100, 200), random(100, 200), random(100, 255));  // Random explosive colors
    }
  }
}

// Unit kill effect - non-blocking implementation
void updateUnitKillEffect(unsigned long now, unsigned long elapsed) {
  // Console success indicator - steady green glow
  int consolePin = getRGBPin();
  if (consolePin != -1) {
    setRGB(consolePin, 0, 200, 0);  // Green success
  }
  
  // Victory flash sequence on all systems
  if (now - battleEffect.lastUpdate >= 300) {
    battleEffect.lastUpdate = now;
    
    // Get all relevant pins
    int candle1Pin = getPinByLabel("Candle 1");
    int candle2Pin = getPinByLabel("Candle 2");
    int brazierPin = getPinByLabel("Brazier");
    int engine1Pin = getPinByLabel("Engine 1");
    int engine2Pin = getPinByLabel("Engine 2");
    
    if (battleEffect.currentStep % 2 == 0) {
      // Brief synchronized flash
      if (candle1Pin != -1) setLED(candle1Pin, 255);
      if (candle2Pin != -1) setLED(candle2Pin, 255);
      if (brazierPin != -1) setLED(brazierPin, 255);
      if (engine1Pin != -1) setLED(engine1Pin, 255);
      if (engine2Pin != -1) setLED(engine2Pin, 255);
    } else {
      // Brief dim
      if (candle1Pin != -1) setLED(candle1Pin, 50);
      if (candle2Pin != -1) setLED(candle2Pin, 50);
      if (brazierPin != -1) setLED(brazierPin, 50);
      if (engine1Pin != -1) setLED(engine1Pin, 50);
      if (engine2Pin != -1) setLED(engine2Pin, 50);
    }
    battleEffect.currentStep++;
  }
}