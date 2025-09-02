#include "LedController.h"

namespace BattleAura {

LedController::LedController() {
    // Constructor - nothing to initialize
}

LedController::~LedController() {
    // Destructor - cleanup if needed
}

bool LedController::begin() {
    Serial.println("LedController: Initializing...");
    
    // Initialize any hardware-specific settings here
    // For now, just return success
    
    Serial.println("LedController: Ready");
    return true;
}

void LedController::addZone(const Zone& zone) {
    // Check if zone already exists
    if (findZone(zone.id) != nullptr) {
        Serial.printf("LedController: Zone %d already exists, updating\n", zone.id);
        removeZone(zone.id);
    }
    
    // Only support PWM zones for Phase 1
    if (zone.type != ZoneType::PWM) {
        Serial.printf("LedController: Zone %d type not supported in Phase 1\n", zone.id);
        return;
    }
    
    // Validate GPIO
    if (!zone.enabled) {
        Serial.printf("LedController: Zone %d is disabled, skipping\n", zone.id);
        return;
    }
    
    // Add to zones list first to get correct channel index
    zones.emplace_back(zone);
    
    // Setup PWM for this GPIO
    if (!setupPWM(zones.back())) {
        Serial.printf("LedController: Failed to setup PWM for GPIO %d\n", zone.gpio);
        zones.pop_back(); // Remove the zone if PWM setup failed
        return;
    }
    
    Serial.printf("LedController: Added PWM zone %d on GPIO %d\n", zone.id, zone.gpio);
}

void LedController::removeZone(uint8_t zoneId) {
    zones.erase(
        std::remove_if(zones.begin(), zones.end(),
            [zoneId](const ZoneState& zs) { return zs.zone.id == zoneId; }),
        zones.end()
    );
}

void LedController::setZoneBrightness(uint8_t zoneId, uint8_t brightness) {
    ZoneState* zoneState = findZone(zoneId);
    if (!zoneState) {
        return;
    }
    
    // Clamp brightness to zone's maximum
    uint8_t maxBrightness = zoneState->zone.brightness;
    uint8_t clampedBrightness = (brightness > maxBrightness) ? maxBrightness : brightness;
    
    if (zoneState->targetBrightness != clampedBrightness) {
        zoneState->targetBrightness = clampedBrightness;
        zoneState->needsUpdate = true;
    }
}

uint8_t LedController::getZoneBrightness(uint8_t zoneId) const {
    const ZoneState* zoneState = findZone(zoneId);
    return zoneState ? zoneState->currentBrightness : 0;
}

void LedController::update() {
    for (ZoneState& zoneState : zones) {
        if (zoneState.needsUpdate) {
            // For Phase 1, directly set PWM - no smooth transitions yet
            zoneState.currentBrightness = zoneState.targetBrightness;
            
            updatePWM(zoneState.pwmChannel, zoneState.currentBrightness, zoneState.zone.brightness);
            
            zoneState.needsUpdate = false;
        }
    }
}

bool LedController::isZoneConfigured(uint8_t zoneId) const {
    return findZone(zoneId) != nullptr;
}

void LedController::printStatus() const {
    Serial.println("=== LedController Status ===");
    Serial.printf("Configured zones: %d\n", zones.size());
    
    for (const ZoneState& zoneState : zones) {
        Serial.printf("  Zone %d: GPIO %d (CH%d), Current: %d, Target: %d, Max: %d\n",
                     zoneState.zone.id,
                     zoneState.zone.gpio,
                     zoneState.pwmChannel,
                     zoneState.currentBrightness,
                     zoneState.targetBrightness,
                     zoneState.zone.brightness);
    }
}

// Private methods

bool LedController::setupPWM(ZoneState& zoneState) {
    // ESP32 LEDC setup for PWM
    // Use sequential channel assignment for Phase 1
    uint8_t channel = zones.size() - 1;  // Use zone index as channel
    
    // ESP32 has 16 PWM channels (0-15)
    if (channel >= 16) {
        Serial.printf("LedController: No PWM channels available for GPIO %d\n", zoneState.zone.gpio);
        return false;
    }
    
    // Store channel in zone state
    zoneState.pwmChannel = channel;
    
    // Configure LEDC channel
    ledcSetup(channel, 5000, 8);  // 5kHz frequency, 8-bit resolution
    ledcAttachPin(zoneState.zone.gpio, channel);
    
    Serial.printf("LedController: PWM setup on GPIO %d, channel %d\n", zoneState.zone.gpio, channel);
    return true;
}

void LedController::updatePWM(uint8_t channel, uint8_t brightness, uint8_t maxBrightness) {
    // Scale brightness from 0-maxBrightness to 0-255
    uint16_t scaledBrightness = (brightness * 255) / maxBrightness;
    if (scaledBrightness > 255) scaledBrightness = 255;
    
    ledcWrite(channel, scaledBrightness);
}

LedController::ZoneState* LedController::findZone(uint8_t zoneId) {
    for (ZoneState& zoneState : zones) {
        if (zoneState.zone.id == zoneId) {
            return &zoneState;
        }
    }
    return nullptr;
}

const LedController::ZoneState* LedController::findZone(uint8_t zoneId) const {
    for (const ZoneState& zoneState : zones) {
        if (zoneState.zone.id == zoneId) {
            return &zoneState;
        }
    }
    return nullptr;
}

} // namespace BattleAura