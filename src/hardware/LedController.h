#pragma once

#include <Arduino.h>
#include "../config/ZoneConfig.h"
#include <vector>

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
    uint8_t getZoneBrightness(uint8_t zoneId) const;
    
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
        bool needsUpdate;
        uint8_t pwmChannel;  // PWM channel for this zone
        
        ZoneState(const Zone& z) : zone(z), currentBrightness(0), targetBrightness(0), needsUpdate(false), pwmChannel(255) {}
    };
    
    std::vector<ZoneState> zones;
    
    // PWM Management
    bool setupPWM(ZoneState& zoneState);
    void updatePWM(uint8_t channel, uint8_t brightness, uint8_t maxBrightness);
    
    // Helper methods
    ZoneState* findZone(uint8_t zoneId);
    const ZoneState* findZone(uint8_t zoneId) const;
};

} // namespace BattleAura