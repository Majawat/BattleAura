#ifndef EFFECTS_H
#define EFFECTS_H

#include "Arduino.h"
#include <Adafruit_NeoPixel.h>

// RGB LED support for addressable LEDs (WS2812) 
#define RGB_LED_PIN D3 // Default RGB pin (will be dynamic in future)
#define RGB_LED_COUNT 1 // Maximum RGB LEDs supported per pin

// Global LED settings
extern bool ledsEnabled;
extern int globalBrightness; // 0-100 percentage

// RGB LED strip object
extern Adafruit_NeoPixel rgbStrip;

// Effect function declarations
void candleFlicker(int ledPin);
void enginePulse(int ledPin, int minBright, int maxBright, int speed);
void enginePulseSmooth(int ledPin, int phase);
void engineHeat(int ledPin);
void consoleDataStream(int ledPin);

// RGB LED effect functions
void consoleDataStreamRGB(int rgbPin);
void rgbStaticColor(int rgbPin, uint8_t r, uint8_t g, uint8_t b);
void rgbFlash(int rgbPin, uint8_t r, uint8_t g, uint8_t b, int duration);
void rgbPulse(int rgbPin, uint8_t r, uint8_t g, uint8_t b);

// Utility functions
void setLED(int ledPin, int brightness);
void setRGB(int rgbPin, uint8_t r, uint8_t g, uint8_t b);

// Weapon effects
void machineGunEffect(class DFRobotDFPlayerMini* dfPlayer, int ledPin, int audioTrack);
void flamethrowerEffect(class DFRobotDFPlayerMini* dfPlayer, int ledPin, int audioTrack);

// Vehicle effects
void engineRevEffect(class DFRobotDFPlayerMini* dfPlayer, int engineLed1, int engineLed2, int audioTrack);

// Battle effects
void takingHitsEffect(class DFRobotDFPlayerMini* dfPlayer, int audioTrack);
void destroyedEffect(class DFRobotDFPlayerMini* dfPlayer, int audioTrack);
void rocketEffect(class DFRobotDFPlayerMini* dfPlayer, int audioTrack);
void unitKillEffect(class DFRobotDFPlayerMini* dfPlayer, int audioTrack);

#endif