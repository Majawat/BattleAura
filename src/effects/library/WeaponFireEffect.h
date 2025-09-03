#pragma once

#include <Arduino.h>
#include "../BaseEffect.h"

namespace BattleAura {

class WeaponFireEffect : public BaseEffect {
public:
    WeaponFireEffect(LedController& ledController, Configuration& config);
    
    // BaseEffect implementation
    void begin() override;
    void update() override;
    void trigger(uint32_t duration = 1500) override; // 1.5s default duration
    
private:
    // Per-zone weapon fire state
    struct FireState {
        uint32_t fireStartTime;
        uint8_t flashPattern;      // Different flash patterns
        uint32_t lastFlash;
        uint8_t flashCount;
        bool isFlashing;
    };
    
    std::vector<FireState> fireStates;
    
    // Weapon fire parameters
    static const uint16_t FLASH_INTERVAL = 50;      // 50ms between flashes
    static const uint8_t MAX_FLASHES = 8;           // Up to 8 rapid flashes
    static const uint8_t FLASH_BRIGHTNESS = 255;    // Maximum brightness during flash
    
    void updateFireForZone(size_t zoneIndex, Zone* zone);
    void startFiring();
};

} // namespace BattleAura