#include "LedController.h"

namespace BattleAura {

LedController::LedController() {
    // Reserve space to prevent vector reallocation which breaks WS2812B leds pointers
    zones.reserve(12); // ESP32-C3 supports max ~11 GPIO pins for zones
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
    
    // Support both PWM and WS2812B zones in Phase 2
    if (zone.type != ZoneType::PWM && zone.type != ZoneType::WS2812B) {
        Serial.printf("LedController: Zone %d has unsupported type\n", zone.id);
        return;
    }
    
    // Validate GPIO
    if (!zone.enabled) {
        Serial.printf("LedController: Zone %d is disabled, skipping\n", zone.id);
        return;
    }
    
    // Add to zones list first
    zones.emplace_back(zone);
    
    // Setup hardware based on zone type
    bool setupSuccess = false;
    if (zone.type == ZoneType::PWM) {
        setupSuccess = setupPWM(zones.back());
        if (setupSuccess) {
            Serial.printf("LedController: Added PWM zone %d on GPIO %d\n", zone.id, zone.gpio);
        }
    } else if (zone.type == ZoneType::WS2812B) {
        setupSuccess = setupWS2812B(zones.back());
        if (setupSuccess) {
            Serial.printf("LedController: Added WS2812B zone %d on GPIO %d (%d LEDs)\n", 
                         zone.id, zone.gpio, zone.ledCount);
        }
    }
    
    if (!setupSuccess) {
        Serial.printf("LedController: Failed to setup zone %d on GPIO %d\n", zone.id, zone.gpio);
        zones.pop_back(); // Remove the zone if setup failed
        return;
    }
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
    
    // Scale effect brightness by user brightness setting
    // brightness parameter is the effect brightness (0-255)
    // userBrightness is the user's maximum setting (0-255) 
    uint16_t scaledBrightness = (brightness * zoneState->userBrightness) / 255;
    if (scaledBrightness > 255) scaledBrightness = 255;
    
    uint8_t finalBrightness = (uint8_t)scaledBrightness;
    
    if (zoneState->targetBrightness != finalBrightness) {
        zoneState->targetBrightness = finalBrightness;
        zoneState->needsUpdate = true;
    }
}

uint8_t LedController::getZoneBrightness(uint8_t zoneId) const {
    const ZoneState* zoneState = findZone(zoneId);
    return zoneState ? zoneState->currentBrightness : 0;
}

void LedController::setZoneColor(uint8_t zoneId, CRGB color) {
    ZoneState* zoneState = findZone(zoneId);
    if (!zoneState || zoneState->zone.type != ZoneType::WS2812B) {
        return; // Only WS2812B zones support color
    }
    
    if (zoneState->targetColor != color) {
        zoneState->targetColor = color;
        zoneState->needsUpdate = true;
    }
}

void LedController::setZoneColorAndBrightness(uint8_t zoneId, CRGB color, uint8_t brightness) {
    ZoneState* zoneState = findZone(zoneId);
    if (!zoneState) return;
    
    // Scale effect brightness by user brightness setting
    uint16_t scaledBrightness = (brightness * zoneState->userBrightness) / 255;
    if (scaledBrightness > 255) scaledBrightness = 255;
    
    uint8_t finalBrightness = (uint8_t)scaledBrightness;
    
    bool needsUpdate = false;
    
    if (zoneState->targetBrightness != finalBrightness) {
        zoneState->targetBrightness = finalBrightness;
        needsUpdate = true;
    }
    
    if (zoneState->zone.type == ZoneType::WS2812B && zoneState->targetColor != color) {
        zoneState->targetColor = color;
        needsUpdate = true;
    }
    
    if (needsUpdate) {
        zoneState->needsUpdate = true;
    }
}

CRGB LedController::getZoneColor(uint8_t zoneId) const {
    const ZoneState* zoneState = findZone(zoneId);
    return (zoneState && zoneState->zone.type == ZoneType::WS2812B) ? 
           zoneState->currentColor : CRGB::Black;
}

void LedController::update() {
    for (ZoneState& zoneState : zones) {
        if (zoneState.needsUpdate) {
            // For Phase 2, directly set values - no smooth transitions yet
            zoneState.currentBrightness = zoneState.targetBrightness;
            zoneState.currentColor = zoneState.targetColor;
            
            // Update hardware based on zone type
            if (zoneState.zone.type == ZoneType::PWM) {
                updatePWM(zoneState.pwmChannel, zoneState.currentBrightness, zoneState.zone.brightness);
            } else if (zoneState.zone.type == ZoneType::WS2812B) {
                updateWS2812B(zoneState);
            }
            
            zoneState.needsUpdate = false;
        }
    }
    
    // Update FastLED for all WS2812B changes
    FastLED.show();
}

bool LedController::isZoneConfigured(uint8_t zoneId) const {
    return findZone(zoneId) != nullptr;
}

void LedController::printStatus() const {
    Serial.println("=== LedController Status ===");
    Serial.printf("Configured zones: %d\n", zones.size());
    
    for (const ZoneState& zoneState : zones) {
        const char* typeStr = (zoneState.zone.type == ZoneType::PWM) ? "PWM" : "WS2812B";
        Serial.printf("  Zone %d: GPIO %d, Type: %s", 
                     zoneState.zone.id, zoneState.zone.gpio, typeStr);
        
        if (zoneState.zone.type == ZoneType::PWM) {
            Serial.printf(", PWM Ch: %d", zoneState.pwmChannel);
        } else {
            Serial.printf(", LEDs: %d, Color: R%d G%d B%d", 
                         zoneState.zone.ledCount, 
                         zoneState.currentColor.r, 
                         zoneState.currentColor.g, 
                         zoneState.currentColor.b);
        }
        
        Serial.printf(", Brightness: %d/%d/%d (current/target/max)\n",
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

bool LedController::setupWS2812B(ZoneState& zoneState) {
    // Allocate LED array for this zone
    if (zoneState.zone.ledCount == 0) {
        Serial.printf("LedController: WS2812B zone %d has 0 LEDs\n", zoneState.zone.id);
        return false;
    }
    
    zoneState.leds = new CRGB[zoneState.zone.ledCount];
    if (!zoneState.leds) {
        Serial.printf("LedController: Failed to allocate LEDs for zone %d\n", zoneState.zone.id);
        return false;
    }
    
    // Initialize all LEDs to black
    for (uint8_t i = 0; i < zoneState.zone.ledCount; i++) {
        zoneState.leds[i] = CRGB::Black;
    }
    
    // Add LED strip to FastLED - using template matching for common pins
    switch (zoneState.zone.gpio) {
        case 2:  FastLED.addLeds<WS2812B, 2, GRB>(zoneState.leds, zoneState.zone.ledCount); break;
        case 3:  FastLED.addLeds<WS2812B, 3, GRB>(zoneState.leds, zoneState.zone.ledCount); break;
        case 4:  FastLED.addLeds<WS2812B, 4, GRB>(zoneState.leds, zoneState.zone.ledCount); break;
        case 5:  FastLED.addLeds<WS2812B, 5, GRB>(zoneState.leds, zoneState.zone.ledCount); break;
        case 6:  FastLED.addLeds<WS2812B, 6, GRB>(zoneState.leds, zoneState.zone.ledCount); break;
        case 7:  FastLED.addLeds<WS2812B, 7, GRB>(zoneState.leds, zoneState.zone.ledCount); break;
        case 8:  FastLED.addLeds<WS2812B, 8, GRB>(zoneState.leds, zoneState.zone.ledCount); break;
        case 9:  FastLED.addLeds<WS2812B, 9, GRB>(zoneState.leds, zoneState.zone.ledCount); break;
        case 10: FastLED.addLeds<WS2812B, 10, GRB>(zoneState.leds, zoneState.zone.ledCount); break;
        case 20: FastLED.addLeds<WS2812B, 20, GRB>(zoneState.leds, zoneState.zone.ledCount); break;
        case 21: FastLED.addLeds<WS2812B, 21, GRB>(zoneState.leds, zoneState.zone.ledCount); break;
        default:
            Serial.printf("LedController: Unsupported GPIO %d for WS2812B\n", zoneState.zone.gpio);
            delete[] zoneState.leds;
            zoneState.leds = nullptr;
            return false;
    }
    
    Serial.printf("LedController: WS2812B setup SUCCESS on GPIO %d, %d LEDs\n", 
                 zoneState.zone.gpio, zoneState.zone.ledCount);
    return true;
}

void LedController::updateWS2812B(ZoneState& zoneState) {
    if (!zoneState.leds) return;
    
    // Apply color and brightness to all LEDs in this zone
    CRGB color = zoneState.currentColor;
    
    // Scale brightness - FastLED handles this internally but we need to respect zone max
    uint8_t scaledBrightness = (zoneState.currentBrightness * 255) / zoneState.zone.brightness;
    if (scaledBrightness > 255) scaledBrightness = 255;
    
    color.nscale8(scaledBrightness);  // Apply brightness scaling to color
    
    // Set all LEDs in this zone to the same color/brightness
    for (uint8_t i = 0; i < zoneState.zone.ledCount; i++) {
        zoneState.leds[i] = color;
    }
}

// User brightness control methods
void LedController::setUserBrightness(uint8_t zoneId, uint8_t brightness) {
    ZoneState* zoneState = findZone(zoneId);
    if (!zoneState) {
        return;
    }
    
    // Clamp to 0-255 range
    if (brightness > 255) brightness = 255;
    
    zoneState->userBrightness = brightness;
    
    // Force recalculation of current effect brightness
    zoneState->needsUpdate = true;
    
    Serial.printf("LedController: Set user brightness for zone %d to %d\n", zoneId, brightness);
}

uint8_t LedController::getUserBrightness(uint8_t zoneId) const {
    const ZoneState* zoneState = findZone(zoneId);
    return zoneState ? zoneState->userBrightness : 0;
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