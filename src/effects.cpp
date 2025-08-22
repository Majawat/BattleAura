#include "effects.h"
#include "dfplayer.h"
#include "config.h"

// For RGB LED support (WS2812/addressable LEDs)
#include <Adafruit_NeoPixel.h>

// RGB LED strip object for D3 console
Adafruit_NeoPixel rgbStrip(RGB_LED_COUNT, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

// Global LED settings
bool ledsEnabled = true;
int globalBrightness = 100; // Will be set from device config

// Helper function to set LED with global brightness and on/off control
void setLED(int ledPin, int brightness) {
  if (!ledsEnabled) {
    analogWrite(ledPin, 0);
    return;
  }
  
  // Apply global brightness percentage
  int adjustedBrightness = (brightness * globalBrightness) / 100;
  analogWrite(ledPin, constrain(adjustedBrightness, 0, 255));
}

// Helper function to set RGB LED with global brightness and on/off control
void setRGB(int rgbPin, uint8_t r, uint8_t g, uint8_t b) {
  if (!ledsEnabled) {
    rgbStrip.setPixelColor(0, 0, 0, 0);
    rgbStrip.show();
    return;
  }
  
  // Apply global brightness percentage
  r = (r * globalBrightness) / 100;
  g = (g * globalBrightness) / 100;
  b = (b * globalBrightness) / 100;
  
  rgbStrip.setPixelColor(0, r, g, b);
  rgbStrip.show();
}

// Candle flicker effect
void candleFlicker(int ledPin) {
  // Each pin gets its own timing (up to 8 pins)
  static unsigned long lastBaseUpdate[8] = {0};
  static int baseIntensity[8] = {60, 60, 60, 60, 60, 60, 60, 60};
  static unsigned long nextUpdateTime[8] = {0};
  
  // Convert pin to index (D0=0, D1=1, D2=2, D3=3, D4=4, D5=5, D8=6, D9=7)
  int pinIndex = (ledPin <= D5) ? ledPin - D0 : ledPin - D8 + 6;
  if (pinIndex < 0 || pinIndex >= 8) return; // Safety check
  
  // Update base intensity at random intervals
  if (millis() >= nextUpdateTime[pinIndex]) {
    baseIntensity[pinIndex] = random(40, 80);
    nextUpdateTime[pinIndex] = millis() + random(200, 500);
  }
  
  // Add flicker every call
  int flicker = random(-20, 30);
  int brightness = constrain(baseIntensity[pinIndex] + flicker, 5, 120);
  
  setLED(ledPin, brightness);
}

// Engine breathing effect (original - unused now)
void enginePulse(int ledPin, int minBright, int maxBright, int speed) {
  // Each pin gets its own state
  static unsigned long lastUpdate[8] = {0};
  static int brightness[8] = {60, 60, 60, 60, 60, 60, 60, 60};
  static int direction[8] = {1, 1, 1, 1, 1, 1, 1, 1};

  // Convert pin to index
  int pinIndex = (ledPin <= D5) ? ledPin - D0 : ledPin - D8 + 6;
  if (pinIndex < 0 || pinIndex >= 8) return;

  if (millis() - lastUpdate[pinIndex] > speed) {
    lastUpdate[pinIndex] = millis();
    brightness[pinIndex] += direction[pinIndex];
    if (brightness[pinIndex] >= maxBright || brightness[pinIndex] <= minBright) {
      direction[pinIndex] = -direction[pinIndex];
    }
    setLED(ledPin, brightness[pinIndex]);
  }
}

// Smooth engine pulse using sine wave
void enginePulseSmooth(int ledPin, int phase) {
  // Use sine wave for smooth breathing
  float angle = (millis() + phase) * 0.003;  // Slow breathing
  int brightness = 40 + 80 * (sin(angle) + 1) / 2;  // Range 40-120
  setLED(ledPin, brightness);
}

// Engine heat flicker effect
void engineHeat(int ledPin) {
  static unsigned long lastUpdate = 0;
  static int baseHeat = 80;
  
  // Slow heat base changes
  if (millis() - lastUpdate > random(300, 800)) {
    baseHeat = random(60, 120);
    lastUpdate = millis();
  }
  
  // Fast flicker on top of base heat
  int flicker = random(-15, 20);
  int brightness = constrain(baseHeat + flicker, 30, 150);
  setLED(ledPin, brightness);
}

// Console data stream effect
void consoleDataStream(int ledPin) {
  // Each pin gets its own state
  static unsigned long lastPulse[8] = {0};
  static bool isOn[8] = {false};
  static unsigned long nextPulseTime[8] = {0};
  
  // Convert pin to index
  int pinIndex = (ledPin <= D5) ? ledPin - D0 : ledPin - D8 + 6;
  if (pinIndex < 0 || pinIndex >= 8) return;
  
  // Fast, irregular pulses like data scrolling
  if (millis() >= nextPulseTime[pinIndex]) {
    isOn[pinIndex] = !isOn[pinIndex];
    int brightness = isOn[pinIndex] ? random(80, 150) : random(10, 30);
    setLED(ledPin, brightness);
    nextPulseTime[pinIndex] = millis() + random(100, 400);
  }
}

// RGB Console data stream effect - colorful data scrolling
void consoleDataStreamRGB(int rgbPin) {
  static unsigned long nextPulseTime = 0;
  static int currentMode = 0;
  
  // Fast, irregular pulses with different colors for different data types
  if (millis() >= nextPulseTime) {
    int mode = random(0, 4);
    
    switch (mode) {
      case 0: // System data - blue
        setRGB(rgbPin, 0, random(50, 100), random(100, 200));
        break;
      case 1: // Warning data - yellow/orange
        setRGB(rgbPin, random(150, 255), random(100, 150), 0);
        break;
      case 2: // Tactical data - green
        setRGB(rgbPin, 0, random(100, 200), random(0, 50));
        break;
      case 3: // Low activity - dim white
        setRGB(rgbPin, random(20, 60), random(20, 60), random(20, 60));
        break;
    }
    
    nextPulseTime = millis() + random(100, 500);
  }
}

// Static RGB color
void rgbStaticColor(int rgbPin, uint8_t r, uint8_t g, uint8_t b) {
  setRGB(rgbPin, r, g, b);
}

// RGB Flash effect
void rgbFlash(int rgbPin, uint8_t r, uint8_t g, uint8_t b, int duration) {
  static unsigned long flashStart = 0;
  static bool flashActive = false;
  
  if (!flashActive) {
    flashStart = millis();
    flashActive = true;
    setRGB(rgbPin, r, g, b);
  } else if (millis() - flashStart >= duration) {
    flashActive = false;
    setRGB(rgbPin, 0, 0, 0);
  }
}

// RGB Pulse effect - breathing
void rgbPulse(int rgbPin, uint8_t r, uint8_t g, uint8_t b) {
  // Use sine wave for smooth breathing
  float angle = millis() * 0.003;  // Slow breathing
  float intensity = (sin(angle) + 1) / 2;  // Range 0-1
  
  uint8_t pulsedR = r * intensity;
  uint8_t pulsedG = g * intensity;
  uint8_t pulsedB = b * intensity;
  
  setRGB(rgbPin, pulsedR, pulsedG, pulsedB);
}

// Machine gun effect - burst fire with configurable LED and audio (NON-BLOCKING)
void machineGunEffect(DFRobotDFPlayerMini* dfPlayer, int ledPin, int audioTrack) {
  // This function is now deprecated - use startWeaponEffect() instead for non-blocking operation
  // Keeping for backward compatibility, but calling the non-blocking version
  Serial.println("Machine gun effect triggered - using non-blocking weapon effect system");
  startWeaponEffect(0, dfPlayer, ledPin, audioTrack, "machine-gun");
}

// Flamethrower effect - sustained flame with configurable LED and audio (NON-BLOCKING)
void flamethrowerEffect(DFRobotDFPlayerMini* dfPlayer, int ledPin, int audioTrack) {
  // This function is now deprecated - use startWeaponEffect() instead for non-blocking operation
  // Keeping for backward compatibility, but calling the non-blocking version
  Serial.println("Flamethrower effect triggered - using non-blocking weapon effect system");
  startWeaponEffect(1, dfPlayer, ledPin, audioTrack, "flamethrower");
}

// Engine rev effect - dual engine stack rev-up with configurable LEDs and audio (NON-BLOCKING)
void engineRevEffect(DFRobotDFPlayerMini* dfPlayer, int engineLed1, int engineLed2, int audioTrack) {
  // This function is now deprecated - use startWeaponEffect() instead for non-blocking operation
  // For now, use the primary engine LED for the weapon effect system
  Serial.println("Engine rev effect triggered - using non-blocking weapon effect system");
  startWeaponEffect(2, dfPlayer, engineLed1, audioTrack, "engine-rev");
  
  // TODO: Enhance weapon effect system to handle dual LED effects properly
  // For now, the secondary LED will be handled by the main weapon effect system
}

// Taking hits effect - damage alerts and power fluctuation (NON-BLOCKING)
void takingHitsEffect(DFRobotDFPlayerMini* dfPlayer, int audioTrack) {
  Serial.println("Taking hits effect triggered - using non-blocking battle effect system");
  triggerActivity();
  startBattleEffect("taking-hits", dfPlayer, audioTrack);
}

// Destroyed effect - complete system failure and shutdown (NON-BLOCKING)
void destroyedEffect(DFRobotDFPlayerMini* dfPlayer, int audioTrack) {
  Serial.println("CRITICAL: Unit destroyed - initiating shutdown sequence");
  triggerActivity();
  startBattleEffect("destroyed", dfPlayer, audioTrack);
  // Note: This effect will eventually call ESP.deepSleep(0) from the non-blocking system
}

// Rocket effect - explosive backlight and system strain (NON-BLOCKING)
void rocketEffect(DFRobotDFPlayerMini* dfPlayer, int audioTrack) {
  Serial.println("ROCKET FIRED - explosive backlight effect");
  triggerActivity();
  startBattleEffect("rocket", dfPlayer, audioTrack);
}

// Unit kill effect - victory celebration (NON-BLOCKING)
void unitKillEffect(DFRobotDFPlayerMini* dfPlayer, int audioTrack) {
  Serial.println("ENEMY ELIMINATED - victory sequence");
  triggerActivity();
  startBattleEffect("unit-kill", dfPlayer, audioTrack);
}