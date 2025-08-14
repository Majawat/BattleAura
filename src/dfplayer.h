#ifndef DFPLAYER_H
#define DFPLAYER_H

#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

// External variables (defined in dfplayer.cpp)
extern bool dfPlayerConnected;
extern bool dfPlayerPlaying;
extern int currentTrack;
extern int currentVolume;
extern String dfPlayerStatus;
extern unsigned long lastStatusCheck;
extern unsigned long lastActivity;
extern bool idleAudioActive;

// Function declarations
void printDetail(uint8_t type, int value);
void setResumeIdleCallback(void (*callback)());
void triggerActivity();
void checkIdleTimeout();

#endif