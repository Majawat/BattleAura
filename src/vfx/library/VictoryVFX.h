#pragma once

#include <Arduino.h>
#include "../BaseVFX.h"

namespace BattleAura {

class VictoryVFX : public BaseVFX {
public:
    VictoryVFX(LedController& ledController, Configuration& config);
    
    // BaseVFX implementation
    void begin() override;
    void update() override;
    void trigger(uint32_t duration = 5000) override; // 5s default duration
    
private:
    // Per-zone victory state
    struct VictoryState {
        uint32_t victoryStartTime;
        uint8_t currentIntensity;
        uint32_t lastUpdate;
        uint32_t lastPulse;
        bool isCelebrating;
        uint8_t pulseCount;
        uint8_t celebrationPhase;      // 0=triumph pulses, 1=victory glow, 2=fade
    };
    
    std::vector<VictoryState> victoryStates;
    
    // Victory parameters
    static const uint16_t UPDATE_INTERVAL = 80;        // 80ms update rate
    static const uint16_t PULSE_INTERVAL = 400;        // 400ms between victory pulses
    static const uint8_t MAX_PULSES = 6;               // Number of triumph pulses
    static const uint8_t MAX_INTENSITY = 255;          // Peak intensity
    static const uint32_t TRIUMPH_PHASE_TIME = 3000;   // 3s of triumph pulses
    static const uint32_t GLOW_PHASE_TIME = 1500;      // 1.5s steady glow
    static const uint32_t FADE_PHASE_TIME = 500;       // 0.5s fade out
    
    void updateVictoryForZone(size_t zoneIndex, Zone* zone);
    void startVictory();
    uint8_t calculateVictoryIntensity(uint32_t elapsed, uint8_t phase, uint8_t pulseCount);
    CRGB getVictoryColor(uint8_t phase, uint8_t intensity);
};

} // namespace BattleAura