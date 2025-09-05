#pragma once

#include <Arduino.h>
#include "../BaseVFX.h"

namespace BattleAura {

class EngineIdleEffect : public BaseEffect {
public:
    EngineIdleEffect(LedController& ledController, Configuration& config);
    
    // BaseEffect implementation
    void begin() override;
    void update() override;
    
private:
    // Per-zone engine idle state
    struct IdleState {
        uint32_t lastUpdate;
        float currentBrightness;
        float baseBrightness;
        float pulsePhase;
        float pulseSpeed;
        uint32_t nextVariation;
    };
    
    std::vector<IdleState> idleStates;
    
    // Engine idle parameters - steady pulse with subtle variation
    static const uint16_t UPDATE_INTERVAL = 30;     // Update every 30ms
    static const uint8_t BASE_BRIGHTNESS = 120;     // Base engine idle brightness
    static const uint8_t PULSE_AMPLITUDE = 40;      // Pulse range
    
    void updateIdleForZone(size_t zoneIndex, Zone* zone);
};

} // namespace BattleAura