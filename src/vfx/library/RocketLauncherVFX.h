#pragma once

#include <Arduino.h>
#include "../BaseVFX.h"

namespace BattleAura {

class RocketLauncherVFX : public BaseVFX {
public:
    RocketLauncherVFX(LedController& ledController, Configuration& config);
    
    // BaseVFX implementation
    void begin() override;
    void update() override;
    void trigger(uint32_t duration = 2000) override; // 2s default duration
    
private:
    // Per-zone rocket launch state
    struct LaunchState {
        uint32_t launchStartTime;
        uint8_t currentIntensity;
        uint32_t lastUpdate;
        bool isLaunching;
        uint8_t launchPhase;           // 0=charge, 1=flash, 2=afterglow, 3=fade
    };
    
    std::vector<LaunchState> launchStates;
    
    // Rocket launcher parameters
    static const uint16_t UPDATE_INTERVAL = 50;        // 50ms update rate
    static const uint8_t MAX_INTENSITY = 255;          // Peak flash intensity
    static const uint32_t CHARGE_TIME = 300;           // 300ms charge up
    static const uint32_t FLASH_TIME = 200;            // 200ms bright flash
    static const uint32_t AFTERGLOW_TIME = 800;        // 800ms afterglow
    static const uint32_t FADE_TIME = 700;             // 700ms fade out
    
    void updateLaunchForZone(size_t zoneIndex, Zone* zone);
    void startLaunching();
    uint8_t calculateLaunchIntensity(uint32_t elapsed, uint8_t phase);
    CRGB getLaunchColor(uint8_t phase, uint8_t intensity);
};

} // namespace BattleAura