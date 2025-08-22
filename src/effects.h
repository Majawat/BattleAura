#ifndef EFFECTS_H
#define EFFECTS_H

#include "Arduino.h"
#include <Adafruit_NeoPixel.h>

// LED pin definitions
#define LED1 D0 // Candle fiber optics 1
#define LED2 D1 // Candle fiber optics 2
#define LED3 D2 // Brazier
#define LED4 D3 // Console screen (single pin - now will be RGB)
#define LED5 D4 // Weapon 1 - machine gun
#define LED6 D5 // Weapon 2 - flamethrower
#define LED7 D8 // Engine stack 1
#define LED8 D9 // Engine stack 2
#define LED9 D10 // Unused currently

// RGB LED support for D3 console screen
// Note: ESP32 supports RGB LEDs via multiple PWM channels on single pin
// For addressable RGB (WS2812), we use the pin as data line
#define RGB_LED_PIN D3 // Same as LED4, but for RGB operations
#define RGB_LED_COUNT 1 // Number of RGB LEDs on D3

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