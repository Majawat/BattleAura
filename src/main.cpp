#include <Arduino.h>
#include "core/config.h"
#include "hal/gpio_manager.h"
#include "web/server.h"

// Application state
bool systemInitialized = false;
unsigned long lastStatusReport = 0;
const unsigned long STATUS_INTERVAL = 30000; // 30 seconds

void setup() {
    Serial.begin(115200);
    delay(2000); // Allow serial monitor to connect
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("BattleAura v" BATTLEARUA_VERSION " Starting...");
    Serial.println("========================================");
    Serial.printf("Chip: %s\n", ESP.getChipModel());
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Flash size: %d bytes\n", ESP.getFlashChipSize());
    Serial.println("========================================");
    
    // Initialize core components in proper order
    if (!initializeSystem()) {
        Serial.println("CRITICAL: System initialization failed!");
        Serial.println("System will continue with limited functionality");
        // Don't return - continue with degraded functionality
    }
    
    Serial.println();
    Serial.println("🎉 BattleAura Ready! 🎉");
    printSystemInfo();
    Serial.println("========================================");
    
    systemInitialized = true;
}

void loop() {
    // Handle web server requests
    WebServerManager::getInstance().handleClient();
    
    // Periodic status reporting
    unsigned long currentTime = millis();
    if (currentTime - lastStatusReport >= STATUS_INTERVAL) {
        reportSystemStatus();
        lastStatusReport = currentTime;
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}

bool initializeSystem() {
    bool allSuccess = true;
    
    // 1. Initialize Configuration Manager
    Serial.println("Initializing ConfigManager...");
    if (ConfigManager::getInstance().initialize()) {
        Serial.println("✓ ConfigManager initialized successfully");
    } else {
        Serial.println("✗ ConfigManager initialization failed");
        allSuccess = false;
    }
    
    // 2. Initialize GPIO Manager  
    Serial.println("Initializing GPIOManager...");
    GPIOManager& gpio = GPIOManager::getInstance();
    
    // Configure pins based on config
    const SystemConfig& config = ConfigManager::getInstance().getConfig();
    for (uint8_t i = 0; i < 10; i++) {
        const PinConfig& pinCfg = config.pins[i];
        if (pinCfg.enabled) {
            if (gpio.configurePins(pinCfg.pin, pinCfg.pinMode)) {
                Serial.printf("✓ Configured GPIO %d as %s\n", 
                    pinCfg.pin, getPinModeString(pinCfg.pinMode));
            } else {
                Serial.printf("✗ Failed to configure GPIO %d\n", pinCfg.pin);
                allSuccess = false;
            }
        }
    }
    
    // 3. Initialize Web Server
    Serial.println("Initializing WebServer...");
    if (WebServerManager::getInstance().initialize()) {
        Serial.println("✓ WebServer initialized successfully");
    } else {
        Serial.println("✗ WebServer initialization failed");
        allSuccess = false;
    }
    
    // 4. Setup WiFi
    Serial.println("Setting up WiFi...");
    if (setupWiFi()) {
        Serial.println("✓ WiFi setup completed");
    } else {
        Serial.println("✗ WiFi setup failed");
        allSuccess = false;
    }
    
    return allSuccess;
}

bool setupWiFi() {
    const SystemConfig& config = ConfigManager::getInstance().getConfig();
    WebServerManager& webServer = WebServerManager::getInstance();
    
    if (config.wifiEnabled && config.wifiSSID.length() > 0) {
        Serial.printf("Attempting to connect to WiFi: %s\n", config.wifiSSID.c_str());
        
        if (webServer.connectWiFiStation(config.wifiSSID, config.wifiPassword)) {
            Serial.printf("✓ Connected to WiFi. IP: %s\n", webServer.getIPAddress().c_str());
            return true;
        } else {
            Serial.println("✗ WiFi connection failed, falling back to AP mode");
        }
    }
    
    // Fallback to AP mode
    Serial.println("Starting WiFi Access Point...");
    String apName = config.deviceName;
    if (apName.length() == 0) {
        apName = "BattleAura-" + String(random(1000, 9999));
    }
    
    if (webServer.startWiFiAP(apName, "battlesync")) {
        Serial.printf("✓ WiFi AP started: %s\n", apName.c_str());
        Serial.printf("✓ AP IP Address: %s\n", webServer.getIPAddress().c_str());
        return true;
    } else {
        Serial.println("✗ Failed to start WiFi AP");
        return false;
    }
}

void printSystemInfo() {
    const SystemConfig& config = ConfigManager::getInstance().getConfig();
    WebServerManager& webServer = WebServerManager::getInstance();
    
    Serial.printf("Device Name: %s\n", config.deviceName.c_str());
    Serial.printf("IP Address: %s\n", webServer.getIPAddress().c_str());
    Serial.printf("Web Interface: http://%s/\n", webServer.getIPAddress().c_str());
    Serial.printf("Active Pins: %d\n", config.activePins);
    Serial.printf("Audio: %s (Volume: %d)\n", 
        config.audioEnabled ? "Enabled" : "Disabled", config.volume);
    
    if (config.activePins > 0) {
        Serial.println("Configured Pins:");
        for (uint8_t i = 0; i < 10; i++) {
            const PinConfig& pin = config.pins[i];
            if (pin.enabled) {
                Serial.printf("  GPIO %d: %s (%s)\n", 
                    pin.pin, pin.name.c_str(), getPinModeString(pin.pinMode));
            }
        }
    }
}

void reportSystemStatus() {
    if (!systemInitialized) return;
    
    Serial.printf("[%lu] Status: Free heap: %d bytes | Uptime: %lu seconds\n",
        millis(), ESP.getFreeHeap(), millis() / 1000);
}

const char* getPinModeString(PinMode mode) {
    switch (mode) {
        case PinMode::PIN_DISABLED: return "Disabled";
        case PinMode::OUTPUT_STANDARD: return "Standard Output";
        case PinMode::OUTPUT_PWM: return "PWM Output";  
        case PinMode::OUTPUT_WS2812B: return "WS2812B RGB";
        case PinMode::INPUT_DIGITAL: return "Digital Input";
        case PinMode::INPUT_ANALOG: return "Analog Input";
        default: return "Unknown";
    }
}