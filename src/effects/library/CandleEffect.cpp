#include "CandleEffect.h"
#include <FastLED.h>

namespace BattleAura {

CandleEffect::CandleEffect(LedController& ledController, Configuration& config) 
    : ledController(ledController), config(config), enabled(false) {
}

void CandleEffect::begin() {
    Serial.println("CandleFlicker: Initializing...");
    
    // Initialize flicker state for each zone
    auto zones = config.getAllZones();
    flickerStates.clear();
    flickerStates.resize(zones.size());
    
    for (size_t i = 0; i < zones.size(); i++) {
        FlickerState& state = flickerStates[i];
        state.lastUpdate = millis();
        state.currentBrightness = MIN_BRIGHTNESS;
        state.baseBrightness = MIN_BRIGHTNESS + random(0, 30);  // Vary base brightness
        state.flickerPhase = random(0, 628) / 100.0;  // Random starting phase (0-2π)
        state.flickerSpeed = random(50, 200) / 100.0; // Random speed multiplier
        state.nextChange = millis() + random(500, 2000); // Change pattern every 0.5-2s
    }
    
    Serial.printf("CandleFlicker: Initialized for %d zones\n", zones.size());
}

void CandleEffect::update() {
    if (!enabled) return;
    
    auto zones = config.getAllZones();
    
    // Ensure we have flicker states for all zones
    if (flickerStates.size() != zones.size()) {
        begin(); // Reinitialize if zone count changed
    }
    
    for (size_t i = 0; i < zones.size(); i++) {
        if (i < flickerStates.size()) {
            updateFlickerForZone(i, zones[i]);
        }
    }
}

void CandleEffect::setEnabled(bool enabled) {
    if (this->enabled != enabled) {
        this->enabled = enabled;
        Serial.printf("CandleFlicker: %s\n", enabled ? "Enabled" : "Disabled");
        
        if (enabled) {
            begin(); // Initialize when enabled
        }
    }
}

void CandleEffect::updateFlickerForZone(size_t zoneIndex, Zone* zone) {
    if (!zone || !zone->enabled) return;
    
    FlickerState& state = flickerStates[zoneIndex];
    uint32_t currentTime = millis();
    
    // Only update every UPDATE_INTERVAL ms for smooth animation
    if (currentTime - state.lastUpdate < UPDATE_INTERVAL) {
        return;
    }
    
    float deltaTime = (currentTime - state.lastUpdate) / 1000.0; // Time in seconds
    
    // Update flicker phase
    state.flickerPhase += deltaTime * state.flickerSpeed * 3.14159; // Advance phase
    
    // Generate realistic candle flicker using multiple frequencies
    float baseWave = sin(state.flickerPhase) * 0.3;                    // Main flicker
    float microFlicker = sin(state.flickerPhase * 7.3) * 0.15;        // Fast micro-flicker
    float slowDrift = sin(state.flickerPhase * 0.4) * 0.2;            // Slow drift
    
    // Add some random noise
    float noise = (random(-100, 100) / 1000.0) * 0.1; // Small random component
    
    // Combine all components
    float flickerAmount = baseWave + microFlicker + slowDrift + noise;
    
    // Calculate final brightness
    float maxBrightness = (float)zone->brightness;
    float targetBrightness = state.baseBrightness + (flickerAmount * BRIGHTNESS_VARIANCE);
    
    // Clamp to valid range
    targetBrightness = max((float)MIN_BRIGHTNESS, min(maxBrightness, targetBrightness));
    
    // Smooth interpolation toward target
    float smoothing = 0.3; // Adjust for responsiveness (0.1 = slow, 0.9 = fast)
    state.currentBrightness = state.currentBrightness * (1.0 - smoothing) + targetBrightness * smoothing;
    
    // Occasionally change the base parameters for variety
    if (currentTime >= state.nextChange) {
        state.baseBrightness = MIN_BRIGHTNESS + random(0, 40);
        state.flickerSpeed = random(50, 200) / 100.0;
        state.nextChange = currentTime + random(1000, 3000);
    }
    
    // Apply brightness to LED controller - adapt to zone type
    uint8_t brightness = (uint8_t)round(state.currentBrightness);
    
    if (zone->type == ZoneType::PWM) {
        // PWM zones: just set brightness
        ledController.setZoneBrightness(zone->id, brightness);
    } else if (zone->type == ZoneType::WS2812B) {
        // RGB zones: set warm flickering candle color with brightness
        // Candle flame colors: orange-yellow with some red variation
        uint8_t red = 255;
        uint8_t green = map(brightness, 0, 255, 60, 180);  // Varies with flicker
        uint8_t blue = map(brightness, 0, 255, 0, 30);     // Minimal blue for warmth
        
        CRGB candleColor = CRGB(red, green, blue);
        ledController.setZoneColorAndBrightness(zone->id, candleColor, brightness);
    }
    
    state.lastUpdate = currentTime;
}


} // namespace BattleAura