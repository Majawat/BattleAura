#pragma once

#include <Arduino.h>
#include "../BaseVFX.h"

namespace BattleAura {

class EngineRevVFX : public BaseVFX {
public:
    EngineRevVFX(LedController& ledController, Configuration& config);
    
    // BaseVFX implementation
    void begin() override;
    void update() override;
    void trigger(uint32_t duration = 4000) override; // 4s default duration
    
private:
    // Per-zone engine rev state
    struct RevState {
        uint32_t revStartTime;
        uint8_t currentIntensity;
        uint8_t targetIntensity;
        uint32_t lastUpdate;
        bool isRevving;
        uint8_t revPhase;              // 0=ramp up, 1=peak, 2=ramp down
    };
    
    std::vector<RevState> revStates;
    
    // Engine rev parameters
    static const uint16_t UPDATE_INTERVAL = 50;        // 50ms update rate
    static const uint8_t MIN_INTENSITY = 80;           // Starting intensity
    static const uint8_t MAX_INTENSITY = 255;          // Peak intensity
    static const uint8_t RAMP_RATE = 8;                // Intensity change per update
    static const uint32_t RAMP_UP_TIME = 1500;         // 1.5s ramp up
    static const uint32_t PEAK_TIME = 1000;            // 1s at peak
    static const uint32_t RAMP_DOWN_TIME = 1500;       // 1.5s ramp down
    
    void updateRevForZone(size_t zoneIndex, Zone* zone);
    void startRevving();
    uint8_t calculateRevIntensity(uint32_t elapsed, uint8_t phase);
};

} // namespace BattleAura