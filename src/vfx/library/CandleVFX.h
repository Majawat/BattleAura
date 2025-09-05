#pragma once

#include <Arduino.h>
#include "../BaseVFX.h"

namespace BattleAura {

class CandleEffect : public BaseEffect {
public:
    CandleEffect(LedController& ledController, Configuration& config);
    
    // BaseEffect implementation
    void begin() override;
    void update() override;
    void setEnabled(bool enabled) override;
    
private:
    
    // Per-zone flicker state
    struct FlickerState {
        uint32_t lastUpdate;
        float currentBrightness;    // Use float for smooth interpolation
        float baseBrightness;       // Base flickering level
        float flickerPhase;         // Phase for sine wave component
        float flickerSpeed;         // Random flicker speed
        uint32_t nextChange;        // When to change flicker pattern
    };
    
    std::vector<FlickerState> flickerStates;
    
    // Effect parameters - realistic candle flicker
    static const uint16_t UPDATE_INTERVAL = 20;    // Update every 20ms for smoothness
    static const uint8_t MIN_BRIGHTNESS = 40;      // Minimum candle brightness
    static const uint8_t BRIGHTNESS_VARIANCE = 80;  // Maximum flicker range
    
    void updateFlickerForZone(size_t zoneIndex, Zone* zone);
};

} // namespace BattleAura