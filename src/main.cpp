#include <Arduino.h>
#include "config/Configuration.h"
#include "hardware/LedController.h"

using namespace BattleAura;

// Global instances
Configuration BattleAura::config;
LedController ledController;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== BattleAura v2.0.0 - Phase 1 Test ===");
    
    // Initialize configuration
    Serial.println("Initializing configuration...");
    if (!config.begin()) {
        Serial.println("ERROR: Configuration failed to initialize!");
        return;
    }
    
    // Initialize LED controller
    Serial.println("Initializing LED controller...");
    if (!ledController.begin()) {
        Serial.println("ERROR: LED controller failed to initialize!");
        return;
    }
    
    // Add zones from configuration to LED controller
    auto zones = config.getAllZones();
    Serial.printf("Adding %d zones to LED controller...\n", zones.size());
    for (Zone* zone : zones) {
        ledController.addZone(*zone);
    }
    
    // Print status
    config.printStatus();
    ledController.printStatus();
    
    Serial.println("\n=== Starting LED brightness test ===");
    Serial.println("LEDs will fade up and down on configured pins");
}

void loop() {
    static uint32_t lastUpdate = 0;
    static uint8_t brightness = 0;
    static int8_t direction = 1;
    
    // Update every 50ms for smooth fading
    if (millis() - lastUpdate >= 50) {
        lastUpdate = millis();
        
        // Update brightness
        brightness += direction * 5;
        if (brightness >= 255 || brightness <= 0) {
            direction = -direction;
            if (brightness <= 0) brightness = 0;
            if (brightness >= 255) brightness = 255;
        }
        
        // Set brightness on all zones
        auto zones = config.getAllZones();
        for (Zone* zone : zones) {
            ledController.setZoneBrightness(zone->id, brightness);
        }
        
        // Apply changes to hardware
        ledController.update();
        
        // Print brightness every 2 seconds
        static uint32_t lastPrint = 0;
        if (millis() - lastPrint >= 2000) {
            lastPrint = millis();
            Serial.printf("Current brightness: %d\n", brightness);
        }
    }
}