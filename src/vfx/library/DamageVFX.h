#pragma once

#include <Arduino.h>
#include "../BaseVFX.h"

namespace BattleAura {

class DamageVFX : public BaseVFX {
public:
    DamageVFX(LedController& ledController, Configuration& config);
    
    // BaseVFX implementation
    void begin() override;
    void update() override;
    void trigger(uint32_t duration = 2000) override; // 2s default duration
    
private:
    // Per-zone damage state
    struct DamageState {
        uint32_t damageStartTime;
        uint32_t lastFlicker;
        uint8_t originalBrightness;
        CRGB originalColor;
        bool hasOriginalState;
        float intensity;        // Damage intensity (1.0 = max)
    };
    
    std::vector<DamageState> damageStates;
    
    // Damage effect parameters
    static const uint16_t FLICKER_INTERVAL = 80;    // Fast damage flicker
    static const uint8_t DAMAGE_BRIGHTNESS = 200;   // Bright damage flash
    
    void updateDamageForZone(size_t zoneIndex, Zone* zone);
    void startDamage();
    void restoreZone(size_t zoneIndex, Zone* zone);
};

} // namespace BattleAura