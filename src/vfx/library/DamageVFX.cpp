#include "DamageVFX.h"
#include <FastLED.h>

namespace BattleAura {

DamageVFX::DamageVFX(LedController& ledController, Configuration& config) 
    : BaseVFX(ledController, config, "Damage", VFXPriority::GLOBAL) {
}

void DamageVFX::begin() {
    Serial.println("Damage: Initializing...");
    
    std::vector<Zone*> zones = hasTargetZones() ? getTargetZones() : config.getAllZones();
    damageStates.clear();
    damageStates.resize(zones.size());
    
    // Initialize damage states
    for (size_t i = 0; i < zones.size(); i++) {
        DamageState& state = damageStates[i];
        state.damageStartTime = 0;
        state.lastFlicker = 0;
        state.originalBrightness = 0;
        state.originalColor = CRGB::Black;
        state.hasOriginalState = false;
        state.intensity = 1.0;
    }
    
    Serial.printf("Damage: Initialized for %d zones\n", zones.size());
}

void DamageVFX::trigger(uint32_t duration) {
    BaseVFX::trigger(duration);
    startDamage();
}

void DamageVFX::update() {
    if (!enabled) return;
    
    // Check if timed effect should stop
    if (shouldStop()) {
        // Restore all zones before stopping
        std::vector<Zone*> zones = hasTargetZones() ? getTargetZones() : config.getAllZones();
        for (size_t i = 0; i < zones.size(); i++) {
            if (i < damageStates.size()) {
                restoreZone(i, zones[i]);
            }
        }
        stop();
        return;
    }
    
    std::vector<Zone*> zones = hasTargetZones() ? getTargetZones() : config.getAllZones();
    
    // Ensure we have damage states for all zones
    if (damageStates.size() != zones.size()) {
        begin(); // Reinitialize if zone count changed
    }
    
    for (size_t i = 0; i < zones.size(); i++) {
        if (i < damageStates.size()) {
            updateDamageForZone(i, zones[i]);
        }
    }
}

void DamageVFX::startDamage() {
    if (!enabled) return;
    
    uint32_t currentTime = millis();
    std::vector<Zone*> zones = hasTargetZones() ? getTargetZones() : config.getAllZones();
    
    // Store original states and start damage effect
    for (size_t i = 0; i < damageStates.size() && i < zones.size(); i++) {
        DamageState& state = damageStates[i];
        Zone* zone = zones[i];
        
        if (zone && zone->enabled) {
            // Store original state for restoration
            state.originalBrightness = ledController.getZoneBrightness(zone->id);
            state.originalColor = ledController.getZoneColor(zone->id);
            state.hasOriginalState = true;
            
            state.damageStartTime = currentTime;
            state.lastFlicker = currentTime;
            state.intensity = 1.0; // Start at full intensity
        }
    }
}

void DamageVFX::updateDamageForZone(size_t zoneIndex, Zone* zone) {
    if (!zone || !zone->enabled) return;
    
    DamageState& state = damageStates[zoneIndex];
    uint32_t currentTime = millis();
    
    if (!state.hasOriginalState) return;
    
    // Calculate damage intensity (fades over time)
    float elapsed = (currentTime - state.damageStartTime) / 1000.0;
    state.intensity = max(0.0f, 1.0f - (elapsed / ((float)triggerDuration / 1000.0f)));
    
    // Damage flicker timing
    bool shouldFlicker = (currentTime - state.lastFlicker) < (FLICKER_INTERVAL / 2);
    if (currentTime - state.lastFlicker >= FLICKER_INTERVAL) {
        state.lastFlicker = currentTime;
    }
    
    if (zone->type == ZoneType::PWM) {
        // PWM zones: rapid brightness flicker with red intensity
        uint8_t brightness;
        if (shouldFlicker) {
            brightness = DAMAGE_BRIGHTNESS * state.intensity;
            if (brightness > zone->brightness) brightness = zone->brightness;
        } else {
            brightness = state.originalBrightness * 0.3; // Dimmed between flickers
        }
        ledController.setZoneBrightness(zone->id, brightness);
        
    } else if (zone->type == ZoneType::WS2812B) {
        // RGB zones: red damage flashes
        CRGB damageColor;
        uint8_t brightness;
        
        if (shouldFlicker) {
            // Bright red damage flash
            damageColor = CRGB(255, 50, 0); // Bright red with slight orange
            brightness = DAMAGE_BRIGHTNESS * state.intensity;
            if (brightness > zone->brightness) brightness = zone->brightness;
        } else {
            // Dimmed original color between flashes
            damageColor = state.originalColor;
            damageColor.nscale8(80); // Dim to 30% between flashes
            brightness = state.originalBrightness * 0.3;
        }
        
        ledController.setZoneColorAndBrightness(zone->id, damageColor, brightness);
    }
}

void DamageVFX::restoreZone(size_t zoneIndex, Zone* zone) {
    if (!zone || zoneIndex >= damageStates.size()) return;
    
    DamageState& state = damageStates[zoneIndex];
    if (!state.hasOriginalState) return;
    
    // Restore original state
    if (zone->type == ZoneType::PWM) {
        ledController.setZoneBrightness(zone->id, state.originalBrightness);
    } else if (zone->type == ZoneType::WS2812B) {
        ledController.setZoneColorAndBrightness(zone->id, state.originalColor, state.originalBrightness);
    }
    
    state.hasOriginalState = false;
}

} // namespace BattleAura