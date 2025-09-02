#include "CandleEffect.h"

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
        state.targetBrightness = MIN_BRIGHTNESS;
        state.flickerDelay = random(MIN_FLICKER_DELAY, MAX_FLICKER_DELAY);
        state.needsNewTarget = true;
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
    
    // Check if it's time to update this zone
    if (currentTime - state.lastUpdate < state.flickerDelay) {
        return;
    }
    
    // Generate new target brightness if needed
    if (state.needsNewTarget) {
        state.targetBrightness = generateFlickerBrightness(zone->brightness);
        state.flickerDelay = random(MIN_FLICKER_DELAY, MAX_FLICKER_DELAY);
        state.needsNewTarget = false;
    }
    
    // Move current brightness toward target
    if (state.currentBrightness < state.targetBrightness) {
        state.currentBrightness = min((uint8_t)(state.currentBrightness + 5), state.targetBrightness);
    } else if (state.currentBrightness > state.targetBrightness) {
        state.currentBrightness = max((int)state.currentBrightness - 5, (int)state.targetBrightness);
    }
    
    // If we reached target, mark for new target next time
    if (state.currentBrightness == state.targetBrightness) {
        state.needsNewTarget = true;
    }
    
    // Apply brightness to LED controller
    ledController.setZoneBrightness(zone->id, state.currentBrightness);
    
    state.lastUpdate = currentTime;
}

uint8_t CandleEffect::generateFlickerBrightness(uint8_t maxBrightness) {
    // Generate flickering brightness between MIN_BRIGHTNESS and maxBrightness
    uint8_t range = min((uint8_t)BRIGHTNESS_VARIANCE, (uint8_t)(maxBrightness - MIN_BRIGHTNESS));
    uint8_t flickerValue = random(0, range + 1);
    
    return min((uint8_t)(MIN_BRIGHTNESS + flickerValue), maxBrightness);
}

} // namespace BattleAura