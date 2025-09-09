#pragma once

#include <Arduino.h>
#include "../BaseVFX.h"

namespace BattleAura {

class DestroyedVFX : public BaseVFX {
public:
    DestroyedVFX(LedController& ledController, Configuration& config);
    
    // BaseVFX implementation
    void begin() override;
    void update() override;
    void trigger(uint32_t duration = 8000) override; // 8s default duration
    
private:
    // Per-zone destruction state
    struct DestructionState {
        uint32_t destructionStartTime;
        uint8_t currentIntensity;
        uint32_t lastUpdate;
        uint32_t lastExplosion;
        bool isDestroyed;
        uint8_t destructionPhase;      // 0=explosions, 1=fire, 2=sparks, 3=fade
        uint8_t explosionCount;
    };
    
    std::vector<DestructionState> destructionStates;
    
    // Destruction parameters
    static const uint16_t UPDATE_INTERVAL = 100;       // 100ms update rate
    static const uint16_t EXPLOSION_INTERVAL = 300;    // 300ms between explosions
    static const uint8_t MAX_EXPLOSIONS = 5;           // Number of initial explosions
    static const uint8_t MAX_INTENSITY = 255;          // Peak explosion intensity
    static const uint32_t EXPLOSION_PHASE_TIME = 2000; // 2s of explosions
    static const uint32_t FIRE_PHASE_TIME = 3000;      // 3s of fire
    static const uint32_t SPARK_PHASE_TIME = 2000;     // 2s of sparks
    static const uint32_t FADE_PHASE_TIME = 1000;      // 1s fade out
    
    void updateDestructionForZone(size_t zoneIndex, Zone* zone);
    void startDestruction();
    uint8_t calculateExplosionIntensity(uint32_t elapsed, uint8_t phase);
    CRGB getDestructionColor(uint8_t phase, uint8_t intensity);
};

} // namespace BattleAura