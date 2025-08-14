#include "effects.h"
#include "dfplayer.h"

#include "config.h"

// Global LED settings
bool ledsEnabled = true;
int globalBrightness = 100; // Will be set from device config

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

// Taking hits effect - damage alerts and power fluctuation
void takingHitsEffect(DFRobotDFPlayerMini* dfPlayer, int audioTrack) {
  
  if (dfPlayerConnected && dfPlayer != nullptr) {
    // Stop current audio and play taking hits sound
    dfPlayer->stop();
    delay(100);
    dfPlayer->play(audioTrack);
    dfPlayerPlaying = true;
    currentTrack = audioTrack;
    Serial.println("Playing taking hits audio with damage effect visuals");
    
    // Store original states to restore later
    bool originalLedsEnabled = ledsEnabled;
    
    // Damage effect sequence while audio plays
    unsigned long effectStart = millis();
    while (dfPlayerPlaying && currentTrack == audioTrack && (millis() - effectStart < 8000)) {
      
      // Console critical alerts - rapid red warning flashes
      for (int flash = 0; flash < 3; flash++) {
        setLED(LED4, 255);  // Console full brightness
        delay(80);
        setLED(LED4, 0);    // Off
        delay(60);
      }
      
      // Power fluctuation - dim all candles briefly
      int originalBrightness = globalBrightness;
      globalBrightness = 30; // Dim to 30%
      delay(200);
      globalBrightness = originalBrightness; // Restore
      
      // Engine strain - brief stutter in engine pulse
      setLED(LED7, 50);  // Engines dim
      setLED(LED8, 50);
      delay(100);
      
      // Check if audio finished
      if (dfPlayer->available()) {
        printDetail(dfPlayer->readType(), dfPlayer->read());
      }
      
      delay(300); // Pause between damage bursts
    }
    
    // Restore normal operations
    Serial.println("Taking hits effect finished - systems recovering");
  } else {
    // Fallback visual effect if no audio
    Serial.println("No audio available, doing fallback damage effect");
    for (int cycle = 0; cycle < 5; cycle++) {
      // Console warning flashes
      for (int i = 0; i < 3; i++) {
        setLED(LED4, 255);
        delay(80);
        setLED(LED4, 0);
        delay(60);
      }
      delay(400);
    }
  }
}

// Destroyed effect - complete system failure and shutdown
void destroyedEffect(DFRobotDFPlayerMini* dfPlayer, int audioTrack) {
  
  if (dfPlayerConnected && dfPlayer != nullptr) {
    // Stop current audio and play destruction sound
    dfPlayer->stop();
    delay(100);
    dfPlayer->play(audioTrack);
    dfPlayerPlaying = true;
    currentTrack = audioTrack;
    Serial.println("CRITICAL: Unit destroyed - initiating shutdown sequence");
    
    // Death sequence - all systems failing
    unsigned long effectStart = millis();
    while (dfPlayerPlaying && currentTrack == audioTrack && (millis() - effectStart < 10000)) {
      
      // Console critical failure
      setLED(LED4, random(100, 255));
      delay(random(50, 150));
      setLED(LED4, 0);
      delay(random(100, 300));
      
      // Candles dying - erratic flicker getting weaker
      for (int i = 0; i < 3; i++) {
        int candleLed = LED1 + i;
        int intensity = random(20, 100); // Getting dimmer over time
        setLED(candleLed, intensity);
        delay(random(80, 200));
        setLED(candleLed, 0);
        delay(random(50, 150));
      }
      
      // Engines violent death throes
      setLED(LED7, random(0, 200));
      setLED(LED8, random(0, 200));
      delay(random(100, 250));
      
      // Check if audio finished
      if (dfPlayer->available()) {
        printDetail(dfPlayer->readType(), dfPlayer->read());
      }
    }
    
    // Final shutdown sequence - everything dies
    Serial.println("FINAL SHUTDOWN: All systems offline");
    for (int fade = 100; fade >= 0; fade -= 10) {
      globalBrightness = fade;
      delay(200);
    }
    
    // Turn off all LEDs explicitly
    setLED(LED1, 0);
    setLED(LED2, 0);
    setLED(LED3, 0);
    setLED(LED4, 0);
    setLED(LED5, 0);
    setLED(LED6, 0);
    setLED(LED7, 0);
    setLED(LED8, 0);
    
    // Stop all audio before shutdown
    dfPlayer->stop();
    delay(500);
    
    Serial.println("Unit destroyed. Entering deep sleep...");
    delay(1000);
    
    // Complete shutdown - device will need physical reset to wake
    ESP.deepSleep(0);
    
  } else {
    // Fallback without audio
    Serial.println("No audio - simulating destruction sequence");
    for (int cycle = 10; cycle >= 0; cycle--) {
      globalBrightness = cycle * 10;
      // Flicker all LEDs erratically
      setLED(LED1, random(0, 100));
      setLED(LED2, random(0, 100));
      setLED(LED3, random(0, 100));
      setLED(LED4, random(0, 150));
      setLED(LED7, random(0, 100));
      setLED(LED8, random(0, 100));
      delay(300);
    }
    
    // Final shutdown
    ESP.deepSleep(0);
  }
}

// Rocket effect - explosive backlight and system strain  
void rocketEffect(DFRobotDFPlayerMini* dfPlayer, int audioTrack) {
  
  if (dfPlayerConnected && dfPlayer != nullptr) {
    // Stop current audio and play rocket sound
    dfPlayer->stop();
    delay(100);
    dfPlayer->play(audioTrack);
    dfPlayerPlaying = true;
    currentTrack = audioTrack;
    Serial.println("ROCKET FIRED - explosive backlight effect");
    
    // Rocket effect while audio plays
    while (dfPlayerPlaying && currentTrack == audioTrack) {
      
      // Massive explosion backlight - all candles flare intensely
      setLED(LED1, 255);
      setLED(LED2, 255); 
      setLED(LED3, 255);
      delay(200);
      
      // Console overload flash
      setLED(LED4, 255);
      delay(100);
      setLED(LED4, 0);
      delay(100);
      setLED(LED4, 255);
      delay(100);
      setLED(LED4, 0);
      
      // Candles settle to intense glow
      setLED(LED1, random(180, 220));
      setLED(LED2, random(180, 220));
      setLED(LED3, random(180, 220));
      delay(300);
      
      // Check if audio finished
      if (dfPlayer->available()) {
        printDetail(dfPlayer->readType(), dfPlayer->read());
      }
    }
    
    Serial.println("Rocket effect finished");
  } else {
    // Fallback effect
    Serial.println("No audio - doing fallback rocket explosion effect");
    // Massive flash
    setLED(LED1, 255);
    setLED(LED2, 255);
    setLED(LED3, 255);
    setLED(LED4, 255);
    delay(300);
    
    // Intense flickering
    for (int i = 0; i < 10; i++) {
      setLED(LED1, random(150, 255));
      setLED(LED2, random(150, 255));
      setLED(LED3, random(150, 255));
      setLED(LED4, random(100, 200));
      delay(100);
    }
  }
}

// Unit kill effect - victory celebration
void unitKillEffect(DFRobotDFPlayerMini* dfPlayer, int audioTrack) {
  
  if (dfPlayerConnected && dfPlayer != nullptr) {
    // Stop current audio and play victory sound
    dfPlayer->stop();
    delay(100);
    dfPlayer->play(audioTrack);
    dfPlayerPlaying = true;
    currentTrack = audioTrack;
    Serial.println("ENEMY ELIMINATED - victory sequence");
    
    // Victory effect while audio plays
    while (dfPlayerPlaying && currentTrack == audioTrack) {
      
      // Console success indicator - steady bright glow
      setLED(LED4, 200);
      
      // Victory flash sequence on all systems
      for (int flash = 0; flash < 2; flash++) {
        // Brief synchronized flash
        setLED(LED1, 255);
        setLED(LED2, 255);
        setLED(LED3, 255);
        setLED(LED7, 255);
        setLED(LED8, 255);
        delay(150);
        
        // Brief dim
        setLED(LED1, 50);
        setLED(LED2, 50);
        setLED(LED3, 50);
        setLED(LED7, 50);
        setLED(LED8, 50);
        delay(150);
      }
      
      // Check if audio finished
      if (dfPlayer->available()) {
        printDetail(dfPlayer->readType(), dfPlayer->read());
      }
      
      delay(500);
    }
    
    Serial.println("Victory effect finished");
  } else {
    // Fallback effect
    Serial.println("No audio - doing fallback victory effect");
    for (int cycle = 0; cycle < 3; cycle++) {
      // Synchronized victory flashes
      setLED(LED1, 255);
      setLED(LED2, 255);
      setLED(LED3, 255);
      setLED(LED4, 255);
      setLED(LED7, 255);
      setLED(LED8, 255);
      delay(200);
      
      setLED(LED1, 0);
      setLED(LED2, 0);
      setLED(LED3, 0);
      setLED(LED4, 0);
      setLED(LED7, 0);
      setLED(LED8, 0);
      delay(200);
    }
  }
}