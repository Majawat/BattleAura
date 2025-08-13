#ifndef DFPLAYER_H
#define DFPLAYER_H

#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

// External variables (defined in dfplayer.cpp)
extern bool dfPlayerConnected;
extern bool dfPlayerPlaying;
extern int currentTrack;
extern String dfPlayerStatus;
extern unsigned long lastStatusCheck;

// Function declaration
void printDetail(uint8_t type, int value);

#endif