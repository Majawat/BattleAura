#include "WeaponFireEffect.h"
#include <FastLED.h>

namespace BattleAura {

WeaponFireEffect::WeaponFireEffect(LedController& ledController, Configuration& config) 
    : BaseEffect(ledController, config, "WeaponFire", EffectPriority::ACTIVE) {
}

void WeaponFireEffect::begin() {
    Serial.println("WeaponFire: Initializing...");
    
    auto zones = config.getAllZones();
    fireStates.clear();
    fireStates.resize(zones.size());
    
    // Initialize fire states
    for (size_t i = 0; i < zones.size(); i++) {
        FireState& state = fireStates[i];
        state.fireStartTime = 0;
        state.flashPattern = random(0, 4); // Different flash patterns
        state.lastFlash = 0;
        state.flashCount = 0;
        state.isFlashing = false;
    }
    
    Serial.printf("WeaponFire: Initialized for %d zones\n", zones.size());
}

void WeaponFireEffect::trigger(uint32_t duration) {
    BaseEffect::trigger(duration);
    startFiring();
}

void WeaponFireEffect::update() {
    if (!enabled) return;
    
    // Check if timed effect should stop
    if (shouldStop()) {
        stop();
        return;
    }
    
    auto zones = config.getAllZones();
    
    // Ensure we have fire states for all zones
    if (fireStates.size() != zones.size()) {
        begin(); // Reinitialize if zone count changed
    }
    
    for (size_t i = 0; i < zones.size(); i++) {
        if (i < fireStates.size()) {
            updateFireForZone(i, zones[i]);
        }
    }
}

void WeaponFireEffect::startFiring() {
    if (!enabled) return;
    
    uint32_t currentTime = millis();
    
    // Reset all fire states to start firing
    for (FireState& state : fireStates) {
        state.fireStartTime = currentTime;
        state.lastFlash = currentTime;
        state.flashCount = 0;
        state.isFlashing = true;
    }
}

void WeaponFireEffect::updateFireForZone(size_t zoneIndex, Zone* zone) {
    if (!zone || !zone->enabled) return;
    
    FireState& state = fireStates[zoneIndex];
    uint32_t currentTime = millis();
    
    if (!state.isFlashing) return;
    
    // Check if it's time for next flash
    if (currentTime - state.lastFlash >= FLASH_INTERVAL) {
        state.flashCount++;
        state.lastFlash = currentTime;
        
        // Stop flashing after max flashes
        if (state.flashCount >= MAX_FLASHES) {
            state.isFlashing = false;
        }
    }
    
    // Determine current brightness based on flash timing
    uint8_t brightness = 0;
    uint32_t flashTime = currentTime - state.lastFlash;
    
    if (state.isFlashing && flashTime < (FLASH_INTERVAL / 2)) {
        // Flash on for first half of interval
        switch (state.flashPattern) {
            case 0: brightness = FLASH_BRIGHTNESS; break;                    // Full flash
            case 1: brightness = FLASH_BRIGHTNESS * 0.8; break;             // Slightly dimmer
            case 2: brightness = (state.flashCount % 2) ? FLASH_BRIGHTNESS : FLASH_BRIGHTNESS * 0.6; break; // Alternating
            case 3: brightness = FLASH_BRIGHTNESS - (state.flashCount * 20); break; // Diminishing
        }
        
        // Clamp brightness
        if (brightness > zone->brightness) brightness = zone->brightness;
    }
    
    // Apply to LED controller - adapt to zone type
    if (zone->type == ZoneType::PWM) {
        // PWM zones: rapid bright flashes
        ledController.setZoneBrightness(zone->id, brightness);
    } else if (zone->type == ZoneType::WS2812B) {
        // RGB zones: bright white/yellow weapon flashes
        CRGB weaponColor;
        if (brightness > 0) {
            weaponColor = CRGB(255, 200, 100); // Bright yellow-white muzzle flash
        } else {
            weaponColor = CRGB::Black;
        }
        ledController.setZoneColorAndBrightness(zone->id, weaponColor, brightness);
    }
}

} // namespace BattleAura