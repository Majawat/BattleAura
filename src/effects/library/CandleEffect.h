#pragma once

#include <Arduino.h>
#include "../../hardware/LedController.h"
#include "../../config/Configuration.h"

namespace BattleAura {

class CandleEffect {
public:
    CandleEffect(LedController& ledController, Configuration& config);
    
    // Effect control
    void begin();
    void update();
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled; }
    
private:
    LedController& ledController;
    Configuration& config;
    bool enabled;
    
    // Per-zone flicker state
    struct FlickerState {
        uint32_t lastUpdate;
        uint8_t currentBrightness;
        uint8_t targetBrightness;
        uint16_t flickerDelay;
        bool needsNewTarget;
    };
    
    std::vector<FlickerState> flickerStates;
    
    // Effect parameters
    static const uint16_t MIN_FLICKER_DELAY = 50;
    static const uint16_t MAX_FLICKER_DELAY = 300;
    static const uint8_t MIN_BRIGHTNESS = 20;
    static const uint8_t BRIGHTNESS_VARIANCE = 60;
    
    void updateFlickerForZone(size_t zoneIndex, Zone* zone);
    uint8_t generateFlickerBrightness(uint8_t maxBrightness);
};

} // namespace BattleAura