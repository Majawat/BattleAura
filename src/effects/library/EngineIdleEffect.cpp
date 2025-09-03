#include "EngineIdleEffect.h"
#include <FastLED.h>

namespace BattleAura {

EngineIdleEffect::EngineIdleEffect(LedController& ledController, Configuration& config) 
    : BaseEffect(ledController, config, "EngineIdle", EffectPriority::AMBIENT) {
}

void EngineIdleEffect::begin() {
    Serial.println("EngineIdle: Initializing...");
    
    // Initialize idle state for each zone
    auto zones = config.getAllZones();
    idleStates.clear();
    idleStates.resize(zones.size());
    
    for (size_t i = 0; i < zones.size(); i++) {
        IdleState& state = idleStates[i];
        state.lastUpdate = millis();
        state.currentBrightness = BASE_BRIGHTNESS;
        state.baseBrightness = BASE_BRIGHTNESS + random(-20, 20); // Vary base slightly
        state.pulsePhase = random(0, 628) / 100.0;  // Random starting phase (0-2π)
        state.pulseSpeed = random(80, 120) / 100.0; // Vary pulse speed
        state.nextVariation = millis() + random(2000, 5000); // Variation every 2-5s
    }
    
    Serial.printf("EngineIdle: Initialized for %d zones\n", zones.size());
}

void EngineIdleEffect::update() {
    if (!enabled) return;
    
    auto zones = config.getAllZones();
    
    // Ensure we have idle states for all zones
    if (idleStates.size() != zones.size()) {
        begin(); // Reinitialize if zone count changed
    }
    
    for (size_t i = 0; i < zones.size(); i++) {
        if (i < idleStates.size()) {
            updateIdleForZone(i, zones[i]);
        }
    }
}

void EngineIdleEffect::updateIdleForZone(size_t zoneIndex, Zone* zone) {
    if (!zone || !zone->enabled) return;
    
    IdleState& state = idleStates[zoneIndex];
    uint32_t currentTime = millis();
    
    // Only update every UPDATE_INTERVAL ms
    if (currentTime - state.lastUpdate < UPDATE_INTERVAL) {
        return;
    }
    
    float deltaTime = (currentTime - state.lastUpdate) / 1000.0; // Time in seconds
    
    // Update pulse phase
    state.pulsePhase += deltaTime * state.pulseSpeed * 2.0; // Slow steady pulse
    
    // Generate smooth engine idle pulse
    float pulseWave = sin(state.pulsePhase) * 0.5 + 0.5; // 0-1 range
    float targetBrightness = state.baseBrightness + (pulseWave * PULSE_AMPLITUDE);
    
    // Clamp to valid range
    float maxBrightness = (float)zone->brightness;
    targetBrightness = max(0.0f, min(maxBrightness, targetBrightness));
    
    // Smooth interpolation
    float smoothing = 0.2;
    state.currentBrightness = state.currentBrightness * (1.0 - smoothing) + targetBrightness * smoothing;
    
    // Occasional variation in base brightness
    if (currentTime >= state.nextVariation) {
        state.baseBrightness = BASE_BRIGHTNESS + random(-30, 30);
        state.pulseSpeed = random(60, 140) / 100.0;
        state.nextVariation = currentTime + random(3000, 8000);
    }
    
    // Apply to LED controller - adapt to zone type
    uint8_t brightness = (uint8_t)round(state.currentBrightness);
    
    if (zone->type == ZoneType::PWM) {
        // PWM zones: steady glow with subtle pulse
        ledController.setZoneBrightness(zone->id, brightness);
    } else if (zone->type == ZoneType::WS2812B) {
        // RGB zones: blue engine glow with brightness variation
        uint8_t red = 50;
        uint8_t green = 100;
        uint8_t blue = 255;
        
        CRGB engineColor = CRGB(red, green, blue);
        ledController.setZoneColorAndBrightness(zone->id, engineColor, brightness);
    }
    
    state.lastUpdate = currentTime;
}

} // namespace BattleAura