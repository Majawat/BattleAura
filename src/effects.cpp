#include "effects.h"

// Candle flicker effect
void candleFlicker(int ledPin) {
  static unsigned long lastBaseUpdate = 0;
  static int baseIntensity = 60;  // Starting base
  
  // Update base intensity every 200-500ms (randomly)
  if (millis() - lastBaseUpdate > random(200, 500)) {
    baseIntensity = random(40, 80);
    lastBaseUpdate = millis();
  }
  
  // Add flicker every call
  int flicker = random(-20, 30);
  int brightness = constrain(baseIntensity + flicker, 5, 120);
  
  analogWrite(ledPin, brightness);
}

// Engine breathing effect (original - unused now)
void enginePulse(int ledPin, int minBright, int maxBright, int speed) {
  static unsigned long lastUpdate = 0;
  static int brightness = minBright;
  static int direction = 1;

  if (millis() - lastUpdate > speed) {
    lastUpdate = millis();
    brightness += direction;
    if (brightness >= maxBright || brightness <= minBright) {
      direction = -direction;
    }
    analogWrite(ledPin, brightness);
  }
}

// Smooth engine pulse using sine wave
void enginePulseSmooth(int ledPin, int phase) {
  // Use sine wave for smooth breathing
  float angle = (millis() + phase) * 0.003;  // Slow breathing
  int brightness = 40 + 80 * (sin(angle) + 1) / 2;  // Range 40-120
  analogWrite(ledPin, brightness);
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
  analogWrite(ledPin, brightness);
}