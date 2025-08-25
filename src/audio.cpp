#include "audio.h"

AudioManager& AudioManager::getInstance() {
  static AudioManager instance;
  return instance;
}

bool AudioManager::begin() {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  if (!config.audioEnabled) {
    Serial.println("Audio disabled in config");
    return true;
  }
  
  dfSerial = new SoftwareSerial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
  dfSerial->begin(9600);
  
  Serial.println("Initializing DFPlayer...");
  
  if (!dfPlayer.begin(*dfSerial, true, false)) {
    Serial.println("Unable to begin DFPlayer:");
    printError();
    initialized = false;
    return false;
  }
  
  initialized = true;
  currentlyPlaying = false;
  currentVolume = config.volume;
  lastStatusCheck = 0;
  
  dfPlayer.volume(currentVolume);
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  
  delay(1000);
  
  Serial.println("DFPlayer Mini online");
  Serial.printf("Volume set to: %d\n", currentVolume);
  
  return true;
}

void AudioManager::update() {
  if (!initialized) return;
  
  uint32_t currentTime = millis();
  if (currentTime - lastStatusCheck > 1000) {
    lastStatusCheck = currentTime;
    
    if (dfPlayer.available()) {
      uint8_t type = dfPlayer.readType();
      int value = dfPlayer.read();
      
      switch (type) {
        case DFPlayerPlayFinished:
          Serial.printf("Finished playing track %d\n", value);
          currentlyPlaying = false;
          break;
        case DFPlayerError:
          Serial.printf("DFPlayer Error: %d\n", value);
          printError();
          break;
        default:
          break;
      }
    }
  }
}

void AudioManager::playFile(AudioFile file, bool loop) {
  playFile(static_cast<uint8_t>(file), loop);
}

void AudioManager::playFile(uint8_t fileNumber, bool loop) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  if (!initialized || !config.audioEnabled || fileNumber == 0) {
    return;
  }
  
  Serial.printf("Playing audio file: %d (loop: %s)\n", fileNumber, loop ? "yes" : "no");
  
  if (loop) {
    dfPlayer.loop(fileNumber);
  } else {
    dfPlayer.play(fileNumber);
  }
  
  currentlyPlaying = true;
  delay(100);
}

void AudioManager::stopPlaying() {
  if (!initialized) return;
  
  Serial.println("Stopping audio playback");
  dfPlayer.stop();
  currentlyPlaying = false;
}

void AudioManager::setVolume(uint8_t volume) {
  if (!initialized) return;
  
  currentVolume = constrain(volume, 0, 30);
  dfPlayer.volume(currentVolume);
  
  ConfigManager& configMgr = ConfigManager::getInstance();
  configMgr.setVolume(currentVolume);
  
  Serial.printf("Volume set to: %d\n", currentVolume);
}

void AudioManager::pausePlayback() {
  if (!initialized) return;
  
  dfPlayer.pause();
  Serial.println("Audio playback paused");
}

void AudioManager::resumePlayback() {
  if (!initialized) return;
  
  dfPlayer.start();
  Serial.println("Audio playback resumed");
}

bool AudioManager::isPlaying() {
  return currentlyPlaying;
}

bool AudioManager::isAvailable() {
  return initialized;
}

void AudioManager::printError() {
  if (!dfPlayer.available()) return;
  
  uint8_t type = dfPlayer.readType();
  int value = dfPlayer.read();
  
  switch (value) {
    case Busy:
      Serial.println("Card not found");
      break;
    case Sleeping:
      Serial.println("Sleeping");
      break;
    case SerialWrongStack:
      Serial.println("Get Wrong Stack");
      break;
    case CheckSumNotMatch:
      Serial.println("Check Sum Not Match");
      break;
    case FileIndexOut:
      Serial.println("File Index Out of Bound");
      break;
    case FileMismatch:
      Serial.println("Cannot Find File");
      break;
    case Advertise:
      Serial.println("In Advertise");
      break;
    default:
      Serial.printf("Unknown error: %d\n", value);
      break;
  }
}

bool AudioManager::checkConnection() {
  if (!initialized) return false;
  
  return dfPlayer.available();
}