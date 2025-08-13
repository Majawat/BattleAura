#include "effects.h"
#include "dfplayer.h"

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

// Console data stream effect
void consoleDataStream(int ledPin) {
  static unsigned long lastPulse = 0;
  static bool isOn = false;
  
  // Fast, irregular pulses like data scrolling
  if (millis() - lastPulse > random(100, 400)) {
    isOn = !isOn;
    int brightness = isOn ? random(80, 150) : random(10, 30);
    analogWrite(ledPin, brightness);
    lastPulse = millis();
  }
}

// Machine gun effect - burst fire with configurable LED and audio
void machineGunEffect(DFRobotDFPlayerMini* dfPlayer, int ledPin, int audioTrack) {
  
  if (dfPlayerConnected && dfPlayer != nullptr) {
    // Stop current audio and play weapon fire sound
    dfPlayer->stop();
    delay(100);
    dfPlayer->play(audioTrack);
    dfPlayerPlaying = true; // Mark as playing so callback knows to resume idle
    currentTrack = audioTrack;
    Serial.println("Playing machine gun audio with synchronized LED flashing");
    
    // Keep flashing LED while weapon audio is playing
    while (dfPlayerPlaying && currentTrack == audioTrack) {
      analogWrite(ledPin, 255); // Muzzle flash on
      delay(50);
      analogWrite(ledPin, 0);   // Off
      delay(80);
      
      // Check if audio finished (this gets updated by the callback)
      if (dfPlayer->available()) {
        printDetail(dfPlayer->readType(), dfPlayer->read());
      }
    }
    
    // Ensure LED is off when done
    analogWrite(ledPin, 0);
    Serial.println("Machine gun effect finished");
  } else {
    // If no audio, do a few flashes as fallback
    Serial.println("No audio available, doing fallback LED flashes");
    for (int i = 0; i < 5; i++) {
      analogWrite(ledPin, 255);
      delay(50);
      analogWrite(ledPin, 0);
      delay(80);
    }
  }
}

// Flamethrower effect - sustained flame with configurable LED and audio
void flamethrowerEffect(DFRobotDFPlayerMini* dfPlayer, int ledPin, int audioTrack) {
  
  if (dfPlayerConnected && dfPlayer != nullptr) {
    // Stop current audio and play flamethrower sound
    dfPlayer->stop();
    delay(100);
    dfPlayer->play(audioTrack);
    dfPlayerPlaying = true; // Mark as playing so callback knows to resume idle
    currentTrack = audioTrack;
    Serial.println("Playing flamethrower audio with synchronized LED flame effect");
    
    // Keep flickering LED like a flame while audio is playing
    while (dfPlayerPlaying && currentTrack == audioTrack) {
      // Flickering flame effect - more realistic than steady glow
      int intensity = random(180, 255);
      analogWrite(ledPin, intensity);
      delay(random(80, 150)); // Vary the flicker timing
      
      // Check if audio finished (this gets updated by the callback)
      if (dfPlayer->available()) {
        printDetail(dfPlayer->readType(), dfPlayer->read());
      }
    }
    
    // Ensure LED is off when done
    analogWrite(ledPin, 0);
    Serial.println("Flamethrower effect finished");
  } else {
    // If no audio, do a sustained flame effect as fallback
    Serial.println("No audio available, doing fallback flame effect");
    for (int i = 0; i < 20; i++) {
      int intensity = random(180, 255);
      analogWrite(ledPin, intensity);
      delay(100);
    }
    analogWrite(ledPin, 0);
  }
}