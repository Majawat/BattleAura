#include "dfplayer.h"

// DFPlayer state variables
bool dfPlayerConnected = false;
bool dfPlayerPlaying = false;
int currentTrack = 0;
String dfPlayerStatus = "Unknown";
unsigned long lastStatusCheck = 0;

// Callback for resuming idle audio after effects
void (*resumeIdleCallback)() = nullptr;

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println(F("USB Inserted!"));
      break;
    case DFPlayerUSBRemoved:
      Serial.println(F("USB Removed!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      dfPlayerPlaying = false;
      dfPlayerStatus = "Play Finished";
      
      // If this was a weapon effect (not idle), resume idle audio
      if (value == 3 || value == 4) { // AUDIO_WEAPON_FIRE_1 or AUDIO_WEAPON_FIRE_2
        Serial.println(F("Weapon effect finished, resuming idle audio"));
        if (resumeIdleCallback != nullptr) {
          resumeIdleCallback();
        }
      }
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  
}

// Set the callback function for resuming idle audio
void setResumeIdleCallback(void (*callback)()) {
  resumeIdleCallback = callback;
}
