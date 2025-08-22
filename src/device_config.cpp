#include <Arduino.h>
#include "config.h"
#include "effects.h"
#include "dfplayer.h"

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
    // Check if this is the RGB pin (D3)
    if (ledPin == D3) {
      consoleDataStreamRGB(ledPin);
    } else {
      consoleDataStream(ledPin);
    }
  } else if (effectType == "rgb_blue") {
    if (ledPin == D3) {
      rgbStaticColor(ledPin, 0, 0, 255);
    } else {
      setLED(ledPin, 255); // Fallback to white for non-RGB pins
    }
  } else if (effectType == "rgb_red") {
    if (ledPin == D3) {
      rgbStaticColor(ledPin, 255, 0, 0);
    } else {
      setLED(ledPin, 255); // Fallback to white for non-RGB pins
    }
  } else if (effectType == "rgb_green") {
    if (ledPin == D3) {
      rgbStaticColor(ledPin, 0, 255, 0);
    } else {
      setLED(ledPin, 255); // Fallback to white for non-RGB pins
    }
  } else if (effectType == "rgb_pulse_blue") {
    if (ledPin == D3) {
      rgbPulse(ledPin, 0, 0, 255);
    } else {
      enginePulseSmooth(ledPin, 0); // Fallback to regular pulse
    }
  } else if (effectType == "rgb_pulse_red") {
    if (ledPin == D3) {
      rgbPulse(ledPin, 255, 0, 0);
    } else {
      enginePulseSmooth(ledPin, 0); // Fallback to regular pulse
    }
  } else if (effectType == "static") {
    setLED(ledPin, 255); // Full brightness static
  } else if (effectType == "off") {
    if (ledPin == D3) {
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
      if (step == 0 || step == 2 || step == 4) {
        setRGB(LED4, 255, 0, 0);  // Console RED alert
      } else {
        setRGB(LED4, 0, 0, 0);    // Off
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
      
      if (battleEffect.currentStep % 2 == 0) {
        setLED(LED7, 50);  // Engines dim
        setLED(LED8, 50);
      } else {
        setLED(LED7, 120); // Engines recover
        setLED(LED8, 120);
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
      int r = random(100, 255);
      int g = random(0, 100);  // Mostly red/yellow warning colors
      setRGB(LED4, r, g, 0);
      
      // Candles dying - erratic flicker getting weaker over time
      int maxIntensity = 100 - (elapsed / 100); // Gradually dim
      for (int i = 0; i < 3; i++) {
        int candleLed = LED1 + i;
        int intensity = random(20, maxIntensity);
        setLED(candleLed, intensity);
      }
      
      // Engines violent death throes
      setLED(LED7, random(0, 200));
      setLED(LED8, random(0, 200));
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
        setLED(LED1, 0);
        setLED(LED2, 0);
        setLED(LED3, 0);
        setRGB(LED4, 0, 0, 0);  // Turn off RGB
        setLED(LED5, 0);
        setLED(LED6, 0);
        setLED(LED7, 0);
        setLED(LED8, 0);
        
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
    setLED(LED1, 255);
    setLED(LED2, 255); 
    setLED(LED3, 255);
  } else if (elapsed < 2500) {
    // Phase 2: Console overload flash (1-2.5s)
    if (now - battleEffect.lastUpdate >= 200) {
      battleEffect.lastUpdate = now;
      
      int step = battleEffect.currentStep % 4;
      if (step == 0) {
        setRGB(LED4, 255, 255, 255);  // Bright white flash
      } else if (step == 2) {
        setRGB(LED4, 200, 200, 255);  // Blue explosion glow
      } else {
        setRGB(LED4, 0, 0, 0);
      }
      battleEffect.currentStep++;
    }
  } else {
    // Phase 3: Candles settle to intense glow (2.5s+)
    if (now - battleEffect.lastUpdate >= 100) {
      battleEffect.lastUpdate = now;
      
      setLED(LED1, random(180, 220));
      setLED(LED2, random(180, 220));
      setLED(LED3, random(180, 220));
      setRGB(LED4, random(100, 200), random(100, 200), random(100, 255));  // Random explosive colors
    }
  }
}

// Unit kill effect - non-blocking implementation
void updateUnitKillEffect(unsigned long now, unsigned long elapsed) {
  // Console success indicator - steady green glow
  setRGB(LED4, 0, 200, 0);  // Green success
  
  // Victory flash sequence on all systems
  if (now - battleEffect.lastUpdate >= 300) {
    battleEffect.lastUpdate = now;
    
    if (battleEffect.currentStep % 2 == 0) {
      // Brief synchronized flash
      setLED(LED1, 255);
      setLED(LED2, 255);
      setLED(LED3, 255);
      setLED(LED7, 255);
      setLED(LED8, 255);
    } else {
      // Brief dim
      setLED(LED1, 50);
      setLED(LED2, 50);
      setLED(LED3, 50);
      setLED(LED7, 50);
      setLED(LED8, 50);
    }
    battleEffect.currentStep++;
  }
}