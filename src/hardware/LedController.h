#pragma once

#include <Arduino.h>
#include "../config/ZoneConfig.h"
#include <vector>
#include <FastLED.h>

namespace BattleAura {

class LedController {
public:
    LedController();
    ~LedController();
    
    // Initialization
    bool begin();
    void addZone(const Zone& zone);
    void removeZone(uint8_t zoneId);
    
    // LED Control
    void setZoneBrightness(uint8_t zoneId, uint8_t brightness);
    void setZoneColor(uint8_t zoneId, CRGB color);
    void setZoneColorAndBrightness(uint8_t zoneId, CRGB color, uint8_t brightness);
    uint8_t getZoneBrightness(uint8_t zoneId) const;
    CRGB getZoneColor(uint8_t zoneId) const;
    
    // User brightness control (separate from effect brightness)
    void setUserBrightness(uint8_t zoneId, uint8_t brightness);
    uint8_t getUserBrightness(uint8_t zoneId) const;
    
    // Update hardware (apply changes)
    void update();
    
    // Utility
    bool isZoneConfigured(uint8_t zoneId) const;
    void printStatus() const;

private:
    struct ZoneState {
        Zone zone;
        uint8_t currentBrightness;
        uint8_t targetBrightness;
        uint8_t userBrightness;      // User-controlled brightness level (0-255)
        CRGB currentColor;
        CRGB targetColor;
        bool needsUpdate;
        uint8_t pwmChannel;          // PWM channel for PWM zones
        CRGB* leds;                  // FastLED array for WS2812B zones
        
        ZoneState(const Zone& z) : zone(z), currentBrightness(0), targetBrightness(0), 
                                   userBrightness(z.brightness), currentColor(CRGB::Black), 
                                   targetColor(CRGB::White), needsUpdate(false), 
                                   pwmChannel(255), leds(nullptr) {}
        
        // Copy constructor
        ZoneState(const ZoneState& other) : zone(other.zone), 
                                          currentBrightness(other.currentBrightness),
                                          targetBrightness(other.targetBrightness),
                                          userBrightness(other.userBrightness),
                                          currentColor(other.currentColor),
                                          targetColor(other.targetColor),
                                          needsUpdate(other.needsUpdate),
                                          pwmChannel(other.pwmChannel),
                                          leds(nullptr) {
            // Don't copy leds pointer - each instance manages its own memory
        }
        
        // Copy assignment
        ZoneState& operator=(const ZoneState& other) {
            if (this != &other) {
                zone = other.zone;
                currentBrightness = other.currentBrightness;
                targetBrightness = other.targetBrightness;
                userBrightness = other.userBrightness;
                currentColor = other.currentColor;
                targetColor = other.targetColor;
                needsUpdate = other.needsUpdate;
                pwmChannel = other.pwmChannel;
                // Don't copy leds pointer - each instance manages its own memory
                leds = nullptr;
            }
            return *this;
        }
        
        ~ZoneState() {
            if (leds) {
                delete[] leds;
                leds = nullptr;
            }
        }
    };
    
    std::vector<ZoneState> zones;
    
    // Hardware Management
    bool setupPWM(ZoneState& zoneState);
    bool setupWS2812B(ZoneState& zoneState);
    void updatePWM(uint8_t channel, uint8_t brightness, uint8_t maxBrightness);
    void updateWS2812B(ZoneState& zoneState);
    
    // Helper methods
    ZoneState* findZone(uint8_t zoneId);
    const ZoneState* findZone(uint8_t zoneId) const;
};

} // namespace BattleAura