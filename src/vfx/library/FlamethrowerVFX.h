#pragma once

#include <Arduino.h>
#include "../BaseVFX.h"

namespace BattleAura {

class FlamethrowerVFX : public BaseVFX {
public:
    FlamethrowerVFX(LedController& ledController, Configuration& config);
    
    // BaseVFX implementation
    void begin() override;
    void update() override;
    void trigger(uint32_t duration = 3000) override; // 3s default duration
    
private:
    // Per-zone flamethrower state
    struct FlameState {
        uint32_t flameStartTime;
        uint8_t baseIntensity;         // Base flame intensity
        uint32_t lastFlicker;
        uint8_t flickerPhase;          // Phase for flame flickering
        bool isFlaming;
    };
    
    std::vector<FlameState> flameStates;
    
    // Flamethrower parameters
    static const uint16_t FLICKER_INTERVAL = 30;       // 30ms flicker rate
    static const uint8_t MIN_INTENSITY = 180;          // Minimum flame intensity
    static const uint8_t MAX_INTENSITY = 255;          // Maximum flame intensity
    static const uint8_t INTENSITY_RANGE = 50;         // Range of intensity variation
    
    void updateFlameForZone(size_t zoneIndex, Zone* zone);
    void startFlaming();
    uint8_t calculateFlameIntensity(uint8_t phase);
};

} // namespace BattleAura