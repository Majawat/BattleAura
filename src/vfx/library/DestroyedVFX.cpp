#include "DestroyedVFX.h"
#include "../../config/Configuration.h"

namespace BattleAura {

DestroyedVFX::DestroyedVFX(LedController& ledController, Configuration& config)
    : BaseVFX(ledController, config, "Destroyed", VFXPriority::GLOBAL) {
}

void DestroyedVFX::begin() {
    Serial.println("DestroyedVFX: Initializing");
    
    // Initialize destruction states for all zones
    destructionStates.clear();
    destructionStates.resize(targetZones.size());
    
    for (auto& state : destructionStates) {
        state.destructionStartTime = 0;
        state.currentIntensity = 0;
        state.lastUpdate = 0;
        state.lastExplosion = 0;
        state.isDestroyed = false;
        state.destructionPhase = 0;
        state.explosionCount = 0;
    }
}

void DestroyedVFX::update() {
    if (!enabled) {
        return;
    }
    
    uint32_t currentTime = millis();
    
    // Check if duration has expired
    if (shouldStop()) {
        Serial.println("DestroyedVFX: Duration expired");
        stop();
        return;
    }
    
    // Update destruction effect for each target zone
    for (size_t i = 0; i < targetZones.size(); i++) {
        Zone* zone = targetZones[i];
        if (zone && zone->enabled) {
            updateDestructionForZone(i, zone);
        }
    }
}

void DestroyedVFX::trigger(uint32_t duration) {
    Serial.printf("DestroyedVFX: Triggered with duration %dms\n", duration);
    
    BaseVFX::trigger(duration);
    
    startDestruction();
}

void DestroyedVFX::startDestruction() {
    Serial.printf("DestroyedVFX: Starting destruction sequence on %d zones\n", targetZones.size());
    
    uint32_t currentTime = millis();
    
    // Initialize destruction state for each zone with slight randomization
    for (size_t i = 0; i < destructionStates.size(); i++) {
        auto& state = destructionStates[i];
        state.destructionStartTime = currentTime + (i * 50); // Stagger start times
        state.currentIntensity = 0;
        state.lastUpdate = currentTime;
        state.lastExplosion = currentTime;
        state.isDestroyed = true;
        state.destructionPhase = 0; // Start with explosions
        state.explosionCount = 0;
    }
}

void DestroyedVFX::updateDestructionForZone(size_t zoneIndex, Zone* zone) {
    if (zoneIndex >= destructionStates.size()) return;
    
    auto& state = destructionStates[zoneIndex];
    if (!state.isDestroyed) return;
    
    uint32_t currentTime = millis();
    
    // Update timing
    if (currentTime - state.lastUpdate < UPDATE_INTERVAL) {
        return; // Not time for update yet
    }
    
    state.lastUpdate = currentTime;
    uint32_t elapsed = currentTime - state.destructionStartTime;
    
    // Determine destruction phase based on elapsed time
    if (elapsed <= EXPLOSION_PHASE_TIME) {
        state.destructionPhase = 0; // Explosions
        
        // Handle explosion timing
        if (currentTime - state.lastExplosion >= EXPLOSION_INTERVAL && 
            state.explosionCount < MAX_EXPLOSIONS) {
            state.lastExplosion = currentTime;
            state.explosionCount++;
            state.currentIntensity = MAX_INTENSITY; // Flash to max
        } else {
            // Fade explosion
            if (state.currentIntensity > 0) {
                state.currentIntensity = max(0, (int)state.currentIntensity - 20);
            }
        }
        
    } else if (elapsed <= EXPLOSION_PHASE_TIME + FIRE_PHASE_TIME) {
        state.destructionPhase = 1; // Fire phase
        // Flickering fire effect
        uint32_t fireElapsed = elapsed - EXPLOSION_PHASE_TIME;
        float fireIntensity = 0.7 + 0.3 * sin(fireElapsed * 0.01);
        state.currentIntensity = (uint8_t)(fireIntensity * 200) + random(0, 40);
        
    } else if (elapsed <= EXPLOSION_PHASE_TIME + FIRE_PHASE_TIME + SPARK_PHASE_TIME) {
        state.destructionPhase = 2; // Sparks phase
        // Random sparking effect
        if (random(0, 100) < 20) { // 20% chance of spark
            state.currentIntensity = random(100, 200);
        } else {
            state.currentIntensity = max(0, (int)state.currentIntensity - 10);
        }
        
    } else {
        state.destructionPhase = 3; // Fade phase
        uint32_t fadeElapsed = elapsed - EXPLOSION_PHASE_TIME - FIRE_PHASE_TIME - SPARK_PHASE_TIME;
        state.currentIntensity = map(fadeElapsed, 0, FADE_PHASE_TIME, 50, 0);
        
        if (fadeElapsed >= FADE_PHASE_TIME) {
            state.isDestroyed = false; // Effect complete
        }
    }
    
    // Apply destruction effect based on zone type
    if (zone->type == ZoneType::PWM) {
        // For PWM zones, directly control brightness
        ledController.setZoneBrightness(zone->id, state.currentIntensity);
    } else if (zone->type == ZoneType::WS2812B) {
        // For RGB zones, use destruction colors
        CRGB destructionColor = getDestructionColor(state.destructionPhase, state.currentIntensity);
        ledController.setZoneColor(zone->id, destructionColor);
    }
}

uint8_t DestroyedVFX::calculateExplosionIntensity(uint32_t elapsed, uint8_t phase) {
    switch (phase) {
        case 0: // Explosions - bright flashes
            return (elapsed % EXPLOSION_INTERVAL < 100) ? MAX_INTENSITY : 0;
        case 1: // Fire - flickering
            return 150 + random(0, 80);
        case 2: // Sparks - random
            return random(0, 100) < 30 ? random(80, 150) : 20;
        case 3: // Fade - diminishing
            return max(0, 50 - (int)(elapsed * 0.1));
        default:
            return 0;
    }
}

CRGB DestroyedVFX::getDestructionColor(uint8_t phase, uint8_t intensity) {
    CRGB color;
    
    switch (phase) {
        case 0: // Explosions - white/yellow
            color.r = intensity;
            color.g = intensity;
            color.b = intensity / 2;
            break;
        case 1: // Fire - orange/red
            color.r = intensity;
            color.g = (intensity * 60) / 255;
            color.b = 0;
            break;
        case 2: // Sparks - yellow/white
            color.r = intensity;
            color.g = (intensity * 80) / 255;
            color.b = intensity / 4;
            break;
        case 3: // Fade - dim red
            color.r = intensity;
            color.g = intensity / 4;
            color.b = 0;
            break;
        default:
            color = CRGB::Black;
    }
    
    return color;
}

} // namespace BattleAura