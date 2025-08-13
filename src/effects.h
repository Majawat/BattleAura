#ifndef EFFECTS_H
#define EFFECTS_H

#include "Arduino.h"

// LED pin definitions
#define LED1 D0 // Candle fiber optics 1
#define LED2 D1 // Candle fiber optics 2
#define LED3 D2 // Brazier
#define LED4 D3 // Console screen
#define LED5 D4 // Weapon 1 - machine gun
#define LED6 D5 // Weapon 2 - flamethrower
#define LED7 D8 // Engine stack 1
#define LED8 D9 // Engine stack 2
#define LED9 D10 // Unused currently

// Effect function declarations
void candleFlicker(int ledPin);
void enginePulse(int ledPin, int minBright, int maxBright, int speed);
void enginePulseSmooth(int ledPin, int phase);
void engineHeat(int ledPin);
void consoleDataStream(int ledPin);

// Weapon effects
void machineGunEffect(class DFRobotDFPlayerMini* dfPlayer, int ledPin, int audioTrack);
void flamethrowerEffect(class DFRobotDFPlayerMini* dfPlayer, int ledPin, int audioTrack);

#endif