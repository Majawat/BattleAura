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
        setRGB(LED4, 255, 0, 0);  // Console RED alert
        delay(80);
        setRGB(LED4, 0, 0, 0);    // Off
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
        setRGB(LED4, 255, 0, 0);  // Red warning
        delay(80);
        setRGB(LED4, 0, 0, 0);
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
      
      // Console critical failure - random warning colors
      int r = random(100, 255);
      int g = random(0, 100);  // Mostly red/yellow warning colors
      setRGB(LED4, r, g, 0);
      delay(random(50, 150));
      setRGB(LED4, 0, 0, 0);
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
    setRGB(LED4, 0, 0, 0);  // Turn off RGB
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
      setRGB(LED4, random(0, 150), random(0, 100), 0);  // Dying RGB console
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
      
      // Console overload flash - bright white/blue explosion effect
      setRGB(LED4, 255, 255, 255);  // Bright white flash
      delay(100);
      setRGB(LED4, 0, 0, 0);
      delay(100);
      setRGB(LED4, 200, 200, 255);  // Blue explosion glow
      delay(100);
      setRGB(LED4, 0, 0, 0);
      
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
    setRGB(LED4, 255, 255, 255);  // Bright white explosion
    delay(300);
    
    // Intense flickering
    for (int i = 0; i < 10; i++) {
      setLED(LED1, random(150, 255));
      setLED(LED2, random(150, 255));
      setLED(LED3, random(150, 255));
      setRGB(LED4, random(100, 200), random(100, 200), random(100, 255));  // Random explosive colors
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
      
      // Console success indicator - steady green glow
      setRGB(LED4, 0, 200, 0);  // Green success
      
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
      setRGB(LED4, 0, 255, 0);  // Green victory
      setLED(LED7, 255);
      setLED(LED8, 255);
      delay(200);
      
      setLED(LED1, 0);
      setLED(LED2, 0);
      setLED(LED3, 0);
      setRGB(LED4, 0, 0, 0);  // Turn off RGB
      setLED(LED7, 0);
      setLED(LED8, 0);
      delay(200);
    }
  }
}