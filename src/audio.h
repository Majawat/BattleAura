#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "config.h"

enum class AudioFile {
  NONE = 0,
  TANK_IDLE = 1,
  TANK_IDLE_2 = 2,
  MACHINE_GUN = 3,
  FLAMETHROWER = 4,
  TAKING_HITS = 5,
  ENGINE_REVVING = 6,
  EXPLOSION = 7,
  ROCKET_LAUNCHER = 8,
  KILL_CONFIRMED = 9
};

class AudioManager {
public:
  static AudioManager& getInstance();
  bool begin();
  void update();
  void playFile(AudioFile file, bool loop = false);
  void playFile(uint8_t fileNumber, bool loop = false);
  void stopPlaying();
  void setVolume(uint8_t volume);
  void pausePlayback();
  void resumePlayback();
  bool isPlaying();
  bool isAvailable();
  
private:
  AudioManager() = default;
  SoftwareSerial* dfSerial;
  DFRobotDFPlayerMini dfPlayer;
  bool initialized;
  bool currentlyPlaying;
  uint8_t currentVolume;
  uint32_t lastStatusCheck;
  
  void printError();
  bool checkConnection();
};