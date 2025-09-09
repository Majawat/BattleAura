#include "VictoryVFX.h"
#include "../../config/Configuration.h"

namespace BattleAura {

VictoryVFX::VictoryVFX(LedController& ledController, Configuration& config)
    : BaseVFX(ledController, config, "Victory", VFXPriority::ACTIVE) {
}

void VictoryVFX::begin() {
    Serial.println("VictoryVFX: Initializing");
    
    // Initialize victory states for all zones
    victoryStates.clear();
    victoryStates.resize(targetZones.size());
    
    for (auto& state : victoryStates) {
        state.victoryStartTime = 0;
        state.currentIntensity = 0;
        state.lastUpdate = 0;
        state.lastPulse = 0;
        state.isCelebrating = false;
        state.pulseCount = 0;
        state.celebrationPhase = 0;
    }
}

void VictoryVFX::update() {
    if (!enabled) {
        return;
    }
    
    uint32_t currentTime = millis();
    
    // Check if duration has expired
    if (shouldStop()) {
        Serial.println("VictoryVFX: Duration expired");
        stop();
        return;
    }
    
    // Update victory effect for each target zone
    for (size_t i = 0; i < targetZones.size(); i++) {
        Zone* zone = targetZones[i];
        if (zone && zone->enabled) {
            updateVictoryForZone(i, zone);
        }
    }
}

void VictoryVFX::trigger(uint32_t duration) {
    Serial.printf("VictoryVFX: Triggered with duration %dms\n", duration);
    
    BaseVFX::trigger(duration);
    
    startVictory();
}

void VictoryVFX::startVictory() {
    Serial.printf("VictoryVFX: Starting victory celebration on %d zones\n", targetZones.size());
    
    uint32_t currentTime = millis();
    
    // Initialize victory state for each zone
    for (size_t i = 0; i < victoryStates.size(); i++) {
        auto& state = victoryStates[i];
        state.victoryStartTime = currentTime;
        state.currentIntensity = 0;
        state.lastUpdate = currentTime;
        state.lastPulse = currentTime;
        state.isCelebrating = true;
        state.pulseCount = 0;
        state.celebrationPhase = 0; // Start with triumph pulses
    }
}

void VictoryVFX::updateVictoryForZone(size_t zoneIndex, Zone* zone) {
    if (zoneIndex >= victoryStates.size()) return;
    
    auto& state = victoryStates[zoneIndex];
    if (!state.isCelebrating) return;
    
    uint32_t currentTime = millis();
    
    // Update timing
    if (currentTime - state.lastUpdate < UPDATE_INTERVAL) {
        return; // Not time for update yet
    }
    
    state.lastUpdate = currentTime;
    uint32_t elapsed = currentTime - state.victoryStartTime;
    
    // Determine victory phase based on elapsed time
    if (elapsed <= TRIUMPH_PHASE_TIME) {
        state.celebrationPhase = 0; // Triumph pulses
        
        // Handle pulse timing
        if (currentTime - state.lastPulse >= PULSE_INTERVAL && 
            state.pulseCount < MAX_PULSES) {
            state.lastPulse = currentTime;
            state.pulseCount++;
            state.currentIntensity = MAX_INTENSITY; // Flash to max
        } else {
            // Fade pulse
            if (state.currentIntensity > 0) {
                state.currentIntensity = max(0, (int)state.currentIntensity - 25);
            }
        }
        
    } else if (elapsed <= TRIUMPH_PHASE_TIME + GLOW_PHASE_TIME) {
        state.celebrationPhase = 1; // Victory glow
        // Steady bright glow with slight pulsing
        float glowPulse = 0.85 + 0.15 * sin((elapsed - TRIUMPH_PHASE_TIME) * 0.008);
        state.currentIntensity = (uint8_t)(glowPulse * 200);
        
    } else if (elapsed <= TRIUMPH_PHASE_TIME + GLOW_PHASE_TIME + FADE_PHASE_TIME) {
        state.celebrationPhase = 2; // Fade phase
        uint32_t fadeElapsed = elapsed - TRIUMPH_PHASE_TIME - GLOW_PHASE_TIME;
        state.currentIntensity = map(fadeElapsed, 0, FADE_PHASE_TIME, 200, 0);
        
    } else {
        // Victory complete
        state.currentIntensity = 0;
        state.isCelebrating = false;
    }
    
    // Apply victory effect based on zone type
    if (zone->type == ZoneType::PWM) {
        // For PWM zones, directly control brightness
        ledController.setZoneBrightness(zone->id, state.currentIntensity);
    } else if (zone->type == ZoneType::WS2812B) {
        // For RGB zones, use victory colors
        CRGB victoryColor = getVictoryColor(state.celebrationPhase, state.currentIntensity);
        ledController.setZoneColor(zone->id, victoryColor);
    }
}

uint8_t VictoryVFX::calculateVictoryIntensity(uint32_t elapsed, uint8_t phase, uint8_t pulseCount) {
    switch (phase) {
        case 0: // Triumph pulses
            return (elapsed % PULSE_INTERVAL < 200) ? MAX_INTENSITY : 50;
        case 1: // Victory glow - steady with slight pulse
            return 180 + (uint8_t)(30 * sin(elapsed * 0.005));
        case 2: // Fade
            return map(elapsed - TRIUMPH_PHASE_TIME - GLOW_PHASE_TIME, 0, FADE_PHASE_TIME, 180, 0);
        default:
            return 0;
    }
}

CRGB VictoryVFX::getVictoryColor(uint8_t phase, uint8_t intensity) {
    CRGB color;
    
    switch (phase) {
        case 0: // Triumph pulses - bright gold/yellow
            color.r = intensity;
            color.g = (intensity * 180) / 255;
            color.b = 0;
            break;
        case 1: // Victory glow - warm golden
            color.r = intensity;
            color.g = (intensity * 160) / 255;
            color.b = (intensity * 20) / 255;
            break;
        case 2: // Fade - dim gold
            color.r = intensity;
            color.g = (intensity * 140) / 255;
            color.b = 0;
            break;
        default:
            color = CRGB::Black;
    }
    
    return color;
}

} // namespace BattleAura