#include "RocketLauncherVFX.h"
#include "../../config/Configuration.h"

namespace BattleAura {

RocketLauncherVFX::RocketLauncherVFX(LedController& ledController, Configuration& config)
    : BaseVFX(ledController, config, "RocketLauncher", VFXPriority::ACTIVE) {
}

void RocketLauncherVFX::begin() {
    Serial.println("RocketLauncherVFX: Initializing");
    
    // Initialize launch states for all zones
    launchStates.clear();
    launchStates.resize(targetZones.size());
    
    for (auto& state : launchStates) {
        state.launchStartTime = 0;
        state.currentIntensity = 0;
        state.lastUpdate = 0;
        state.isLaunching = false;
        state.launchPhase = 0;
    }
}

void RocketLauncherVFX::update() {
    if (!enabled) {
        return;
    }
    
    uint32_t currentTime = millis();
    
    // Check if duration has expired
    if (shouldStop()) {
        Serial.println("RocketLauncherVFX: Duration expired");
        stop();
        return;
    }
    
    // Update launch effect for each target zone
    for (size_t i = 0; i < targetZones.size(); i++) {
        Zone* zone = targetZones[i];
        if (zone && zone->enabled) {
            updateLaunchForZone(i, zone);
        }
    }
}

void RocketLauncherVFX::trigger(uint32_t duration) {
    Serial.printf("RocketLauncherVFX: Triggered with duration %dms\n", duration);
    
    BaseVFX::trigger(duration);
    
    startLaunching();
}

void RocketLauncherVFX::startLaunching() {
    Serial.printf("RocketLauncherVFX: Starting rocket launch effect on %d zones\n", targetZones.size());
    
    uint32_t currentTime = millis();
    
    // Initialize launch state for each zone
    for (size_t i = 0; i < launchStates.size(); i++) {
        auto& state = launchStates[i];
        state.launchStartTime = currentTime;
        state.currentIntensity = 0;
        state.lastUpdate = currentTime;
        state.isLaunching = true;
        state.launchPhase = 0; // Start with charge phase
    }
}

void RocketLauncherVFX::updateLaunchForZone(size_t zoneIndex, Zone* zone) {
    if (zoneIndex >= launchStates.size()) return;
    
    auto& state = launchStates[zoneIndex];
    if (!state.isLaunching) return;
    
    uint32_t currentTime = millis();
    
    // Update timing
    if (currentTime - state.lastUpdate < UPDATE_INTERVAL) {
        return; // Not time for update yet
    }
    
    state.lastUpdate = currentTime;
    uint32_t elapsed = currentTime - state.launchStartTime;
    
    // Determine launch phase based on elapsed time
    if (elapsed <= CHARGE_TIME) {
        state.launchPhase = 0; // Charge phase
        state.currentIntensity = map(elapsed, 0, CHARGE_TIME, 0, 120);
        
    } else if (elapsed <= CHARGE_TIME + FLASH_TIME) {
        state.launchPhase = 1; // Flash phase
        state.currentIntensity = MAX_INTENSITY; // Full brightness flash
        
    } else if (elapsed <= CHARGE_TIME + FLASH_TIME + AFTERGLOW_TIME) {
        state.launchPhase = 2; // Afterglow phase
        uint32_t afterglowElapsed = elapsed - CHARGE_TIME - FLASH_TIME;
        state.currentIntensity = map(afterglowElapsed, 0, AFTERGLOW_TIME, 200, 80);
        
    } else if (elapsed <= CHARGE_TIME + FLASH_TIME + AFTERGLOW_TIME + FADE_TIME) {
        state.launchPhase = 3; // Fade phase
        uint32_t fadeElapsed = elapsed - CHARGE_TIME - FLASH_TIME - AFTERGLOW_TIME;
        state.currentIntensity = map(fadeElapsed, 0, FADE_TIME, 80, 0);
        
    } else {
        // Launch complete
        state.currentIntensity = 0;
        state.isLaunching = false;
    }
    
    // Apply launch effect based on zone type
    if (zone->type == ZoneType::PWM) {
        // For PWM zones, directly control brightness
        ledController.setZoneBrightness(zone->id, state.currentIntensity);
    } else if (zone->type == ZoneType::WS2812B) {
        // For RGB zones, use launch colors
        CRGB launchColor = getLaunchColor(state.launchPhase, state.currentIntensity);
        ledController.setZoneColor(zone->id, launchColor);
    }
}

uint8_t RocketLauncherVFX::calculateLaunchIntensity(uint32_t elapsed, uint8_t phase) {
    switch (phase) {
        case 0: // Charge - gradual buildup
            return map(elapsed, 0, CHARGE_TIME, 0, 120);
        case 1: // Flash - maximum intensity
            return MAX_INTENSITY;
        case 2: // Afterglow - bright but fading
            return map(elapsed - CHARGE_TIME - FLASH_TIME, 0, AFTERGLOW_TIME, 200, 80);
        case 3: // Fade - diminishing
            return map(elapsed - CHARGE_TIME - FLASH_TIME - AFTERGLOW_TIME, 0, FADE_TIME, 80, 0);
        default:
            return 0;
    }
}

CRGB RocketLauncherVFX::getLaunchColor(uint8_t phase, uint8_t intensity) {
    CRGB color;
    
    switch (phase) {
        case 0: // Charge - blue/white buildup
            color.r = intensity / 2;
            color.g = intensity / 2;
            color.b = intensity;
            break;
        case 1: // Flash - bright white
            color.r = intensity;
            color.g = intensity;
            color.b = intensity;
            break;
        case 2: // Afterglow - orange/yellow
            color.r = intensity;
            color.g = (intensity * 80) / 255;
            color.b = intensity / 4;
            break;
        case 3: // Fade - dim orange
            color.r = intensity;
            color.g = intensity / 3;
            color.b = 0;
            break;
        default:
            color = CRGB::Black;
    }
    
    return color;
}

} // namespace BattleAura