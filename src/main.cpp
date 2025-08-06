#include "Arduino.h"
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"

SoftwareSerial softSerial(D7, D6); // RX=D7(GPIO20), TX=D6(GPIO21)
DFRobotDFPlayerMini myDFPlayer;

// LED pins for candle effect
#define LED1 D0 // GPIO2
#define LED2 D1 // GPIO3
#define audio_tank_idle 2

// Function declaration
void candleFlicker(int ledPin);
void printDetail(uint8_t type, int value);


void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("=== BattleAura ===");
  
  // Initialize LEDs
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  // Initialize DFPlayer
  softSerial.begin(9600);
  
  if (myDFPlayer.begin(softSerial, false, false)) {
    Serial.println("DFPlayer connected!");
    
    // Set volume and start looping
    myDFPlayer.volume(20); // Volume 0-30
    delay(500);
    myDFPlayer.loop(audio_tank_idle);
    
    Serial.println("Playing 0001.mp3 in loop with candle flicker...");
  } else {
    Serial.println("DFPlayer connection failed!");
  }
}

void loop() {
  // Candle flicker effect for both LEDs
  candleFlicker(LED1);
  candleFlicker(LED2);
  
  // Small delay for smooth flicker
  delay(50);
  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }

}

void candleFlicker(int ledPin) {
  // Generate random flicker values
  int baseIntensity = random(40, 80);  // Base candle glow (40-80 out of 255)
  int flicker = random(-20, 30);       // Random flicker amount
  int brightness = baseIntensity + flicker;
  
  // Keep within valid PWM range
  brightness = constrain(brightness, 5, 120); // Never fully off, never too bright
  
  analogWrite(ledPin, brightness);
}

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
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
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
