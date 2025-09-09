#include "FlamethrowerVFX.h"
#include "../../config/Configuration.h"

namespace BattleAura {

FlamethrowerVFX::FlamethrowerVFX(LedController& ledController, Configuration& config)
    : BaseVFX(ledController, config, "Flamethrower", VFXPriority::ACTIVE) {
}

void FlamethrowerVFX::begin() {
    Serial.println("FlamethrowerVFX: Initializing");
    
    // Initialize flame states for all zones
    flameStates.clear();
    flameStates.resize(targetZones.size());
    
    for (auto& state : flameStates) {
        state.flameStartTime = 0;
        state.baseIntensity = MIN_INTENSITY;
        state.lastFlicker = 0;
        state.flickerPhase = 0;
        state.isFlaming = false;
    }
}

void FlamethrowerVFX::update() {
    if (!enabled) {
        return;
    }
    
    uint32_t currentTime = millis();
    
    // Check if duration has expired
    if (shouldStop()) {
        Serial.println("FlamethrowerVFX: Duration expired");
        stop();
        return;
    }
    
    // Update flame effect for each target zone
    for (size_t i = 0; i < targetZones.size(); i++) {
        Zone* zone = targetZones[i];
        if (zone && zone->enabled) {
            updateFlameForZone(i, zone);
        }
    }
}

void FlamethrowerVFX::trigger(uint32_t duration) {
    Serial.printf("FlamethrowerVFX: Triggered with duration %dms\n", duration);
    
    // BaseVFX::trigger handles the timing
    BaseVFX::trigger(duration);
    
    startFlaming();
}

void FlamethrowerVFX::startFlaming() {
    Serial.printf("FlamethrowerVFX: Starting flame effect on %d zones\n", targetZones.size());
    
    uint32_t currentTime = millis();
    
    // Initialize flame state for each zone
    for (size_t i = 0; i < flameStates.size(); i++) {
        auto& state = flameStates[i];
        state.flameStartTime = currentTime;
        state.baseIntensity = MIN_INTENSITY;
        state.lastFlicker = currentTime;
        state.flickerPhase = random(0, 100);  // Random starting phase for variety
        state.isFlaming = true;
    }
}

void FlamethrowerVFX::updateFlameForZone(size_t zoneIndex, Zone* zone) {
    if (zoneIndex >= flameStates.size()) return;
    
    auto& state = flameStates[zoneIndex];
    if (!state.isFlaming) return;
    
    uint32_t currentTime = millis();
    
    // Update flicker timing
    if (currentTime - state.lastFlicker >= FLICKER_INTERVAL) {
        state.lastFlicker = currentTime;
        state.flickerPhase = (state.flickerPhase + 1) % 100;
    }
    
    // Calculate flame intensity with flickering
    uint8_t flameIntensity = calculateFlameIntensity(state.flickerPhase);
    
    // Apply flame intensity based on zone type
    if (zone->type == ZoneType::PWM) {
        // For PWM zones, directly control brightness
        ledController.setZoneBrightness(zone->id, flameIntensity);
    } else if (zone->type == ZoneType::WS2812B) {
        // For RGB zones, create orange/red flame colors with intensity
        CRGB flameColor;
        flameColor.r = flameIntensity;
        flameColor.g = (flameIntensity * 60) / 255;  // Orange tint
        flameColor.b = 0;  // No blue for flame
        
        ledController.setZoneColor(zone->id, flameColor);
    }
}

uint8_t FlamethrowerVFX::calculateFlameIntensity(uint8_t phase) {
    // Create realistic flame flickering using sine wave with noise
    float baseWave = sin(phase * 0.1) * 0.3 + 0.7;  // Base wave between 0.4-1.0
    
    // Add some random variation for flame irregularity
    uint8_t randomNoise = random(0, 20) - 10;  // -10 to +10
    
    // Calculate final intensity
    float intensity = baseWave * MAX_INTENSITY + randomNoise;
    
    // Clamp to valid range
    intensity = constrain(intensity, MIN_INTENSITY, MAX_INTENSITY);
    
    return (uint8_t)intensity;
}

} // namespace BattleAura