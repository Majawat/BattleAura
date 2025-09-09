#include "EngineRevVFX.h"
#include "../../config/Configuration.h"

namespace BattleAura {

EngineRevVFX::EngineRevVFX(LedController& ledController, Configuration& config)
    : BaseVFX(ledController, config, "EngineRev", VFXPriority::ACTIVE) {
}

void EngineRevVFX::begin() {
    Serial.println("EngineRevVFX: Initializing");
    
    // Initialize rev states for all zones
    revStates.clear();
    revStates.resize(targetZones.size());
    
    for (auto& state : revStates) {
        state.revStartTime = 0;
        state.currentIntensity = MIN_INTENSITY;
        state.targetIntensity = MIN_INTENSITY;
        state.lastUpdate = 0;
        state.isRevving = false;
        state.revPhase = 0;
    }
}

void EngineRevVFX::update() {
    if (!enabled) {
        return;
    }
    
    uint32_t currentTime = millis();
    
    // Check if duration has expired
    if (shouldStop()) {
        Serial.println("EngineRevVFX: Duration expired");
        stop();
        return;
    }
    
    // Update rev effect for each target zone
    for (size_t i = 0; i < targetZones.size(); i++) {
        Zone* zone = targetZones[i];
        if (zone && zone->enabled) {
            updateRevForZone(i, zone);
        }
    }
}

void EngineRevVFX::trigger(uint32_t duration) {
    Serial.printf("EngineRevVFX: Triggered with duration %dms\n", duration);
    
    BaseVFX::trigger(duration);
    
    startRevving();
}

void EngineRevVFX::startRevving() {
    Serial.printf("EngineRevVFX: Starting engine rev effect on %d zones\n", targetZones.size());
    
    uint32_t currentTime = millis();
    
    // Initialize rev state for each zone
    for (size_t i = 0; i < revStates.size(); i++) {
        auto& state = revStates[i];
        state.revStartTime = currentTime;
        state.currentIntensity = MIN_INTENSITY;
        state.targetIntensity = MIN_INTENSITY;
        state.lastUpdate = currentTime;
        state.isRevving = true;
        state.revPhase = 0; // Start with ramp up
    }
}

void EngineRevVFX::updateRevForZone(size_t zoneIndex, Zone* zone) {
    if (zoneIndex >= revStates.size()) return;
    
    auto& state = revStates[zoneIndex];
    if (!state.isRevving) return;
    
    uint32_t currentTime = millis();
    
    // Update timing
    if (currentTime - state.lastUpdate < UPDATE_INTERVAL) {
        return; // Not time for update yet
    }
    
    state.lastUpdate = currentTime;
    uint32_t elapsed = currentTime - state.revStartTime;
    
    // Determine rev phase based on elapsed time
    if (elapsed <= RAMP_UP_TIME) {
        state.revPhase = 0; // Ramp up
        state.targetIntensity = map(elapsed, 0, RAMP_UP_TIME, MIN_INTENSITY, MAX_INTENSITY);
    } else if (elapsed <= RAMP_UP_TIME + PEAK_TIME) {
        state.revPhase = 1; // Peak
        state.targetIntensity = MAX_INTENSITY;
    } else if (elapsed <= RAMP_UP_TIME + PEAK_TIME + RAMP_DOWN_TIME) {
        state.revPhase = 2; // Ramp down
        uint32_t rampDownElapsed = elapsed - RAMP_UP_TIME - PEAK_TIME;
        state.targetIntensity = map(rampDownElapsed, 0, RAMP_DOWN_TIME, MAX_INTENSITY, MIN_INTENSITY);
    } else {
        // Rev complete
        state.targetIntensity = MIN_INTENSITY;
        state.isRevving = false;
    }
    
    // Smoothly transition current intensity toward target
    if (state.currentIntensity < state.targetIntensity) {
        state.currentIntensity = min((int)state.currentIntensity + RAMP_RATE, (int)state.targetIntensity);
    } else if (state.currentIntensity > state.targetIntensity) {
        state.currentIntensity = max((int)state.currentIntensity - RAMP_RATE, (int)state.targetIntensity);
    }
    
    // Apply intensity based on zone type
    if (zone->type == ZoneType::PWM) {
        // For PWM zones, directly control brightness
        ledController.setZoneBrightness(zone->id, state.currentIntensity);
    } else if (zone->type == ZoneType::WS2812B) {
        // For RGB zones, create engine glow colors (blue/white)
        CRGB engineColor;
        engineColor.r = (state.currentIntensity * 180) / 255;  // Warm white
        engineColor.g = (state.currentIntensity * 200) / 255;  
        engineColor.b = state.currentIntensity;                // Blue engine glow
        
        ledController.setZoneColor(zone->id, engineColor);
    }
}

uint8_t EngineRevVFX::calculateRevIntensity(uint32_t elapsed, uint8_t phase) {
    switch (phase) {
        case 0: // Ramp up
            return map(elapsed, 0, RAMP_UP_TIME, MIN_INTENSITY, MAX_INTENSITY);
        case 1: // Peak
            return MAX_INTENSITY;
        case 2: // Ramp down
            return map(elapsed - RAMP_UP_TIME - PEAK_TIME, 0, RAMP_DOWN_TIME, MAX_INTENSITY, MIN_INTENSITY);
        default:
            return MIN_INTENSITY;
    }
}

} // namespace BattleAura