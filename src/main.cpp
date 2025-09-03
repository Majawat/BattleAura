#include <Arduino.h>
#include "config/Configuration.h"
#include "hardware/LedController.h"
#include "web/WebServer.h"
#include "effects/EffectManager.h"
#include "audio/AudioController.h"

using namespace BattleAura;

// Global instances
Configuration BattleAura::config;
LedController ledController;
AudioController audioController(config);
EffectManager effectManager(ledController, config);
WebServer webServer(config, ledController, effectManager, audioController);

void setup() {
    Serial.begin(115200);
    delay(6000);
    Serial.println("\n=== BattleAura v2.2.0-audio-integration - Complete System ===");
    
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
    
    // Initialize EffectManager
    Serial.println("Initializing EffectManager...");
    if (!effectManager.begin()) {
        Serial.println("ERROR: EffectManager failed to initialize!");
        return;
    }
    
    // Initialize AudioController
    Serial.println("Initializing AudioController...");
    if (!audioController.begin()) {
        Serial.println("WARNING: AudioController failed to initialize (audio will be disabled)");
    }
    
    // Print status
    config.printStatus();
    ledController.printStatus();
    webServer.printStatus();
    effectManager.printStatus();
    
    Serial.println("\n=== Phase 2 System Ready ===");
    Serial.println("- Full effect library with priority system active");
    Serial.println("- Mixed PWM and RGB LED support via FastLED");
    Serial.println("- Ambient effects running automatically");
    Serial.println("- Web interface available for remote control");
    Serial.println("- OTA firmware updates available via web interface");
    Serial.printf("- Access at: http://%s\n", webServer.getIPAddress().c_str());
}

void loop() {
    // Handle web server and OTA
    webServer.handle();
    
    // Update all effects via EffectManager
    effectManager.update();
    
    // Apply LED changes to hardware
    ledController.update();
    
    // Update audio controller
    audioController.update();
    
    // Print status every 15 seconds
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint >= 15000) {
        lastPrint = millis();
        Serial.printf("Status: Effects Active | WiFi: %s | IP: %s\n",
                     webServer.isWiFiConnected() ? "Connected" : "AP Mode",
                     webServer.getIPAddress().c_str());
        effectManager.printStatus();
    }
}