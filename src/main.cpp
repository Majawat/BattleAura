#include <Arduino.h>
#include "config/Configuration.h"
#include "hardware/LedController.h"
#include "web/WebServer.h"

using namespace BattleAura;

// Global instances
Configuration BattleAura::config;
LedController ledController;
WebServer webServer(config, ledController);

void setup() {
    Serial.begin(115200);
    delay(6000);
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
    
    // Initialize web server
    Serial.println("Initializing web server...");
    if (!webServer.begin()) {
        Serial.println("ERROR: Web server failed to initialize!");
        return;
    }
    
    // Print status
    config.printStatus();
    ledController.printStatus();
    webServer.printStatus();
    
    Serial.println("\n=== Phase 1 System Ready ===");
    Serial.println("- LEDs will fade up and down automatically");
    Serial.println("- Web interface available for remote control");
    Serial.printf("- Access at: http://%s\n", webServer.getIPAddress().c_str());
}

void loop() {
    static uint32_t lastUpdate = 0;
    static uint8_t brightness = 0;
    static int8_t direction = 1;
    
    // Handle web server
    webServer.handle();
    
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
        
        // Print status every 10 seconds
        static uint32_t lastPrint = 0;
        if (millis() - lastPrint >= 10000) {
            lastPrint = millis();
            Serial.printf("Status: Brightness %d | WiFi: %s | IP: %s\n", 
                         brightness, 
                         webServer.isWiFiConnected() ? "Connected" : "AP Mode",
                         webServer.getIPAddress().c_str());
        }
    }
}