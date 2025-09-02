#include <Arduino.h>
#include "config/Configuration.h"
#include "hardware/LedController.h"
#include "web/WebServer.h"
#include "effects/library/CandleEffect.h"

using namespace BattleAura;

// Global instances
Configuration BattleAura::config;
LedController ledController;
WebServer webServer(config, ledController);
CandleEffect candleEffect(ledController, config);

void setup() {
    Serial.begin(115200);
    delay(6000);
    Serial.println("\n=== BattleAura v2.0.1 - Phase 1 Complete ===");
    
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
    
    // Initialize CandleEffect
    Serial.println("Initializing CandleEffect...");
    candleEffect.begin();
    candleEffect.setEnabled(true);
    
    // Print status
    config.printStatus();
    ledController.printStatus();
    webServer.printStatus();
    
    Serial.println("\n=== Phase 1 System Ready ===");
    Serial.println("- LEDs will flicker like candles automatically");
    Serial.println("- Web interface available for remote control");
    Serial.println("- OTA firmware updates available via web interface");
    Serial.printf("- Access at: http://%s\n", webServer.getIPAddress().c_str());
}

void loop() {
    // Handle web server and OTA
    webServer.handle();
    
    // Update CandleEffect
    candleEffect.update();
    
    // Apply LED changes to hardware
    ledController.update();
    
    // Print status every 10 seconds
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint >= 10000) {
        lastPrint = millis();
        Serial.printf("Status: CandleEffect %s | WiFi: %s | IP: %s\n", 
                     candleEffect.isEnabled() ? "ON" : "OFF",
                     webServer.isWiFiConnected() ? "Connected" : "AP Mode",
                     webServer.getIPAddress().c_str());
    }
}