#include "effects.h"
#include "dfplayer.h"

// Global LED settings
bool ledsEnabled = true;
int globalBrightness = 100; // 0-100 percentage

// Helper function to set LED with global brightness and on/off control
void setLED(int ledPin, int brightness) {
  if (!ledsEnabled) {
    setLED(ledPin, 0);
    return;
  }
  
  // Apply global brightness percentage
  int adjustedBrightness = (brightness * globalBrightness) / 100;
  analogWrite(ledPin, constrain(adjustedBrightness, 0, 255));
}

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
  
  setLED(ledPin, brightness);
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
    setLED(ledPin, brightness);
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
  static unsigned long lastPulse = 0;
  static bool isOn = false;
  
  // Fast, irregular pulses like data scrolling
  if (millis() - lastPulse > random(100, 400)) {
    isOn = !isOn;
    int brightness = isOn ? random(80, 150) : random(10, 30);
    setLED(ledPin, brightness);
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
      setLED(ledPin, 255); // Muzzle flash on
      delay(50);
      setLED(ledPin, 0);   // Off
      delay(80);
      
      // Check if audio finished (this gets updated by the callback)
      if (dfPlayer->available()) {
        printDetail(dfPlayer->readType(), dfPlayer->read());
      }
    }
    
    // Ensure LED is off when done
    setLED(ledPin, 0);
    Serial.println("Machine gun effect finished");
  } else {
    // If no audio, do a few flashes as fallback
    Serial.println("No audio available, doing fallback LED flashes");
    for (int i = 0; i < 5; i++) {
      setLED(ledPin, 255);
      delay(50);
      setLED(ledPin, 0);
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
      setLED(ledPin, intensity);
      delay(random(80, 150)); // Vary the flicker timing
      
      // Check if audio finished (this gets updated by the callback)
      if (dfPlayer->available()) {
        printDetail(dfPlayer->readType(), dfPlayer->read());
      }
    }
    
    // Ensure LED is off when done
    setLED(ledPin, 0);
    Serial.println("Flamethrower effect finished");
  } else {
    // If no audio, do a sustained flame effect as fallback
    Serial.println("No audio available, doing fallback flame effect");
    for (int i = 0; i < 20; i++) {
      int intensity = random(180, 255);
      setLED(ledPin, intensity);
      delay(100);
    }
    setLED(ledPin, 0);
  }
}

// Engine rev effect - dual engine stack rev-up with configurable LEDs and audio
void engineRevEffect(DFRobotDFPlayerMini* dfPlayer, int engineLed1, int engineLed2, int audioTrack) {
  
  if (dfPlayerConnected && dfPlayer != nullptr) {
    // Stop current audio and play engine rev sound
    dfPlayer->stop();
    delay(100);
    dfPlayer->play(audioTrack);
    dfPlayerPlaying = true; // Mark as playing so callback knows to resume idle
    currentTrack = audioTrack;
    Serial.println("Playing engine rev audio with synchronized dual engine LED effects");
    
    // Engine rev effect - starts slow, builds up intensity
    while (dfPlayerPlaying && currentTrack == audioTrack) {
      // Create revving pattern - rapid alternating pulses that build intensity
      for (int cycle = 0; cycle < 3 && dfPlayerPlaying && currentTrack == audioTrack; cycle++) {
        // Engine 1 bright, Engine 2 dim
        setLED(engineLed1, random(200, 255));
        setLED(engineLed2, random(50, 100));
        delay(random(80, 150));
        
        // Engine 1 dim, Engine 2 bright  
        setLED(engineLed1, random(50, 100));
        setLED(engineLed2, random(200, 255));
        delay(random(80, 150));
        
        // Check if audio finished
        if (dfPlayer->available()) {
          printDetail(dfPlayer->readType(), dfPlayer->read());
        }
      }
    }
    
    // Ensure LEDs are off when done
    setLED(engineLed1, 0);
    setLED(engineLed2, 0);
    Serial.println("Engine rev effect finished");
  } else {
    // If no audio, do a rev effect as fallback
    Serial.println("No audio available, doing fallback engine rev effect");
    for (int i = 0; i < 15; i++) {
      // Alternating bright pulses
      setLED(engineLed1, random(180, 255));
      setLED(engineLed2, random(50, 120));
      delay(100);
      setLED(engineLed1, random(50, 120));
      setLED(engineLed2, random(180, 255));
      delay(100);
    }
    setLED(engineLed1, 0);
    setLED(engineLed2, 0);
  }
}