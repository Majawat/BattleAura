#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>
#include <SPIFFS.h>
#include "webfiles.h"

// Application constants
const char* VERSION = "0.12.3-alpha";
const char* AP_SSID = "BattleAura";  
const char* AP_PASS = "battlesync";
const int AP_CHANNEL = 1;

// Configuration constants
#define CONFIG_FILE "/config.json"

// Dynamic audio length detection implemented - effects now sync with actual audio playback duration
// No more hardcoded durations needed! Effects will automatically end when audio finishes.

// Default effect durations when no audio is mapped
#define DEFAULT_EFFECT_DURATION_SHORT  1000   // Quick effects (strobe, pulse)
#define DEFAULT_EFFECT_DURATION_MEDIUM 2000   // Medium effects (machine gun, explosion)
#define DEFAULT_EFFECT_DURATION_LONG   4000   // Long effects (engine rev)

// Board-specific DFPlayer Mini pin configuration
#if defined(CONFIG_IDF_TARGET_ESP32C3)
    // ESP32-C3 (Seeed Xiao ESP32-C3)
    // Physical pins: RX=GPIO20/D7, TX=GPIO21/D6
    #define DFPLAYER_RX_PIN 20    // ESP32 RX <- DFPlayer TX (GPIO20/D7)
    #define DFPLAYER_TX_PIN 21    // ESP32 TX -> DFPlayer RX (GPIO21/D6)
    #define DFPLAYER_UART_NUM 1
    #define MAX_PINS 8
    #define BOARD_NAME "ESP32-C3"
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    // ESP32-S3 (Seeed Xiao ESP32-S3)  
    #define DFPLAYER_RX_PIN 7
    #define DFPLAYER_TX_PIN 6
    #define DFPLAYER_UART_NUM 1
    #define MAX_PINS 8
    #define BOARD_NAME "ESP32-S3"
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
    // ESP32-S2
    #define DFPLAYER_RX_PIN 18
    #define DFPLAYER_TX_PIN 17
    #define DFPLAYER_UART_NUM 1
    #define MAX_PINS 8
    #define BOARD_NAME "ESP32-S2"
#elif defined(CONFIG_IDF_TARGET_ESP32)
    // ESP32 DevKit
    #define DFPLAYER_RX_PIN 16
    #define DFPLAYER_TX_PIN 17
    #define DFPLAYER_UART_NUM 2
    #define MAX_PINS 8
    #define BOARD_NAME "ESP32"
#else
    // Default fallback (ESP32-C3 pins)
    #define DFPLAYER_RX_PIN 20
    #define DFPLAYER_TX_PIN 21
    #define DFPLAYER_UART_NUM 1
    #define MAX_PINS 8
    #define BOARD_NAME "ESP32-Unknown"
    #warning "Unknown ESP32 variant, using ESP32-C3 default pins"
#endif

// Pin mode types
enum class PinMode {
    PIN_DISABLED = 0,
    OUTPUT_DIGITAL = 1,
    OUTPUT_PWM = 2,
    OUTPUT_WS2812B = 3,
    INPUT_DIGITAL = 4,
    INPUT_ANALOG = 5
};

// Effect types
enum class EffectType {
    EFFECT_NONE = 0,
    EFFECT_CANDLE_FLICKER = 1,
    EFFECT_FADE = 2,
    EFFECT_PULSE = 3,
    EFFECT_STROBE = 4,
    EFFECT_ENGINE_IDLE = 5,
    EFFECT_ENGINE_REV = 6,
    EFFECT_MACHINE_GUN = 7,
    EFFECT_FLAMETHROWER = 8,
    EFFECT_TAKING_HITS = 9,
    EFFECT_EXPLOSION = 10,
    EFFECT_ROCKET_LAUNCHER = 11,
    EFFECT_CONSOLE_RGB = 12,
    EFFECT_STATIC_ON = 13,
    EFFECT_STATIC_OFF = 14
};

// Audio mapping configuration
struct AudioMapping {
    uint8_t candleFlicker;     // Audio file for candle flicker effect
    uint8_t fade;              // Audio file for fade effect  
    uint8_t pulse;             // Audio file for pulse effect
    uint8_t strobe;            // Audio file for strobe effect
    uint8_t engineIdle;        // Audio file for engine idle
    uint8_t engineRev;         // Audio file for engine rev
    uint8_t machineGun;        // Audio file for machine gun
    uint8_t flamethrower;      // Audio file for flamethrower
    uint8_t takingHits;        // Audio file for taking hits
    uint8_t explosion;         // Audio file for explosion
    uint8_t rocketLauncher;    // Audio file for rocket launcher
    uint8_t killConfirmed;     // Audio file for kill confirmed
    
    AudioMapping() : candleFlicker(0), fade(0), pulse(0), strobe(0),
                     engineIdle(1), engineRev(6), machineGun(3), flamethrower(4),
                     takingHits(5), explosion(7), rocketLauncher(8), killConfirmed(9) {}
};

// Pin configuration structure
struct PinConfig {
    uint8_t gpio;
    PinMode mode;
    String name;
    bool enabled;
    uint8_t brightness;
    uint32_t color;
    EffectType effect;
    uint16_t effectSpeed;
    bool effectActive;
    uint8_t effectGroup;
    uint16_t ledCount;  // Number of LEDs for WS2812B strips
    EffectType defaultEffect;  // Effect to start automatically on boot
    String type;        // "Engine", "Weapon", "Candle", "Console", etc.
    String group;       // "Engine1", "Weapon1", "Candles", etc.
    
    PinConfig() : gpio(0), mode(PinMode::PIN_DISABLED), name("Unused"), 
                  enabled(false), brightness(255), color(0xFFFFFF),
                  effect(EffectType::EFFECT_NONE), effectSpeed(100), effectActive(false), effectGroup(0), ledCount(10),
                  defaultEffect(EffectType::EFFECT_NONE), type(""), group("") {}
};

// System configuration
struct SystemConfig {
    String deviceName;
    String wifiSSID;
    String wifiPassword;
    bool wifiEnabled;
    uint8_t volume;
    bool audioEnabled;
    uint8_t globalMaxBrightness;  // Global brightness ceiling (0-255)
    PinConfig pins[MAX_PINS];
    AudioMapping audioMap;
    
    SystemConfig() : deviceName("BattleAura"), wifiSSID(""), wifiPassword(""),
                     wifiEnabled(false), volume(15), audioEnabled(true), globalMaxBrightness(255) {}
};

// Global configuration
SystemConfig config;
bool configLoaded = false;

// GPIO state tracking
bool pinStates[MAX_PINS] = {false};

// Effect state tracking
struct EffectState {
    unsigned long lastUpdate;
    uint16_t step;
    uint8_t currentBrightness;
    bool direction;
    unsigned long effectEndTime; // When temporary effect should end (0 = permanent/ambient)
    EffectType savedAmbientEffect; // The ambient effect to restore after temporary effect ends
    bool hasActiveOverride; // True when a temporary effect is overriding the ambient effect
    
    EffectState() : lastUpdate(0), step(0), currentBrightness(0), direction(true), effectEndTime(0), 
                   savedAmbientEffect(EffectType::EFFECT_NONE), hasActiveOverride(false) {}
};

EffectState effectStates[MAX_PINS];

// Global objects
WebServer server(80);

// WS2812B LED strip objects (one per pin)
Adafruit_NeoPixel* neoPixels[MAX_PINS] = {nullptr};
const uint16_t DEFAULT_LED_COUNT = 10; // Default number of LEDs per strip

// DFPlayer Mini objects
HardwareSerial dfPlayerSerial(DFPLAYER_UART_NUM);
DFRobotDFPlayerMini dfPlayer;
bool audioInitialized = false;

// Audio playback tracking
bool audioCurrentlyPlaying = false;
unsigned long audioStartTime = 0;
unsigned long lastAudioCheck = 0;
const unsigned long AUDIO_CHECK_INTERVAL = 100; // Check audio status every 100ms
const unsigned long MAX_AUDIO_TIMEOUT = 30000; // Maximum audio duration timeout (30 seconds)

// Application state
bool systemReady = false;
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 10000; // 10 seconds

// Forward declarations
void setupSPIFFS();
void loadConfiguration();
void saveConfiguration();
void initializeDefaults();
void setupWiFi();
bool setupWiFiStation();
void setupWiFiAP();
void setupmDNS();
void setupWebServer();
void setupGPIO();
void startDefaultEffects();
void handleRoot();
void handleEmbeddedFile(const String& path);
void handleStaticFile(const String& path, const String& contentType);
void handleUpdateUpload();
void handleConfigSave();
void handlePinConfigSave();
void handleAudioMapSave();
String getEffectName(EffectType effect);
EffectType mapEffectNameToType(const String& effectName, PinMode mode);
void handleLedControl(bool state);
void initializeNeoPixel(uint8_t pinIndex);
void setNeoPixelColor(uint8_t pinIndex, uint32_t color, uint8_t brightness = 255);
void setNeoPixelLED(uint8_t pinIndex, uint8_t ledIndex, uint32_t color, uint8_t brightness = 255);
void setNeoPixelRange(uint8_t pinIndex, uint8_t startLED, uint8_t endLED, uint32_t color, uint8_t brightness = 255);
void setNeoPixelAll(uint8_t pinIndex, uint32_t color, uint8_t brightness = 255);
void clearNeoPixelStrip(uint8_t pinIndex);
void setNeoPixelState(uint8_t pinIndex, bool state);
void updateEffects();
void updateCandleFlicker(uint8_t pinIndex);
void updateEngineIdle(uint8_t pinIndex);
void updateEngineRev(uint8_t pinIndex);
void updatePulse(uint8_t pinIndex);
void updateStrobe(uint8_t pinIndex);
void updateMachineGun(uint8_t pinIndex);
void updateFlamethrower(uint8_t pinIndex);
void updateTakingHits(uint8_t pinIndex);
void updateExplosion(uint8_t pinIndex);
void updateRocketLauncher(uint8_t pinIndex);
void updateConsoleRGB(uint8_t pinIndex);
void updateFade(uint8_t pinIndex);
void startEffect(uint8_t pinIndex, EffectType effect);
void startAmbientEffect(uint8_t pinIndex, EffectType effect);
void startTemporaryEffect(uint8_t pinIndex, EffectType effect, unsigned long duration);
void restoreAmbientEffect(uint8_t pinIndex);
void stopEffect(uint8_t pinIndex);
void startGroupEffect(uint8_t group, EffectType effect, uint16_t duration = 0);
void stopGroupEffect(uint8_t group);
void setPinBrightness(uint8_t pinIndex, uint8_t brightness);
uint8_t calculateActualBrightness(uint8_t pinIndex);
void updateAllPinBrightness();
void setupAudio();
void playAudioFile(uint8_t fileNumber);
void playEffectAudio(EffectType effect);
void playScenarioAudio(const String& scenario);
void setAudioVolume(uint8_t volume);
void stopAudio();
bool isAudioReady();
uint16_t getEffectDuration(EffectType effect);
void checkPinConflicts();
bool isAudioStillPlaying();
void updateAudioBasedEffects();
void printSystemInfo();

void setup() {
    Serial.begin(115200);
    delay(1000); // Brief delay for serial stabilization
    
    Serial.println();
    Serial.println("==================================");  
    Serial.printf("BattleAura %s Starting...\n", VERSION);
    Serial.println("==================================");
    Serial.printf("Chip: %s\n", ESP.getChipModel());
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("CPU frequency: %d MHz\n", ESP.getCpuFreqMHz());
    
    // Initialize SPIFFS
    Serial.println("Setting up SPIFFS filesystem...");
    setupSPIFFS();
    
    // Load configuration
    Serial.println("Loading configuration...");
    loadConfiguration();
    
    // Initialize GPIO pins
    Serial.println("Setting up GPIO pins...");
    setupGPIO();
    
    // Start default effects
    Serial.println("Starting default effects...");
    startDefaultEffects();
    
    // Initialize Audio System
    if (config.audioEnabled) {
        Serial.println("Setting up DFPlayer Mini audio...");
        setupAudio();
        checkPinConflicts();
    } else {
        Serial.println("Audio system disabled");
    }
    
    // Initialize WiFi (Station mode with AP fallback)
    Serial.println("Setting up WiFi connection...");
    setupWiFi();
    
    // Initialize Web Server  
    Serial.println("Setting up Web Server...");
    setupWebServer();
    
    // Print final system info
    printSystemInfo();
    
    systemReady = true;
    Serial.println("==================================");
    Serial.println("🎯 SYSTEM READY!");
    Serial.println("==================================");
}

void loop() {
    // Handle web server requests (non-blocking)
    server.handleClient();
    
    // Update lighting effects (non-blocking)
    updateEffects();
    
    // Update audio-based effect timing
    updateAudioBasedEffects();
    
    // Periodic heartbeat (non-blocking timing)
    unsigned long now = millis();
    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
        Serial.printf("[%lu] Heartbeat - Free heap: %d bytes\n", 
                     now / 1000, ESP.getFreeHeap());
        lastHeartbeat = now;
    }
    
    // Small yield to prevent watchdog issues
    delay(1);
}

void setupWiFi() {
    WiFi.disconnect(true);
    delay(100);
    
    // Try station mode first if WiFi is enabled and configured
    if (config.wifiEnabled && config.wifiSSID.length() > 0) {
        if (setupWiFiStation()) {
            setupmDNS();
            return; // Successfully connected to station
        }
        Serial.println("Station mode failed, falling back to AP mode...");
    } else {
        Serial.println("WiFi station mode not configured, using AP mode");
    }
    
    // Fallback to AP mode
    setupWiFiAP();
    setupmDNS();
}

bool setupWiFiStation() {
    Serial.printf("Attempting to connect to WiFi: %s\n", config.wifiSSID.c_str());
    
    // Set hostname for DHCP before connecting
    String hostname = config.deviceName.length() > 0 ? config.deviceName : "battleaura";
    hostname.toLowerCase();
    hostname.replace(" ", "-");
    WiFi.setHostname(hostname.c_str());
    Serial.printf("Setting WiFi hostname: %s\n", hostname.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifiSSID.c_str(), config.wifiPassword.c_str());
    
    // Wait for connection with timeout
    unsigned long startAttemptTime = millis();
    const unsigned long timeout = 15000; // 15 second timeout
    
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("✓ Connected to WiFi: %s\n", config.wifiSSID.c_str());
        Serial.printf("✓ IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("✓ Signal strength: %d dBm\n", WiFi.RSSI());
        return true;
    } else {
        Serial.printf("✗ Failed to connect to WiFi: %s\n", config.wifiSSID.c_str());
        Serial.printf("  Status code: %d\n", WiFi.status());
        return false;
    }
}

void setupWiFiAP() {
    WiFi.mode(WIFI_AP);
    
    // Use configured device name as AP SSID if available
    String apName = config.deviceName.length() > 0 ? config.deviceName : AP_SSID;
    
    bool result = WiFi.softAP(apName.c_str(), AP_PASS, AP_CHANNEL);
    
    if (result) {
        Serial.printf("✓ WiFi AP created: %s\n", apName.c_str());
        Serial.printf("✓ Password: %s\n", AP_PASS); 
        Serial.printf("✓ IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("✗ Failed to create WiFi AP");
        Serial.println("  Continuing with limited functionality...");
    }
}

void setupmDNS() {
    String hostname = config.deviceName.length() > 0 ? config.deviceName : "battleaura";
    
    // Remove spaces and convert to lowercase for hostname
    hostname.toLowerCase();
    hostname.replace(" ", "-");
    
    if (MDNS.begin(hostname.c_str())) {
        Serial.printf("✓ mDNS responder started: %s.local\n", hostname.c_str());
        MDNS.addService("http", "tcp", 80);
        MDNS.addServiceTxt("http", "tcp", "version", VERSION);
        MDNS.addServiceTxt("http", "tcp", "device", "BattleAura");
    } else {
        Serial.println("✗ Failed to start mDNS responder");
    }
}

void setupWebServer() {
    // Root handler
    server.on("/", HTTP_GET, handleRoot);
    
    // Embedded static file handlers
    server.on("/style.css", HTTP_GET, []() { handleEmbeddedFile("/style.css"); });
    server.on("/app.js", HTTP_GET, []() { handleEmbeddedFile("/app.js"); });
    server.on("/config.html", HTTP_GET, []() { handleEmbeddedFile("/config.html"); });
    server.on("/update.html", HTTP_GET, []() { handleEmbeddedFile("/update.html"); });
    server.on("/audio_map.html", HTTP_GET, []() { handleEmbeddedFile("/audio_map.html"); });
    
    // API status endpoint
    server.on("/api/status", HTTP_GET, []() {
        JsonDocument statusDoc;
        statusDoc["version"] = VERSION;
        statusDoc["status"] = "READY";
        statusDoc["freeHeap"] = ESP.getFreeHeap();
        statusDoc["uptime"] = millis() / 1000;
        
        // Add GPIO pin status
        JsonArray pins = statusDoc["pins"].to<JsonArray>();
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (config.pins[i].enabled) {
                JsonObject pin = pins.add<JsonObject>();
                pin["gpio"] = config.pins[i].gpio;
                pin["name"] = config.pins[i].name;
                pin["mode"] = static_cast<int>(config.pins[i].mode);
                pin["state"] = pinStates[i];
                pin["enabled"] = true; // Since we only include enabled pins
            }
        }
        
        String response;
        serializeJson(statusDoc, response);
        server.send(200, "application/json", response);
    });
    
    // API endpoint to get current configuration as JSON
    server.on("/api/config", HTTP_GET, []() {
        JsonDocument configDoc;
        
        // System settings
        configDoc["deviceName"] = config.deviceName;
        configDoc["volume"] = config.volume;
        configDoc["audioEnabled"] = config.audioEnabled;
        configDoc["globalMaxBrightness"] = config.globalMaxBrightness;
        configDoc["wifiSSID"] = config.wifiSSID;
        configDoc["wifiPassword"] = config.wifiPassword;
        configDoc["wifiEnabled"] = config.wifiEnabled;
        
        // Pin configurations
        JsonArray pins = configDoc["pins"].to<JsonArray>();
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            JsonObject pin = pins.add<JsonObject>();
            pin["gpio"] = config.pins[i].gpio;
            pin["mode"] = static_cast<int>(config.pins[i].mode);
            pin["name"] = config.pins[i].name;
            pin["enabled"] = config.pins[i].enabled;
            pin["type"] = config.pins[i].type;
            pin["group"] = config.pins[i].group;
            pin["brightness"] = config.pins[i].brightness;
            pin["ledCount"] = config.pins[i].ledCount;
        }
        
        // Audio mapping configuration
        JsonObject audioMap = configDoc["audioMap"].to<JsonObject>();
        audioMap["candleFlicker"] = config.audioMap.candleFlicker;
        audioMap["fade"] = config.audioMap.fade;
        audioMap["pulse"] = config.audioMap.pulse;
        audioMap["strobe"] = config.audioMap.strobe;
        audioMap["engineIdle"] = config.audioMap.engineIdle;
        audioMap["engineRev"] = config.audioMap.engineRev;
        audioMap["machineGun"] = config.audioMap.machineGun;
        audioMap["flamethrower"] = config.audioMap.flamethrower;
        audioMap["takingHits"] = config.audioMap.takingHits;
        audioMap["explosion"] = config.audioMap.explosion;
        audioMap["rocketLauncher"] = config.audioMap.rocketLauncher;
        audioMap["killConfirmed"] = config.audioMap.killConfirmed;
        
        String response;
        serializeJson(configDoc, response);
        server.send(200, "application/json", response);
    });

    // API endpoint for configured pin types (for dynamic UI generation)
    server.on("/api/types", HTTP_GET, []() {
        JsonDocument typesDoc;
        JsonArray types = typesDoc["types"].to<JsonArray>();
        
        // Collect unique types from enabled pins
        String uniqueTypes[MAX_PINS];
        int typeCount = 0;
        
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (!config.pins[i].enabled || config.pins[i].type.length() == 0) continue;
            
            // Check if this type already exists
            bool found = false;
            for (int t = 0; t < typeCount; t++) {
                if (uniqueTypes[t].equals(config.pins[i].type)) {
                    found = true;
                    break;
                }
            }
            
            if (!found && typeCount < MAX_PINS) {
                uniqueTypes[typeCount] = config.pins[i].type;
                
                JsonObject typeObj = types.add<JsonObject>();
                typeObj["type"] = config.pins[i].type;
                typeObj["count"] = 1; // We'll update this below
                typeObj["hasRGB"] = (config.pins[i].mode == PinMode::OUTPUT_WS2812B);
                typeObj["hasPWM"] = (config.pins[i].mode == PinMode::OUTPUT_PWM);
                
                typeCount++;
            }
        }
        
        // Update counts and capabilities
        for (int t = 0; t < typeCount; t++) {
            int count = 0;
            bool hasRGB = false;
            bool hasPWM = false;
            
            for (uint8_t i = 0; i < MAX_PINS; i++) {
                if (config.pins[i].enabled && config.pins[i].type.equals(uniqueTypes[t])) {
                    count++;
                    if (config.pins[i].mode == PinMode::OUTPUT_WS2812B) hasRGB = true;
                    if (config.pins[i].mode == PinMode::OUTPUT_PWM) hasPWM = true;
                }
            }
            
            types[t]["count"] = count;
            types[t]["hasRGB"] = hasRGB;
            types[t]["hasPWM"] = hasPWM;
        }
        
        String response;
        serializeJson(typesDoc, response);
        server.send(200, "application/json", response);
    });
    
    
    // Favicon handler (prevents 404 errors)
    server.on("/favicon.ico", HTTP_GET, []() {
        server.send(204); // No Content
    });
    
    // Configuration pages
    server.on("/config", HTTP_GET, []() { handleEmbeddedFile("/config.html"); });
    server.on("/config", HTTP_POST, handleConfigSave);
    server.on("/config/device", HTTP_GET, []() { handleEmbeddedFile("/device.html"); });
    server.on("/config/pins", HTTP_GET, []() { handleEmbeddedFile("/pins.html"); });
    server.on("/config/pins", HTTP_POST, handlePinConfigSave);
    server.on("/config/effects", HTTP_GET, []() { handleEmbeddedFile("/effects.html"); });
    server.on("/config/system", HTTP_GET, []() { handleEmbeddedFile("/system.html"); });
    
    // OTA update page  
    server.on("/update", HTTP_GET, []() { handleEmbeddedFile("/update.html"); });
    server.on("/status", HTTP_GET, []() { handleEmbeddedFile("/system.html"); });
    server.on("/update", HTTP_POST, []() {
        server.send(200, F("text/html; charset=utf-8"), F(
            "<html><body style='font-family:Arial; background:#1a1a2e; color:white; text-align:center; padding:50px;'>"
            "<h1>✅ Update Successful!</h1>"
            "<p>Device will restart in 3 seconds...</p>"
            "<script>setTimeout(function(){ window.location='/'; }, 5000);</script>"
            "</body></html>"));
        delay(1000);
        ESP.restart();
    }, handleUpdateUpload);
    
    // System restart endpoint
    server.on("/restart", HTTP_POST, []() {
        server.send(200, F("text/html; charset=utf-8"), F(
            "<html><body style='font-family:Arial; background:#1a1a2e; color:white; text-align:center; padding:50px;'>"
            "<h1>🔄 Restarting Device...</h1>"
            "<p>Device will restart in 3 seconds to apply network configuration.</p>"
            "<p>Please wait for the device to reconnect to your network.</p>"
            "<script>setTimeout(function(){ window.location='/'; }, 8000);</script>"
            "</body></html>"));
        delay(1000);
        ESP.restart();
    });
    
    // GPIO control endpoints
    server.on("/led/on", HTTP_GET, []() { handleLedControl(true); });
    server.on("/led/off", HTTP_GET, []() { handleLedControl(false); });
    server.on("/led/toggle", HTTP_GET, []() { 
        // Find first enabled output pin and toggle its state
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (config.pins[i].enabled && 
                (config.pins[i].mode == PinMode::OUTPUT_DIGITAL || 
                 config.pins[i].mode == PinMode::OUTPUT_PWM ||
                 config.pins[i].mode == PinMode::OUTPUT_WS2812B)) {
                handleLedControl(!pinStates[i]);
                return;
            }
        }
        server.send(404, "text/plain", F("No output pins configured"));
    });
    
    // Effects control endpoints
    server.on("/effects/start", HTTP_GET, []() {
        if (!server.hasArg("pin") || !server.hasArg("effect")) {
            server.send(400, "text/plain", F("Missing pin or effect parameter"));
            return;
        }
        
        uint8_t pin = server.arg("pin").toInt();
        uint8_t effect = server.arg("effect").toInt();
        
        if (pin >= MAX_PINS) {
            server.send(400, "text/plain", F("Invalid pin number"));
            return;
        }
        
        if (!config.pins[pin].enabled) {
            server.send(400, "text/plain", F("Pin not enabled"));
            return;
        }
        
        startEffect(pin, (EffectType)effect);
        server.send(200, "text/plain", F("Effect started"));
    });
    
    server.on("/effects/stop", HTTP_GET, []() {
        if (!server.hasArg("pin")) {
            server.send(400, "text/plain", F("Missing pin parameter"));
            return;
        }
        
        uint8_t pin = server.arg("pin").toInt();
        
        if (pin >= MAX_PINS) {
            server.send(400, "text/plain", F("Invalid pin number"));
            return;
        }
        
        stopEffect(pin);
        server.send(200, "text/plain", F("Effect stopped"));
    });
    
    server.on("/effects/candle", HTTP_GET, []() {
        if (!server.hasArg("pin")) {
            server.send(400, "text/plain", F("Missing pin parameter"));
            return;
        }
        
        uint8_t pin = server.arg("pin").toInt();
        bool persistent = server.hasArg("persistent") && server.arg("persistent") == "true";
        
        if (pin >= MAX_PINS || !config.pins[pin].enabled) {
            server.send(400, "text/plain", F("Invalid or disabled pin"));
            return;
        }
        
        startEffect(pin, EffectType::EFFECT_CANDLE_FLICKER);
        
        if (persistent) {
            config.pins[pin].effect = EffectType::EFFECT_CANDLE_FLICKER;
            config.pins[pin].effectActive = true;
            saveConfiguration();
            server.send(200, "text/plain", F("Candle flicker effect started (persistent)"));
        } else {
            server.send(200, "text/plain", F("Candle flicker effect started"));
        }
    });
    
    server.on("/effects/configure", HTTP_GET, []() {
        if (!server.hasArg("pin") || !server.hasArg("effect") || !server.hasArg("active")) {
            server.send(400, "text/plain", F("Missing parameters: pin, effect, active"));
            return;
        }
        
        uint8_t pin = server.arg("pin").toInt();
        uint8_t effect = server.arg("effect").toInt();
        bool active = server.arg("active") == "true";
        uint16_t speed = server.hasArg("speed") ? server.arg("speed").toInt() : 100;
        
        if (pin >= MAX_PINS || !config.pins[pin].enabled) {
            server.send(400, "text/plain", F("Invalid or disabled pin"));
            return;
        }
        
        // Configure persistent effect settings
        config.pins[pin].effect = (EffectType)effect;
        config.pins[pin].effectActive = active;
        config.pins[pin].effectSpeed = speed;
        
        if (active) {
            startEffect(pin, (EffectType)effect);
        } else {
            stopEffect(pin);
        }
        
        saveConfiguration();
        server.send(200, "text/plain", F("Effect configuration saved"));
    });
    
    server.on("/effects/group", HTTP_GET, []() {
        if (!server.hasArg("group") || !server.hasArg("effect")) {
            server.send(400, "text/plain", F("Missing parameters: group, effect"));
            return;
        }
        
        uint8_t group = server.arg("group").toInt();
        uint8_t effect = server.arg("effect").toInt();
        uint16_t duration = server.hasArg("duration") ? server.arg("duration").toInt() : 0;
        
        if (group == 0) {
            server.send(400, "text/plain", F("Group 0 is reserved (no group)"));
            return;
        }
        
        startGroupEffect(group, (EffectType)effect, duration);
        
        String response = "Group " + String(group) + " effect " + String(effect) + " started";
        if (duration > 0) {
            response += " for " + String(duration) + "ms";
        }
        server.send(200, "text/plain", response);
    });
    
    server.on("/effects/group/stop", HTTP_GET, []() {
        if (!server.hasArg("group")) {
            server.send(400, "text/plain", F("Missing group parameter"));
            return;
        }
        
        uint8_t group = server.arg("group").toInt();
        stopGroupEffect(group);
        server.send(200, "text/plain", "Group " + String(group) + " effects stopped");
    });
    
    // Audio control endpoints
    server.on("/audio/play", HTTP_GET, []() {
        if (!server.hasArg("file")) {
            server.send(400, "text/plain", F("Missing file parameter"));
            return;
        }
        
        uint8_t fileNumber = server.arg("file").toInt();
        if (fileNumber == 0 || fileNumber > 255) {
            server.send(400, "text/plain", F("File number must be 1-255"));
            return;
        }
        
        if (!isAudioReady()) {
            server.send(503, "text/plain", F("Audio system not ready"));
            return;
        }
        
        playAudioFile(fileNumber);
        server.send(200, "text/plain", "Playing file " + String(fileNumber));
    });
    
    server.on("/audio/stop", HTTP_GET, []() {
        stopAudio();
        server.send(200, "text/plain", F("Audio stopped"));
    });
    
    server.on("/audio/volume", HTTP_GET, []() {
        if (!server.hasArg("level")) {
            server.send(400, "text/plain", F("Missing level parameter"));
            return;
        }
        
        uint8_t volume = server.arg("level").toInt();
        if (volume > 30) {
            server.send(400, "text/plain", F("Volume must be 0-30"));
            return;
        }
        
        setAudioVolume(volume);
        config.volume = volume;
        saveConfiguration();
        server.send(200, "text/plain", "Volume set to " + String(volume));
    });

    // Global volume control endpoint (for frontend compatibility)
    server.on("/volume", HTTP_GET, []() {
        if (!server.hasArg("value")) {
            server.send(400, "text/plain", F("Missing volume value (0-30)"));
            return;
        }
        
        uint8_t volume = constrain(server.arg("value").toInt(), 0, 30);
        setAudioVolume(volume);
        config.volume = volume;
        saveConfiguration();
        server.send(200, "text/plain", "Volume set to " + String(volume));
    });
    
    server.on("/audio/status", HTTP_GET, []() {
        String status = "{";
        status += "\"ready\": " + String(isAudioReady() ? "true" : "false") + ",";
        status += "\"enabled\": " + String(config.audioEnabled ? "true" : "false") + ",";
        status += "\"initialized\": " + String(audioInitialized ? "true" : "false") + ",";
        status += "\"volume\": " + String(config.volume) + ",";
        status += "\"rxPin\": " + String(DFPLAYER_RX_PIN) + ",";
        status += "\"txPin\": " + String(DFPLAYER_TX_PIN);
        
        // Try to get current state if initialized
        if (audioInitialized) {
            delay(50); // Brief delay for DFPlayer response
            
            // Get basic status (these are more reliable)
            int currentVol = dfPlayer.readVolume();
            status += ",\"currentVolume\": " + String(currentVol);
            
            // File count can freeze if SD card removed - skip for now to prevent freeze
            // TODO: Implement non-blocking SD card detection
            status += ",\"fileCount\": \"N/A - prevents freeze\"";
        }
        
        status += "}";
        server.send(200, "application/json", status);
    });
    
    server.on("/audio/reinit", HTTP_POST, []() {
        Serial.println("🔄 Manual DFPlayer reinitialization requested");
        setupAudio();
        server.send(200, "text/plain", isAudioReady() ? "Audio reinitialized successfully" : "Audio reinitialization failed");
    });
    
    server.on("/audio/scenario", HTTP_GET, []() {
        if (!server.hasArg("name")) {
            server.send(400, "text/plain", F("Missing name parameter"));
            return;
        }
        
        String scenario = server.arg("name");
        playScenarioAudio(scenario);
        server.send(200, "text/plain", "Playing scenario: " + scenario);
    });
    
    server.on("/audio/effect", HTTP_GET, []() {
        if (!server.hasArg("type")) {
            server.send(400, "text/plain", F("Missing type parameter"));
            return;
        }
        
        uint8_t effectType = server.arg("type").toInt();
        playEffectAudio((EffectType)effectType);
        server.send(200, "text/plain", "Playing effect audio: " + String(effectType));
    });
    
    server.on("/audio/map", HTTP_GET, []() { handleEmbeddedFile("/audio_map.html"); });
    server.on("/audio/map", HTTP_POST, handleAudioMapSave);
    
    // Global brightness control endpoint
    server.on("/brightness", HTTP_GET, []() {
        if (!server.hasArg("value")) {
            server.send(400, "text/plain", F("Missing brightness value (0-255)"));
            return;
        }
        
        uint8_t brightness = constrain(server.arg("value").toInt(), 0, 255);
        config.globalMaxBrightness = brightness;
        
        // Update all pin brightnesses based on new global max
        updateAllPinBrightness();
        
        // Save configuration
        saveConfiguration();
        
        server.send(200, "text/plain", "Global brightness set to " + String(brightness));
    });
    
    server.on("/brightness/pin", HTTP_GET, []() {
        if (!server.hasArg("pin") || !server.hasArg("value")) {
            server.send(400, "text/plain", F("Missing pin or brightness value"));
            return;
        }
        
        uint8_t pin = server.arg("pin").toInt();
        uint8_t brightness = constrain(server.arg("value").toInt(), 0, 255);
        
        if (pin >= MAX_PINS || !config.pins[pin].enabled) {
            server.send(400, "text/plain", F("Invalid or disabled pin"));
            return;
        }
        
        config.pins[pin].brightness = brightness;
        
        // Apply brightness immediately (effects will use updated value)
        if (config.pins[pin].mode == PinMode::OUTPUT_WS2812B) {
            uint8_t actualBrightness = calculateActualBrightness(pin);
            setNeoPixelColor(pin, config.pins[pin].color, actualBrightness);
        } else {
            setPinBrightness(pin, config.pins[pin].brightness);
        }
        
        // Save configuration
        saveConfiguration();
        
        uint8_t actualBrightness = calculateActualBrightness(pin);
        server.send(200, "text/plain", "Pin " + String(pin) + " brightness: " + String(brightness) + " (actual: " + String(actualBrightness) + ")");
    });
    
    // Individual pin effect control endpoint
    server.on("/pin/effect", HTTP_POST, []() {
        if (!server.hasArg("pin") || !server.hasArg("effect")) {
            server.send(400, "text/plain", F("Missing pin or effect parameter"));
            return;
        }
        
        uint8_t pin = server.arg("pin").toInt();
        String effect = server.arg("effect");
        
        if (pin >= MAX_PINS || !config.pins[pin].enabled) {
            server.send(400, "text/plain", F("Invalid or disabled pin"));
            return;
        }
        
        // Find the effect type based on name
        EffectType effectType = EffectType::EFFECT_STATIC_OFF;
        if (effect == "on") effectType = EffectType::EFFECT_STATIC_ON;
        else if (effect == "off") effectType = EffectType::EFFECT_STATIC_OFF;
        else if (effect == "flicker") effectType = EffectType::EFFECT_CANDLE_FLICKER;
        else if (effect == "pulse") effectType = EffectType::EFFECT_PULSE;
        else if (effect == "fade") effectType = EffectType::EFFECT_FADE;
        else if (effect == "strobe") effectType = EffectType::EFFECT_STROBE;
        else if (effect == "idle") effectType = EffectType::EFFECT_ENGINE_IDLE;
        else if (effect == "rev") effectType = EffectType::EFFECT_ENGINE_REV;
        else if (effect == "fire") effectType = EffectType::EFFECT_MACHINE_GUN;
        else if (effect == "scroll") effectType = EffectType::EFFECT_CONSOLE_RGB;
        
        // Start effect on specific pin
        startEffect(pin, effectType);
        
        server.send(200, "text/plain", "Pin " + String(pin) + " effect: " + effect);
    });
    
    // LED count endpoint for WS2812B strips
    server.on("/ledcount", HTTP_GET, []() {
        if (!server.hasArg("value")) {
            server.send(400, "text/plain", F("Missing LED count value (1-100)"));
            return;
        }
        
        uint16_t ledCount = constrain(server.arg("value").toInt(), 1, 100);
        bool found = false;
        
        // Update LED count on all enabled WS2812B pins
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (config.pins[i].enabled && config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                config.pins[i].ledCount = ledCount;
                // Reinitialize the NeoPixel with new count
                initializeNeoPixel(i);
                found = true;
                Serial.printf("Updated LED count to %d on pin %d (GPIO %d)\n", ledCount, i, config.pins[i].gpio);
            }
        }
        
        if (found) {
            // Save updated configuration
            saveConfiguration();
            server.send(200, "text/plain", "LED count updated to " + String(ledCount));
        } else {
            server.send(404, "text/plain", F("No WS2812B pins configured"));
        }
    });
    
    // Pin control endpoints
    server.on("/pin/toggle", HTTP_GET, []() {
        if (!server.hasArg("pin")) {
            server.send(400, "text/plain", F("Missing pin parameter"));
            return;
        }
        
        uint8_t pinIndex = server.arg("pin").toInt();
        if (pinIndex >= MAX_PINS || !config.pins[pinIndex].enabled) {
            server.send(400, "text/plain", F("Invalid or disabled pin"));
            return;
        }
        
        // Toggle pin state
        bool newState = !pinStates[pinIndex];
        pinStates[pinIndex] = newState;
        
        if (config.pins[pinIndex].mode == PinMode::OUTPUT_DIGITAL) {
            digitalWrite(config.pins[pinIndex].gpio, newState ? HIGH : LOW);
        } else if (config.pins[pinIndex].mode == PinMode::OUTPUT_PWM) {
            uint8_t brightness = newState ? calculateActualBrightness(pinIndex) : 0;
            analogWrite(config.pins[pinIndex].gpio, brightness);
        } else if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
            if (newState) {
                uint8_t brightness = calculateActualBrightness(pinIndex);
                setNeoPixelColor(pinIndex, config.pins[pinIndex].color, brightness);
            } else {
                setNeoPixelColor(pinIndex, 0x000000, 0);
            }
        }
        
        server.send(200, "text/plain", "Pin " + String(pinIndex) + " " + (newState ? "ON" : "OFF"));
    });
    
    server.on("/pin/color", HTTP_GET, []() {
        if (!server.hasArg("pin") || !server.hasArg("color")) {
            server.send(400, "text/plain", F("Missing pin or color parameter"));
            return;
        }
        
        uint8_t pinIndex = server.arg("pin").toInt();
        String color = server.arg("color");
        
        if (pinIndex >= MAX_PINS || !config.pins[pinIndex].enabled) {
            server.send(400, "text/plain", F("Invalid or disabled pin"));
            return;
        }
        
        if (config.pins[pinIndex].mode != PinMode::OUTPUT_WS2812B) {
            server.send(400, "text/plain", F("Pin is not WS2812B RGB"));
            return;
        }
        
        uint32_t colorValue = 0;
        if (color == "red") colorValue = 0xFF0000;
        else if (color == "green") colorValue = 0x00FF00;
        else if (color == "blue") colorValue = 0x0000FF;
        else if (color == "white") colorValue = 0xFFFFFF;
        else if (color == "off") colorValue = 0x000000;
        
        config.pins[pinIndex].color = colorValue;
        
        // Update color immediately if not running active effects
        uint8_t brightness = calculateActualBrightness(pinIndex);
        setNeoPixelColor(pinIndex, colorValue, brightness);
        
        // Save configuration
        saveConfiguration();
        
        server.send(200, "text/plain", "Pin " + String(pinIndex) + " color: " + color);
    });
    
    // TYPE-BASED EFFECT ENDPOINTS - Core modular system
    server.on("/effect", HTTP_POST, []() {
        if (!server.hasArg("type") || !server.hasArg("effect")) {
            server.send(400, "text/plain", F("Missing type or effect parameter"));
            return;
        }
        
        String type = server.arg("type");
        String effectName = server.arg("effect");
        String intent = server.hasArg("intent") ? server.arg("intent") : "auto"; // ambient, active, control, or auto
        uint16_t manualDuration = server.hasArg("duration") ? server.arg("duration").toInt() : 0;
        
        // Find all pins matching this type and determine the effect
        bool foundPins = false;
        EffectType effect = EffectType::EFFECT_NONE;
        
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (!config.pins[i].enabled || !config.pins[i].type.equalsIgnoreCase(type)) continue;
            
            foundPins = true;
            effect = mapEffectNameToType(effectName, config.pins[i].mode);
            break; // Just need one pin to determine the effect type
        }
        
        if (!foundPins) {
            server.send(404, "text/plain", "No enabled pins found for type: " + type);
            return;
        }
        
        if (effect == EffectType::EFFECT_NONE) {
            server.send(400, "text/plain", "Unknown effect: " + effectName);
            return;
        }
        
        // Determine if this should be ambient or temporary based on intent
        bool shouldBeAmbient = false;
        uint16_t effectDuration = 0;
        
        if (intent == "ambient") {
            shouldBeAmbient = true;
            effectDuration = 0; // Ambient effects run forever
        } else if (intent == "active" || intent == "control") {
            shouldBeAmbient = false;
            effectDuration = manualDuration > 0 ? manualDuration : getEffectDuration(effect);
        } else {
            // Auto mode - use effect's intrinsic duration to decide
            effectDuration = manualDuration > 0 ? manualDuration : getEffectDuration(effect);
            shouldBeAmbient = (effectDuration == 0);
        }
        
        // Special handling for audio-synced effects (0xFFFF)
        if (effectDuration == 0xFFFF) {
            effectDuration = MAX_AUDIO_TIMEOUT; // Set timeout as fallback, will be overridden by audio completion
            Serial.println("🎵 Effect will sync with audio playback duration");
        }
        
        // Play audio if available and enabled (but not for static control effects)
        if (config.audioEnabled && isAudioReady() && 
            effect != EffectType::EFFECT_STATIC_ON && effect != EffectType::EFFECT_STATIC_OFF) {
            playEffectAudio(effect);
            Serial.printf("🎵 Audio triggered for effect: %s\n", effectName.c_str());
        }
        
        // Apply effect to all matching pins using priority system
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (!config.pins[i].enabled || !config.pins[i].type.equalsIgnoreCase(type)) continue;
            
            if (shouldBeAmbient) {
                // Permanent/ambient effect
                startAmbientEffect(i, effect);
            } else {
                // Temporary effect - use priority system
                startTemporaryEffect(i, effect, effectDuration);
            }
            
            Serial.printf("Type effect: %s.%s applied to pin %d (GPIO %d)", 
                         type.c_str(), effectName.c_str(), i, config.pins[i].gpio);
            if (effectDuration > 0) {
                Serial.printf(" for %dms", effectDuration);
            } else {
                Serial.print(" (ambient)");
            }
            Serial.println();
        }
        
        String response = "Effect " + effectName + " applied to type: " + type;
        if (effectDuration > 0) {
            response += " (duration: " + String(effectDuration) + "ms)";
        }
        server.send(200, "text/plain", response);
    });
    
    // Global effect endpoints
    server.on("/global", HTTP_POST, []() {
        if (!server.hasArg("effect")) {
            server.send(400, "text/plain", F("Missing effect parameter"));
            return;
        }
        
        String effectName = server.arg("effect");
        
        if (effectName == "damage") {
            // Taking damage: all pins flash red briefly
            for (uint8_t i = 0; i < MAX_PINS; i++) {
                if (config.pins[i].enabled) {
                    if (config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                        setNeoPixelColor(i, 0xFF0000, 255); // Bright red
                    } else if (config.pins[i].mode == PinMode::OUTPUT_PWM) {
                        analogWrite(config.pins[i].gpio, 255); // Full brightness
                    }
                }
            }
            server.send(200, "text/plain", F("Global damage effect"));
            
        } else if (effectName == "victory") {
            // Victory: all pins bright green
            for (uint8_t i = 0; i < MAX_PINS; i++) {
                if (config.pins[i].enabled) {
                    if (config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                        setNeoPixelColor(i, 0x00FF00, 255); // Bright green
                    } else if (config.pins[i].mode == PinMode::OUTPUT_PWM) {
                        analogWrite(config.pins[i].gpio, 255); // Full brightness
                    }
                }
            }
            server.send(200, "text/plain", F("Global victory effect"));
            
        } else if (effectName == "alert") {
            // Alert: all pins flash amber
            for (uint8_t i = 0; i < MAX_PINS; i++) {
                if (config.pins[i].enabled) {
                    if (config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                        setNeoPixelColor(i, 0xFF8000, 255); // Amber
                    } else if (config.pins[i].mode == PinMode::OUTPUT_PWM) {
                        analogWrite(config.pins[i].gpio, 200); // High brightness
                    }
                }
            }
            server.send(200, "text/plain", F("Global alert effect"));
            
        } else if (effectName == "explosion") {
            // Explosion: all pins flash bright white/yellow briefly
            for (uint8_t i = 0; i < MAX_PINS; i++) {
                if (config.pins[i].enabled) {
                    if (config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                        setNeoPixelColor(i, 0xFFFFFF, 255); // Bright white
                    } else if (config.pins[i].mode == PinMode::OUTPUT_PWM) {
                        analogWrite(config.pins[i].gpio, 255); // Full brightness
                    }
                }
            }
            server.send(200, "text/plain", F("Global explosion effect"));
            
        } else if (effectName == "off") {
            // Turn off all effects
            for (uint8_t i = 0; i < MAX_PINS; i++) {
                if (config.pins[i].enabled) {
                    stopEffect(i);
                    if (config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                        setNeoPixelColor(i, 0x000000, 0); // Off
                    } else if (config.pins[i].mode == PinMode::OUTPUT_PWM) {
                        analogWrite(config.pins[i].gpio, 0); // Off
                    } else if (config.pins[i].mode == PinMode::OUTPUT_DIGITAL) {
                        digitalWrite(config.pins[i].gpio, LOW); // Off
                    }
                }
            }
            server.send(200, "text/plain", F("All effects stopped"));
            
        } else {
            server.send(400, "text/plain", "Unknown global effect: " + effectName);
        }
    });
    
    // Firmware update check endpoint
    server.on("/api/update/check", HTTP_GET, []() {
        if (!WiFi.isConnected()) {
            server.send(503, "application/json", F("{\"error\":\"WiFi not connected\"}"));
            return;
        }
        
        Serial.println("Checking for firmware updates...");
        
        WiFiClient client;
        HTTPClient http;
        
        // Check version from battlesync.me
        http.begin(client, "https://battlesync.me/api/battleaura/firmware");
        http.addHeader("User-Agent", String("BattleAura/") + VERSION);
        
        int httpCode = http.GET();
        JsonDocument response;
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            DeserializationError error = deserializeJson(response, payload);
            
            if (!error) {
                String latestVersion = response["version"] | "";
                String currentVersion = VERSION;
                
                response.clear();
                response["currentVersion"] = currentVersion;
                response["latestVersion"] = latestVersion;
                response["hasUpdate"] = (latestVersion.length() > 0 && latestVersion != currentVersion);
                
                if (response["hasUpdate"]) {
                    const char* downloadUrl = response["downloadUrl"];
                    if (!downloadUrl) {
                        response["downloadUrl"] = String("https://battlesync.me/api/battleaura/firmware/") + latestVersion + "/firmware.bin";
                    }
                    const char* changelog = response["changelog"];
                    if (!changelog) {
                        response["changelog"] = "Bug fixes and improvements";
                    }
                }
                
                String responseJson;
                serializeJson(response, responseJson);
                server.send(200, "application/json", responseJson);
            } else {
                server.send(500, "application/json", F("{\"error\":\"Failed to parse version response\"}"));
            }
        } else {
            // Fallback - no update available or server unreachable
            response["currentVersion"] = VERSION;
            response["latestVersion"] = "";
            response["hasUpdate"] = false;
            response["error"] = "Update server unreachable";
            
            String responseJson;
            serializeJson(response, responseJson);
            server.send(200, "application/json", responseJson);
        }
        
        http.end();
    });
    
    // Remote firmware update endpoint (requires explicit user confirmation)
    server.on("/api/update/download", HTTP_POST, []() {
        if (!server.hasArg("url") || !server.hasArg("confirmed")) {
            server.send(400, "application/json", F("{\"error\":\"Missing url or confirmation\"}"));
            return;
        }
        
        if (server.arg("confirmed") != "true") {
            server.send(400, "application/json", F("{\"error\":\"User confirmation required\"}"));
            return;
        }
        
        String firmwareUrl = server.arg("url");
        
        // Validate URL is from battlesync.me for security
        if (!firmwareUrl.startsWith("https://battlesync.me/")) {
            server.send(400, "application/json", F("{\"error\":\"Invalid firmware URL - only battlesync.me allowed\"}"));
            return;
        }
        
        Serial.println("Starting remote firmware update from: " + firmwareUrl);
        
        server.send(200, "application/json", F("{\"status\":\"starting\",\"message\":\"Downloading firmware...\"}"));
        
        WiFiClient client;
        HTTPClient http;
        
        http.begin(client, firmwareUrl);
        int httpCode = http.GET();
        
        if (httpCode == HTTP_CODE_OK) {
            int contentLength = http.getSize();
            
            if (contentLength > 0) {
                bool canBegin = Update.begin(contentLength);
                
                if (canBegin) {
                    WiFiClient* stream = http.getStreamPtr();
                    size_t written = Update.writeStream(*stream);
                    
                    if (written == contentLength) {
                        Serial.println("Written : " + String(written) + " successfully");
                    } else {
                        Serial.println("Written only : " + String(written) + "/" + String(contentLength));
                    }
                    
                    if (Update.end()) {
                        Serial.println("OTA done!");
                        if (Update.isFinished()) {
                            Serial.println("Update successfully completed. Rebooting.");
                            delay(1000);
                            ESP.restart();
                        } else {
                            Serial.println("Update not finished? Something went wrong!");
                        }
                    } else {
                        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
                    }
                } else {
                    Serial.println("Not enough space to begin OTA");
                }
            } else {
                Serial.println("There was no content in the response");
            }
        } else {
            Serial.println("HTTP error: " + String(httpCode));
        }
        
        http.end();
    });
    
    // Additional API endpoints for new frontend
    server.on("/api/audio/status", HTTP_GET, []() {
        String response = "{";
        response += "\"enabled\":" + String(config.audioEnabled ? "true" : "false") + ",";
        response += "\"ready\":" + String(isAudioReady() ? "true" : "false") + ",";
        
        // Get actual file count from DFPlayer
        int actualFileCount = 0;
        if (config.audioEnabled && audioInitialized) {
            actualFileCount = dfPlayer.readFileCounts();
            if (actualFileCount < 0) {
                actualFileCount = 0; // Handle error case
            }
        }
        response += "\"fileCount\":" + String(actualFileCount);
        response += "}";
        server.send(200, "application/json", response);
    });
    
    // Audio files list endpoint
    server.on("/api/audio/files", HTTP_GET, []() {
        JsonDocument doc;
        JsonArray files = doc["files"].to<JsonArray>();
        
        if (config.audioEnabled && audioInitialized) {
            int fileCount = dfPlayer.readFileCounts();
            if (fileCount > 0) {
                // Generate file list based on actual count
                for (int i = 1; i <= fileCount && i <= 255; i++) {
                    JsonObject file = files.add<JsonObject>();
                    file["number"] = i;
                    file["filename"] = String("") + (i < 10 ? "000" : i < 100 ? "00" : i < 1000 ? "0" : "") + String(i) + ".mp3";
                    
                    // Add known descriptions for common files
                    String description = "Audio File " + String(i);
                    switch (i) {
                        case 1: description = "Tank Idle"; break;
                        case 2: description = "Tank Idle 2"; break;
                        case 3: description = "Machine Gun"; break;
                        case 4: description = "Flamethrower"; break;
                        case 5: description = "Taking Hits"; break;
                        case 6: description = "Engine Revving"; break;
                        case 7: description = "Explosion"; break;
                        case 8: description = "Rocket Launcher"; break;
                        case 9: description = "Kill Confirmed"; break;
                    }
                    file["description"] = description;
                }
            }
        }
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });
    
    server.on("/api/check-update", HTTP_GET, []() {
        String response = "{";
        response += "\"currentVersion\":\"" + String(VERSION) + "\",";
        response += "\"latestVersion\":\"" + String(VERSION) + "\",";
        response += "\"updateAvailable\":false";
        response += "}";
        server.send(200, "application/json", response);
    });
    
    server.on("/api/diagnostics", HTTP_GET, []() {
        String response = "{\"results\":[";
        
        // WiFi diagnostics
        response += "{\"test\":\"WiFi\",\"status\":\"" + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "\"},";
        
        // Audio diagnostics with detailed status
        String audioStatus;
        if (!config.audioEnabled) {
            audioStatus = "Disabled in config";
        } else if (!audioInitialized) {
            audioStatus = "Not initialized";
        } else if (!isAudioReady()) {
            audioStatus = "Hardware not responding";
        } else {
            // Try to get file count to determine if SD card is present
            int fileCount = dfPlayer.readFileCounts();
            if (fileCount <= 0) {
                audioStatus = "Ready - No SD card or files";
            } else {
                audioStatus = "Ready - " + String(fileCount) + " files detected";
            }
        }
        response += "{\"test\":\"Audio\",\"status\":\"" + audioStatus + "\"},";
        
        // Memory diagnostics
        response += "{\"test\":\"Memory\",\"status\":\"" + String(ESP.getFreeHeap()/1024) + "KB free\"},";
        
        // Flash diagnostics
        response += "{\"test\":\"Flash\",\"status\":\"" + String(ESP.getFlashChipSize()/1024/1024) + "MB total\"}";
        
        response += "]}";
        server.send(200, "application/json", response);
    });
    
    server.on("/api/factory-reset", HTTP_POST, []() {
        server.send(200, "text/plain", "Factory reset initiated");
        delay(1000);
        ESP.restart();
    });
    
    server.on("/api/install-update", HTTP_POST, []() {
        server.send(200, "text/plain", "Update started");
    });
    
    server.on("/api/logs", HTTP_GET, []() {
        server.send(200, "text/plain", "BattleAura v" + String(VERSION) + "\nUptime: " + String(millis()/1000) + "s\nFree heap: " + String(ESP.getFreeHeap()) + " bytes");
    });
    
    server.on("/api/logs", HTTP_DELETE, []() {
        server.send(200, "text/plain", "Logs cleared");
    });
    
    server.on("/api/restart", HTTP_POST, []() {
        server.send(200, "text/plain", "Device restarting");
        delay(1000);
        ESP.restart();
    });
    
    server.on("/api/test-pins", HTTP_POST, []() {
        server.send(200, "text/plain", "Pin testing removed - use individual pin controls instead");
    });
    
    server.on("/api/uptime", HTTP_GET, []() {
        server.send(200, "text/plain", String(millis()/1000) + "s");
    });
    
    // Available effects endpoint - optimized for flash usage
    server.on("/api/effects", HTTP_GET, []() {
        // Use more compact JSON structure to save flash space
        server.send(200, "application/json", F(
        "{\"effects\":["
        "{\"id\":\"off\",\"name\":\"Off\",\"icon\":\"⚫\",\"categories\":[\"basic\"]},"
        "{\"id\":\"on\",\"name\":\"On\",\"icon\":\"💡\",\"categories\":[\"basic\"]},"
        "{\"id\":\"flicker\",\"name\":\"Candle Flicker\",\"icon\":\"🕯️\",\"categories\":[\"candle\",\"ambient\"]},"
        "{\"id\":\"pulse\",\"name\":\"Pulse\",\"icon\":\"💓\",\"categories\":[\"generic\",\"console\"]},"
        "{\"id\":\"fade\",\"name\":\"Fade\",\"icon\":\"🌙\",\"categories\":[\"generic\"]},"
        "{\"id\":\"strobe\",\"name\":\"Strobe\",\"icon\":\"⚡\",\"categories\":[\"generic\",\"damage\"]},"
        "{\"id\":\"idle\",\"name\":\"Engine Idle\",\"icon\":\"🔄\",\"categories\":[\"engine\",\"ambient\"]},"
        "{\"id\":\"rev\",\"name\":\"Engine Rev\",\"icon\":\"⚡\",\"categories\":[\"engine\",\"active\"]},"
        "{\"id\":\"fire\",\"name\":\"Machine Gun\",\"icon\":\"🔫\",\"categories\":[\"weapon\",\"gun\",\"active\"]},"
        "{\"id\":\"flamethrower\",\"name\":\"Flamethrower\",\"icon\":\"🔥\",\"categories\":[\"weapon\",\"active\"]},"
        "{\"id\":\"rocket\",\"name\":\"Rocket Launcher\",\"icon\":\"🚀\",\"categories\":[\"weapon\",\"active\"]},"
        "{\"id\":\"damage\",\"name\":\"Taking Hits\",\"icon\":\"⚡\",\"categories\":[\"damage\",\"active\"]},"
        "{\"id\":\"explosion\",\"name\":\"Explosion\",\"icon\":\"💥\",\"categories\":[\"damage\",\"weapon\",\"cannon\",\"active\"]},"
        "{\"id\":\"scroll\",\"name\":\"Data Scroll\",\"icon\":\"📊\",\"categories\":[\"console\",\"rgb\",\"ambient\"]}"
        "]}")
        );
    });
    
    server.on("/volume/", HTTP_GET, []() {
        String uri = server.uri();
        String volumeStr = uri.substring(8); // Remove "/volume/"
        uint8_t volume = constrain(volumeStr.toInt(), 0, 30);
        setAudioVolume(volume);
        server.send(200, "text/plain", "Volume set to " + String(volume));
    });
    
    server.on("/test/effect/", HTTP_GET, []() {
        String uri = server.uri();
        String effect = uri.substring(13); // Remove "/test/effect/"
        server.send(200, "text/plain", "Testing effect: " + effect);
    });
    
    // Dynamic pin test endpoint
    server.onNotFound([]() {
        String uri = server.uri();
        if (uri.startsWith("/api/test-pin/") && server.method() == HTTP_POST) {
            String pinStr = uri.substring(14); // Remove "/api/test-pin/"
            server.send(200, "text/plain", "Pin testing removed - use pin effect controls instead");
            return;
        }
        server.send(404, "text/plain", F("Not found"));
    });
    
    server.begin();
    Serial.println("✓ Web server started on port 80");
}

// Helper function to map effect names to EffectType based on pin capabilities
EffectType mapEffectNameToType(const String& effectName, PinMode mode) {
    // Basic control
    if (effectName == "off") return EffectType::EFFECT_STATIC_OFF;
    if (effectName == "on") return EffectType::EFFECT_STATIC_ON;
    
    // Engine effects
    if (effectName == "idle") return EffectType::EFFECT_ENGINE_IDLE;
    if (effectName == "rev") return EffectType::EFFECT_ENGINE_REV;
    
    // Weapon effects  
    if (effectName == "fire" || effectName == "machinegun") return EffectType::EFFECT_MACHINE_GUN;
    if (effectName == "flamethrower" || effectName == "flame") return EffectType::EFFECT_FLAMETHROWER;
    if (effectName == "rocket" || effectName == "launcher") return EffectType::EFFECT_ROCKET_LAUNCHER;
    
    // Damage effects
    if (effectName == "damage" || effectName == "hit" || effectName == "taking_hits") return EffectType::EFFECT_TAKING_HITS;
    if (effectName == "explosion" || effectName == "explode") return EffectType::EFFECT_EXPLOSION;
    
    // Candle effects
    if (effectName == "flicker") return EffectType::EFFECT_CANDLE_FLICKER;
    
    // Generic effects
    if (effectName == "pulse") return EffectType::EFFECT_PULSE;
    if (effectName == "fade") return EffectType::EFFECT_FADE;
    if (effectName == "strobe") return EffectType::EFFECT_STROBE;
    
    // Console/RGB specific effects
    if (mode == PinMode::OUTPUT_WS2812B) {
        if (effectName == "scroll" || effectName == "data" || effectName == "console") return EffectType::EFFECT_CONSOLE_RGB;
    }
    
    return EffectType::EFFECT_NONE; // Unknown effect
}

String processTemplate(const String& content) {
    String result = content;
    
    // Replace template variables
    result.replace("{{DEVICE_NAME}}", config.deviceName);
    result.replace("{{STATUS}}", "READY");
    result.replace("{{BRIGHTNESS}}", String(config.globalMaxBrightness));
    result.replace("{{VOLUME}}", String(config.volume));
    result.replace("{{FIRMWARE_VERSION}}", VERSION);
    result.replace("{{WIFI_SSID}}", config.wifiSSID);
    result.replace("{{WIFI_PASSWORD}}", ""); // Don't expose password
    result.replace("{{WIFI_ENABLED_CHECKED}}", config.wifiEnabled ? "checked" : "");
    result.replace("{{AUDIO_ENABLED_CHECKED}}", config.audioEnabled ? "checked" : "");
    result.replace("{{UPTIME}}", String(millis()/1000) + "s");
    result.replace("{{FLASH_USAGE}}", String((ESP.getSketchSize() * 100) / ESP.getFreeSketchSpace()));
    result.replace("{{RAM_USAGE}}", String((ESP.getHeapSize() - ESP.getFreeHeap()) * 100 / ESP.getHeapSize()));
    result.replace("{{WIFI_STATUS}}", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    result.replace("{{AUDIO_STATUS}}", isAudioReady() ? "Ready" : "Not Ready");
    
    // Generate effect buttons based on configured pins
    if (result.indexOf("{{EFFECT_BUTTONS}}") != -1) {
        String buttons = "";
        buttons += "<button onclick=\"triggerEffect('engine')\">🔥 Engine</button>";
        buttons += "<button onclick=\"triggerEffect('weapon')\">💥 Weapon</button>";  
        buttons += "<button onclick=\"triggerEffect('candle')\">🕯️ Candle</button>";
        buttons += "<button onclick=\"triggerEffect('damage')\">💀 Taking Damage</button>";
        result.replace("{{EFFECT_BUTTONS}}", buttons);
    }
    
    // Generate pin slots (simplified)
    if (result.indexOf("{{PIN_SLOTS}}") != -1) {
        String pinSlots = "";
        for (uint8_t i = 0; i < MAX_PINS && i < 8; i++) { // Limit to avoid memory issues
            pinSlots += "<div class=\"pin-slot\">";
            pinSlots += "<h4>Pin " + String(i) + " (GPIO " + String(config.pins[i].gpio) + ")</h4>";
            pinSlots += "<label>Enabled: <input type=\"checkbox\" name=\"pin" + String(i) + "_enabled\"" + (config.pins[i].enabled ? " checked" : "") + "></label>";
            pinSlots += "</div>";
        }
        result.replace("{{PIN_SLOTS}}", pinSlots);
    }
    
    return result;
}

void handleRoot() {
    handleEmbeddedFile("/index.html");
}

void handleEmbeddedFile(const String& path) {
    Serial.printf("handleEmbeddedFile called with path: '%s'\n", path.c_str());
    
    // Look for the file in embedded files array
    for (size_t i = 0; i < embeddedFilesCount; i++) {
        Serial.printf("Checking embedded file [%d]: '%s'\n", i, embeddedFiles[i].path);
        if (String(embeddedFiles[i].path) == path) {
            Serial.printf("Found match! Sending %s with content-type: %s, length: %d\n", 
                         path.c_str(), embeddedFiles[i].contentType, embeddedFiles[i].length);
            
            // For HTML files, process templates
            if (String(embeddedFiles[i].contentType) == "text/html") {
                // Convert binary data to String properly
                char* buffer = new char[embeddedFiles[i].length + 1];
                memcpy_P(buffer, embeddedFiles[i].data, embeddedFiles[i].length);
                buffer[embeddedFiles[i].length] = '\0';
                String content = String(buffer);
                delete[] buffer;
                
                content = processTemplate(content);
                server.send(200, embeddedFiles[i].contentType, content);
            } else {
                // Use send_P to send binary data from PROGMEM with specified length
                server.send_P(200, embeddedFiles[i].contentType, embeddedFiles[i].data, embeddedFiles[i].length);
            }
            return;
        }
    }
    
    // File not found
    Serial.printf("Embedded file not found: %s\n", path.c_str());
    server.send(404, "text/plain", "File not found");
}


// handleUpdate removed - now uses embedded /update.html

void handleUpdateUpload() {
    HTTPUpload& upload = server.upload();
    static bool updateError = false;
    
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Starting firmware update: %s, size: %d\n", upload.filename.c_str(), upload.totalSize);
        updateError = false;
        
        // Begin update with known size if available, otherwise use max available space
        size_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) {
            Serial.println("Update.begin() failed");
            Update.printError(Serial);
            updateError = true;
        }
    } else if (upload.status == UPLOAD_FILE_WRITE && !updateError) {
        Serial.printf("Writing %d bytes...\n", upload.currentSize);
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Serial.println("Update.write() failed");
            Update.printError(Serial);
            updateError = true;
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (!updateError && Update.end(true)) {
            Serial.printf("Update complete: %u bytes written\n", upload.totalSize);
        } else {
            Serial.println("Update failed");
            Update.printError(Serial);
            updateError = true;
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Serial.println("Update aborted");
        Update.end();
        updateError = true;
    }
}

void setupGPIO() {
    Serial.println("=== GPIO Setup Debug ===");
    Serial.printf("Setting up %d pins...\n", MAX_PINS);
    
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        Serial.printf("Pin %d: GPIO=%d, enabled=%s, mode=%d, name='%s'\n", 
                     i, config.pins[i].gpio, config.pins[i].enabled ? "YES" : "NO", 
                     (int)config.pins[i].mode, config.pins[i].name.c_str());
        
        if (config.pins[i].enabled) {
            uint8_t gpio = config.pins[i].gpio;
            PinMode mode = config.pins[i].mode;
            
            switch (mode) {
                case PinMode::OUTPUT_DIGITAL:
                case PinMode::OUTPUT_PWM:
                    pinMode(gpio, OUTPUT);
                    digitalWrite(gpio, LOW);
                    pinStates[i] = false;
                    Serial.printf("✓ GPIO %d: %s (%s)\n", gpio, config.pins[i].name.c_str(),
                                 mode == PinMode::OUTPUT_DIGITAL ? "Digital" : "PWM");
                    break;
                case PinMode::INPUT_DIGITAL:
                    pinMode(gpio, INPUT_PULLUP);
                    Serial.printf("✓ GPIO %d: %s (Input Digital)\n", gpio, config.pins[i].name.c_str());
                    break;
                case PinMode::INPUT_ANALOG:
                    pinMode(gpio, INPUT);
                    Serial.printf("✓ GPIO %d: %s (Input Analog)\n", gpio, config.pins[i].name.c_str());
                    break;
                case PinMode::OUTPUT_WS2812B:
                    initializeNeoPixel(i);
                    pinStates[i] = false;
                    Serial.printf("✓ GPIO %d: %s (WS2812B - %d LEDs)\n", gpio, config.pins[i].name.c_str(), DEFAULT_LED_COUNT);
                    break;
                default:
                    break;
            }
            
            // Auto-start configured effects
            if (config.pins[i].effectActive && config.pins[i].effect != EffectType::EFFECT_NONE) {
                startEffect(i, config.pins[i].effect);
                Serial.printf("✓ Auto-started effect %d on GPIO %d\n", (int)config.pins[i].effect, gpio);
            }
        }
    }
}

void startDefaultEffects() {
    Serial.println("=== Starting Default Ambient Effects ===");
    
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        if (config.pins[i].enabled && config.pins[i].defaultEffect != EffectType::EFFECT_NONE) {
            Serial.printf("Starting ambient effect %d on pin %d (GPIO %d, %s)\n", 
                         (int)config.pins[i].defaultEffect, i, config.pins[i].gpio,
                         config.pins[i].type.c_str());
            startAmbientEffect(i, config.pins[i].defaultEffect);
        }
    }
    
    Serial.println("✓ All ambient effects initialized");
}

void handleLedControl(bool state) {
    Serial.printf("=== LED Control Debug ===\n");
    Serial.printf("Requested state: %s\n", state ? "ON" : "OFF");
    Serial.printf("Checking %d pins...\n", MAX_PINS);
    
    // Find first enabled output pin for compatibility
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        Serial.printf("Pin %d: GPIO=%d, enabled=%s, mode=%d\n", 
                     i, config.pins[i].gpio, config.pins[i].enabled ? "YES" : "NO", (int)config.pins[i].mode);
        
        if (config.pins[i].enabled && 
            (config.pins[i].mode == PinMode::OUTPUT_DIGITAL || 
             config.pins[i].mode == PinMode::OUTPUT_PWM ||
             config.pins[i].mode == PinMode::OUTPUT_WS2812B)) {
            
            Serial.printf("✓ Found valid pin %d (GPIO %d) - setting %s\n", i, config.pins[i].gpio, state ? "HIGH" : "LOW");
            pinStates[i] = state;
            
            if (config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                Serial.printf("Using WS2812B mode\n");
                setNeoPixelState(i, state);
            } else if (config.pins[i].mode == PinMode::OUTPUT_PWM) {
                Serial.printf("Using PWM mode\n");
                uint8_t brightness = state ? calculateActualBrightness(i) : 0;
                analogWrite(config.pins[i].gpio, state ? 255 : 0);
            } else {
                Serial.printf("Using Digital mode\n");
                digitalWrite(config.pins[i].gpio, state ? HIGH : LOW);
            }
            
            Serial.printf("GPIO %d (%s): %s\n", config.pins[i].gpio, config.pins[i].name.c_str(), state ? "ON" : "OFF");
            server.send(200, "text/plain", state ? F("LED ON") : F("LED OFF"));
            return;
        }
    }
    Serial.printf("✗ No valid output pins found!\n");
    server.send(404, "text/plain", F("No output pins configured"));
}

void setupSPIFFS() {
    if (!SPIFFS.begin(true)) {
        Serial.println("✗ SPIFFS initialization failed");
        Serial.println("  Continuing with defaults...");
        return;
    }
    
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    Serial.printf("✓ SPIFFS initialized: %d KB total, %d KB used\n", 
                  totalBytes / 1024, usedBytes / 1024);
}

void initializeDefaults() {
    config = SystemConfig();  // Reset to defaults
    
    // All pins disabled by default - user must configure their actual hardware
    // This prevents accidentally driving wrong hardware types (buttons, sensors, relays, etc.)
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        config.pins[i].gpio = 0;
        config.pins[i].mode = PinMode::PIN_DISABLED;
        config.pins[i].name = "Unused";
        config.pins[i].enabled = false;
        config.pins[i].brightness = 255;
        config.pins[i].color = 0xFFFFFF;
        config.pins[i].defaultEffect = EffectType::EFFECT_NONE;
        config.pins[i].type = "";
        config.pins[i].group = "";
    }
    
    Serial.println("✓ Safe default configuration - all pins disabled, user must configure hardware");
}

void loadConfiguration() {
    if (!SPIFFS.exists(CONFIG_FILE)) {
        Serial.println("⚠ Config file not found - initializing defaults");
        initializeDefaults();
        configLoaded = false;
        return;
    }
    
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("✗ Failed to open config file - using defaults");
        configLoaded = false;
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("✗ Failed to parse config: %s - reinitializing defaults\n", error.c_str());
        initializeDefaults();
        configLoaded = false;
        return;
    }
    
    // Load configuration from JSON
    config.deviceName = doc["deviceName"] | config.deviceName;
    config.wifiSSID = doc["wifiSSID"] | "";
    config.wifiPassword = doc["wifiPassword"] | "";
    config.wifiEnabled = doc["wifiEnabled"] | false;
    config.volume = doc["volume"] | 15;
    config.audioEnabled = doc["audioEnabled"] | true;
    config.globalMaxBrightness = doc["globalMaxBrightness"] | 255;
    
    // Load pin configurations
    if (doc["pins"].is<JsonArray>()) {
        for (uint8_t i = 0; i < MAX_PINS && i < doc["pins"].size(); i++) {
            config.pins[i].gpio = doc["pins"][i]["gpio"] | 0;
            config.pins[i].mode = static_cast<PinMode>(doc["pins"][i]["mode"] | 0);
            config.pins[i].name = doc["pins"][i]["name"] | "Unused";
            config.pins[i].enabled = doc["pins"][i]["enabled"] | false;
            config.pins[i].brightness = doc["pins"][i]["brightness"] | 255;
            config.pins[i].color = doc["pins"][i]["color"] | 0xFFFFFF;
            config.pins[i].effect = static_cast<EffectType>(doc["pins"][i]["effect"] | 0);
            config.pins[i].effectSpeed = doc["pins"][i]["effectSpeed"] | 100;
            config.pins[i].effectActive = doc["pins"][i]["effectActive"] | false;
            config.pins[i].effectGroup = doc["pins"][i]["effectGroup"] | 0;
            config.pins[i].ledCount = doc["pins"][i]["ledCount"] | 10;
            config.pins[i].defaultEffect = static_cast<EffectType>(doc["pins"][i]["defaultEffect"] | 0);
            config.pins[i].type = doc["pins"][i]["type"] | "";
            config.pins[i].group = doc["pins"][i]["group"] | "";
        }
    }
    
    // Load audio mapping
    if (doc["audioMap"].is<JsonObject>()) {
        config.audioMap.candleFlicker = doc["audioMap"]["candleFlicker"] | 0;
        config.audioMap.fade = doc["audioMap"]["fade"] | 0;
        config.audioMap.pulse = doc["audioMap"]["pulse"] | 0;
        config.audioMap.strobe = doc["audioMap"]["strobe"] | 0;
        config.audioMap.engineIdle = doc["audioMap"]["engineIdle"] | 1;
        config.audioMap.engineRev = doc["audioMap"]["engineRev"] | 6;
        config.audioMap.machineGun = doc["audioMap"]["machineGun"] | 3;
        config.audioMap.flamethrower = doc["audioMap"]["flamethrower"] | 4;
        config.audioMap.takingHits = doc["audioMap"]["takingHits"] | 5;
        config.audioMap.explosion = doc["audioMap"]["explosion"] | 7;
        config.audioMap.rocketLauncher = doc["audioMap"]["rocketLauncher"] | 8;
        config.audioMap.killConfirmed = doc["audioMap"]["killConfirmed"] | 9;
    }
    
    configLoaded = true;
    Serial.println("✓ Configuration loaded from file");
}

void saveConfiguration() {
    JsonDocument doc;
    
    doc["version"] = VERSION;
    doc["deviceName"] = config.deviceName;
    doc["wifiSSID"] = config.wifiSSID;
    doc["wifiPassword"] = config.wifiPassword;
    doc["wifiEnabled"] = config.wifiEnabled;
    doc["volume"] = config.volume;
    doc["audioEnabled"] = config.audioEnabled;
    doc["globalMaxBrightness"] = config.globalMaxBrightness;
    
    JsonArray pins = doc["pins"].to<JsonArray>();
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        JsonObject pin = pins.add<JsonObject>();
        pin["gpio"] = config.pins[i].gpio;
        pin["mode"] = static_cast<uint8_t>(config.pins[i].mode);
        pin["name"] = config.pins[i].name;
        pin["enabled"] = config.pins[i].enabled;
        pin["brightness"] = config.pins[i].brightness;
        pin["color"] = config.pins[i].color;
        pin["effect"] = static_cast<uint8_t>(config.pins[i].effect);
        pin["effectSpeed"] = config.pins[i].effectSpeed;
        pin["effectActive"] = config.pins[i].effectActive;
        pin["effectGroup"] = config.pins[i].effectGroup;
        pin["ledCount"] = config.pins[i].ledCount;
        pin["defaultEffect"] = static_cast<uint8_t>(config.pins[i].defaultEffect);
        pin["type"] = config.pins[i].type;
        pin["group"] = config.pins[i].group;
    }
    
    // Save audio mapping
    JsonObject audioMap = doc["audioMap"].to<JsonObject>();
    audioMap["candleFlicker"] = config.audioMap.candleFlicker;
    audioMap["fade"] = config.audioMap.fade;
    audioMap["pulse"] = config.audioMap.pulse;
    audioMap["strobe"] = config.audioMap.strobe;
    audioMap["engineIdle"] = config.audioMap.engineIdle;
    audioMap["engineRev"] = config.audioMap.engineRev;
    audioMap["machineGun"] = config.audioMap.machineGun;
    audioMap["flamethrower"] = config.audioMap.flamethrower;
    audioMap["takingHits"] = config.audioMap.takingHits;
    audioMap["explosion"] = config.audioMap.explosion;
    audioMap["rocketLauncher"] = config.audioMap.rocketLauncher;
    audioMap["killConfirmed"] = config.audioMap.killConfirmed;
    
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("✗ Failed to save config file");
        return;
    }
    
    size_t bytesWritten = serializeJson(doc, file);
    file.close();
    
    Serial.printf("✓ Configuration saved (%d bytes)\n", bytesWritten);
}

void handleConfigSave() {
    // Parse form data and update configuration
    if (server.hasArg("deviceName")) {
        config.deviceName = server.arg("deviceName");
    }
    
    if (server.hasArg("volume")) {
        config.volume = constrain(server.arg("volume").toInt(), 0, 30);
    }
    
    config.audioEnabled = server.hasArg("audioEnabled");
    config.wifiEnabled = server.hasArg("wifiEnabled");
    
    if (server.hasArg("wifiSSID")) {
        config.wifiSSID = server.arg("wifiSSID");
    }
    
    if (server.hasArg("wifiPassword")) {
        config.wifiPassword = server.arg("wifiPassword");
    }
    
    // Update pin configurations with new modular fields
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        String gpioArg = "gpio" + String(i);
        String modeArg = "mode" + String(i);
        String nameArg = "name" + String(i);
        String enabledArg = "enabled" + String(i);
        String typeArg = "type" + String(i);
        String groupArg = "group" + String(i);
        String brightnessArg = "brightness" + String(i);
        String ledCountArg = "ledCount" + String(i);
        
        if (server.hasArg(gpioArg)) {
            uint8_t requestedGPIO = server.arg(gpioArg).toInt();
            // Prevent assignment of DFPlayer pins when audio is enabled
            if (config.audioEnabled && (requestedGPIO == DFPLAYER_RX_PIN || requestedGPIO == DFPLAYER_TX_PIN)) {
                Serial.printf("⚠ GPIO %d blocked - reserved for DFPlayer (Audio enabled)\n", requestedGPIO);
                // Keep existing GPIO or assign a safe default
                if (config.pins[i].gpio == DFPLAYER_RX_PIN || config.pins[i].gpio == DFPLAYER_TX_PIN) {
                    config.pins[i].gpio = 2; // Safe default
                }
            } else {
                config.pins[i].gpio = constrain(requestedGPIO, 0, 21);
            }
        }
        
        if (server.hasArg(modeArg)) {
            config.pins[i].mode = static_cast<PinMode>(server.arg(modeArg).toInt());
        }
        
        if (server.hasArg(nameArg)) {
            config.pins[i].name = server.arg(nameArg);
        }
        
        // NEW: Handle type and group fields
        if (server.hasArg(typeArg)) {
            String typeValue = server.arg(typeArg);
            if (typeValue == "Custom") {
                // Use custom type field if "Custom" is selected
                String customTypeArg = "customType" + String(i);
                if (server.hasArg(customTypeArg)) {
                    config.pins[i].type = server.arg(customTypeArg);
                } else {
                    config.pins[i].type = ""; // Clear if no custom type provided
                }
            } else {
                config.pins[i].type = typeValue;
            }
        }
        
        if (server.hasArg(groupArg)) {
            config.pins[i].group = server.arg(groupArg);
        }
        
        // NEW: Handle per-pin brightness
        if (server.hasArg(brightnessArg)) {
            config.pins[i].brightness = constrain(server.arg(brightnessArg).toInt(), 0, 255);
        }
        
        // NEW: Handle LED count for WS2812B
        if (server.hasArg(ledCountArg)) {
            config.pins[i].ledCount = constrain(server.arg(ledCountArg).toInt(), 1, 100);
        }
        
        config.pins[i].enabled = server.hasArg(enabledArg);
    }
    
    // Save configuration to file
    saveConfiguration();
    
    // Send success response
    server.send(200, F("text/html; charset=utf-8"), F(
        "<html><body style='font-family:Arial; background:#1a1a2e; color:white; text-align:center; padding:50px;'>"
        "<h1>✅ Configuration Saved!</h1>"
        "<p>Settings have been saved to flash memory.</p>"
        "<p>Some changes may require a restart to take effect.</p>"
        "<p><a href=\"/config\" style=\"color: #4CAF50;\">← Back to Configuration</a></p>"
        "<p><a href=\"/\" style=\"color: #ff6b35;\">← Back to Control Panel</a></p>"
        "</body></html>"));
    
    Serial.println("Configuration updated via web interface");
}

void handlePinConfigSave() {
    // Handle JSON pin configuration data
    String body = server.arg("plain");
    
    if (body.length() == 0) {
        server.send(400, "application/json", F("{\"error\":\"No data received\"}"));
        return;
    }
    
    // Parse JSON data
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        server.send(400, "application/json", F("{\"error\":\"Invalid JSON\"}"));
        return;
    }
    
    // Update pin configurations from JSON
    if (doc["pins"].is<JsonArray>()) {
        JsonArray pinsArray = doc["pins"];
        
        for (uint8_t i = 0; i < MAX_PINS && i < pinsArray.size(); i++) {
            JsonObject pinObj = pinsArray[i];
            
            if (pinObj["enabled"].is<bool>()) {
                config.pins[i].enabled = pinObj["enabled"];
            }
            
            if (pinObj["gpio"].is<int>()) {
                uint8_t requestedGPIO = pinObj["gpio"];
                // Prevent assignment of DFPlayer pins when audio is enabled
                if (config.audioEnabled && (requestedGPIO == DFPLAYER_RX_PIN || requestedGPIO == DFPLAYER_TX_PIN)) {
                    Serial.printf("⚠ GPIO %d blocked - reserved for DFPlayer (Audio enabled)\n", requestedGPIO);
                    // Keep existing GPIO or assign a safe default
                    if (config.pins[i].gpio == DFPLAYER_RX_PIN || config.pins[i].gpio == DFPLAYER_TX_PIN) {
                        config.pins[i].gpio = 2; // Safe default
                    }
                } else {
                    config.pins[i].gpio = constrain(requestedGPIO, 0, 21);
                }
            }
            
            if (pinObj["name"].is<const char*>()) {
                config.pins[i].name = pinObj["name"].as<String>();
            }
            
            if (pinObj["type"].is<const char*>()) {
                config.pins[i].type = pinObj["type"].as<String>();
            }
            
            if (pinObj["group"].is<const char*>()) {
                config.pins[i].group = pinObj["group"].as<String>();
            }
            
            if (pinObj["mode"].is<int>()) {
                config.pins[i].mode = static_cast<PinMode>(pinObj["mode"].as<int>());
            }
            
            if (pinObj["brightness"].is<int>()) {
                config.pins[i].brightness = constrain(pinObj["brightness"].as<int>(), 0, 255);
            }
            
            if (pinObj["ledCount"].is<int>()) {
                config.pins[i].ledCount = constrain(pinObj["ledCount"].as<int>(), 1, 100);
            }
        }
    }
    
    // Save configuration to file
    saveConfiguration();
    
    // Send success response
    server.send(200, "application/json", F("{\"success\":true,\"message\":\"Pin configuration saved\"}"));
    
    Serial.println("Pin configuration updated via web interface");
}

void handleAudioMapSave() {
    // Parse audio mapping form data and update configuration
    if (server.hasArg("candleFlicker")) {
        config.audioMap.candleFlicker = constrain(server.arg("candleFlicker").toInt(), 0, 255);
    }
    if (server.hasArg("fade")) {
        config.audioMap.fade = constrain(server.arg("fade").toInt(), 0, 255);
    }
    if (server.hasArg("pulse")) {
        config.audioMap.pulse = constrain(server.arg("pulse").toInt(), 0, 255);
    }
    if (server.hasArg("strobe")) {
        config.audioMap.strobe = constrain(server.arg("strobe").toInt(), 0, 255);
    }
    if (server.hasArg("engineIdle")) {
        config.audioMap.engineIdle = constrain(server.arg("engineIdle").toInt(), 0, 255);
    }
    if (server.hasArg("engineRev")) {
        config.audioMap.engineRev = constrain(server.arg("engineRev").toInt(), 0, 255);
    }
    if (server.hasArg("machineGun")) {
        config.audioMap.machineGun = constrain(server.arg("machineGun").toInt(), 0, 255);
    }
    if (server.hasArg("flamethrower")) {
        config.audioMap.flamethrower = constrain(server.arg("flamethrower").toInt(), 0, 255);
    }
    if (server.hasArg("takingHits")) {
        config.audioMap.takingHits = constrain(server.arg("takingHits").toInt(), 0, 255);
    }
    if (server.hasArg("explosion")) {
        config.audioMap.explosion = constrain(server.arg("explosion").toInt(), 0, 255);
    }
    if (server.hasArg("rocketLauncher")) {
        config.audioMap.rocketLauncher = constrain(server.arg("rocketLauncher").toInt(), 0, 255);
    }
    if (server.hasArg("killConfirmed")) {
        config.audioMap.killConfirmed = constrain(server.arg("killConfirmed").toInt(), 0, 255);
    }

    // Save configuration to file
    saveConfiguration();
    
    // Send success response
    server.send(200, F("text/html; charset=utf-8"), F(
        "<html><body style='font-family:Arial; background:#1a1a2e; color:white; text-align:center; padding:50px;'>"
        "<h1>✅ Audio Configuration Saved!</h1>"
        "<p>Audio-to-effect mappings have been saved to flash memory.</p>"
        "<p>Your custom audio mappings are now active.</p>"
        "<p><a href=\"/audio/map\" style=\"color: #4CAF50;\">← Back to Audio Configuration</a></p>"
        "<p><a href=\"/\" style=\"color: #ff6b35;\">← Back to Control Panel</a></p>"
        "</body></html>"));
    
    Serial.println("Audio mapping configuration updated via web interface");
}

void printSystemInfo() {
    Serial.println();
    Serial.println("System Configuration:");
    Serial.printf("  Device: %s\n", config.deviceName.c_str());
    
    // Show current WiFi status and connection info
    if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
        Serial.printf("  WiFi: Connected to %s\n", WiFi.SSID().c_str());
        Serial.printf("  IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("  Web Interface: http://%s/\n", WiFi.localIP().toString().c_str());
        
        // Show mDNS hostname if available
        String hostname = config.deviceName.length() > 0 ? config.deviceName : "battleaura";
        hostname.toLowerCase();
        hostname.replace(" ", "-");
        Serial.printf("  mDNS: http://%s.local/\n", hostname.c_str());
    } else if (WiFi.getMode() == WIFI_AP) {
        String apName = config.deviceName.length() > 0 ? config.deviceName : AP_SSID;
        Serial.printf("  WiFi: AP Mode (%s)\n", apName.c_str());
        Serial.printf("  IP Address: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.printf("  Web Interface: http://%s/\n", WiFi.softAPIP().toString().c_str());
    }
    
    Serial.printf("  Configuration: /config\n");
    Serial.printf("  Firmware Updates: /update\n");
    Serial.printf("  Board Type: %s\n", BOARD_NAME);
    
    // Show audio system status
    if (config.audioEnabled) {
        Serial.printf("  Audio: %s (DFPlayer Mini)\n", isAudioReady() ? "Ready" : "Not Ready");
        Serial.printf("  Audio Pins: RX=%d, TX=%d (UART%d)\n", DFPLAYER_RX_PIN, DFPLAYER_TX_PIN, DFPLAYER_UART_NUM);
        Serial.printf("  Audio Volume: %d/30\n", config.volume);
    } else {
        Serial.printf("  Audio: Disabled\n");
    }
    
    Serial.printf("  Config File: %s\n", configLoaded ? "Loaded" : "Using defaults");
    Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  Flash size: %d bytes\n", ESP.getFlashChipSize());
}

// WS2812B NeoPixel Functions

void initializeNeoPixel(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    // Clean up existing NeoPixel object if it exists
    if (neoPixels[pinIndex] != nullptr) {
        delete neoPixels[pinIndex];
        neoPixels[pinIndex] = nullptr;
    }
    
    // Create new NeoPixel object with configured LED count
    uint8_t gpio = config.pins[pinIndex].gpio;
    uint16_t ledCount = config.pins[pinIndex].ledCount;
    if (ledCount == 0) ledCount = DEFAULT_LED_COUNT; // Fallback to default
    
    neoPixels[pinIndex] = new Adafruit_NeoPixel(ledCount, gpio, NEO_GRB + NEO_KHZ800);
    
    if (neoPixels[pinIndex] != nullptr) {
        neoPixels[pinIndex]->begin();
        neoPixels[pinIndex]->clear();
        neoPixels[pinIndex]->show();
        Serial.printf("✓ WS2812B initialized on GPIO %d with %d LEDs\n", gpio, ledCount);
    } else {
        Serial.printf("✗ Failed to initialize WS2812B on GPIO %d\n", gpio);
    }
}

void setNeoPixelColor(uint8_t pinIndex, uint32_t color, uint8_t brightness) {
    if (pinIndex >= MAX_PINS || neoPixels[pinIndex] == nullptr) return;
    
    // Apply brightness scaling
    uint8_t r = (uint8_t)((color >> 16) & 0xFF);
    uint8_t g = (uint8_t)((color >> 8) & 0xFF);
    uint8_t b = (uint8_t)(color & 0xFF);
    
    r = (r * brightness) / 255;
    g = (g * brightness) / 255;
    b = (b * brightness) / 255;
    
    uint32_t scaledColor = neoPixels[pinIndex]->Color(r, g, b);
    
    // Set all LEDs to the same color
    uint16_t numLEDs = neoPixels[pinIndex]->numPixels();
    for (uint16_t i = 0; i < numLEDs; i++) {
        neoPixels[pinIndex]->setPixelColor(i, scaledColor);
    }
    neoPixels[pinIndex]->show();
}

void setNeoPixelState(uint8_t pinIndex, bool state) {
    if (pinIndex >= MAX_PINS || neoPixels[pinIndex] == nullptr) return;
    
    if (state) {
        // Turn on with configured color and brightness
        setNeoPixelColor(pinIndex, config.pins[pinIndex].color, config.pins[pinIndex].brightness);
    } else {
        // Turn off (clear all pixels)
        neoPixels[pinIndex]->clear();
        neoPixels[pinIndex]->show();
    }
}

void setNeoPixelLED(uint8_t pinIndex, uint8_t ledIndex, uint32_t color, uint8_t brightness) {
    if (pinIndex >= MAX_PINS || neoPixels[pinIndex] == nullptr) return;
    
    uint16_t numLEDs = neoPixels[pinIndex]->numPixels();
    if (ledIndex >= numLEDs) return;
    
    // Apply brightness scaling
    uint8_t r = (uint8_t)((color >> 16) & 0xFF);
    uint8_t g = (uint8_t)((color >> 8) & 0xFF);
    uint8_t b = (uint8_t)(color & 0xFF);
    
    r = (r * brightness) / 255;
    g = (g * brightness) / 255;
    b = (b * brightness) / 255;
    
    uint32_t scaledColor = neoPixels[pinIndex]->Color(r, g, b);
    neoPixels[pinIndex]->setPixelColor(ledIndex, scaledColor);
    neoPixels[pinIndex]->show();
}

void setNeoPixelRange(uint8_t pinIndex, uint8_t startLED, uint8_t endLED, uint32_t color, uint8_t brightness) {
    if (pinIndex >= MAX_PINS || neoPixels[pinIndex] == nullptr) return;
    
    uint16_t numLEDs = neoPixels[pinIndex]->numPixels();
    if (startLED >= numLEDs || endLED >= numLEDs || startLED > endLED) return;
    
    // Apply brightness scaling
    uint8_t r = (uint8_t)((color >> 16) & 0xFF);
    uint8_t g = (uint8_t)((color >> 8) & 0xFF);
    uint8_t b = (uint8_t)(color & 0xFF);
    
    r = (r * brightness) / 255;
    g = (g * brightness) / 255;
    b = (b * brightness) / 255;
    
    uint32_t scaledColor = neoPixels[pinIndex]->Color(r, g, b);
    
    for (uint8_t i = startLED; i <= endLED; i++) {
        neoPixels[pinIndex]->setPixelColor(i, scaledColor);
    }
    neoPixels[pinIndex]->show();
}

void setNeoPixelAll(uint8_t pinIndex, uint32_t color, uint8_t brightness) {
    // This is essentially the same as setNeoPixelColor, but explicit for clarity
    setNeoPixelColor(pinIndex, color, brightness);
}

void clearNeoPixelStrip(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS || neoPixels[pinIndex] == nullptr) return;
    
    neoPixels[pinIndex]->clear();
    neoPixels[pinIndex]->show();
}

// Effects System Functions

void updateEffects() {
    unsigned long now = millis();
    
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        if (!config.pins[i].enabled) continue;
        
        // Check if temporary effect has expired
        if (config.pins[i].effectActive && 
            effectStates[i].effectEndTime > 0 && 
            now >= effectStates[i].effectEndTime) {
            
            Serial.printf("Temporary effect expired on pin %d, restoring ambient\n", i);
            restoreAmbientEffect(i);
            continue;
        }
        
        if (config.pins[i].effectActive) {
            switch (config.pins[i].effect) {
                case EffectType::EFFECT_CANDLE_FLICKER:
                    updateCandleFlicker(i);
                    break;
                case EffectType::EFFECT_ENGINE_IDLE:
                    updateEngineIdle(i);
                    break;
                case EffectType::EFFECT_ENGINE_REV:
                    updateEngineRev(i);
                    break;
                case EffectType::EFFECT_PULSE:
                    updatePulse(i);
                    break;
                case EffectType::EFFECT_STROBE:
                    updateStrobe(i);
                    break;
                case EffectType::EFFECT_MACHINE_GUN:
                    updateMachineGun(i);
                    break;
                case EffectType::EFFECT_FLAMETHROWER:
                    updateFlamethrower(i);
                    break;
                case EffectType::EFFECT_TAKING_HITS:
                    updateTakingHits(i);
                    break;
                case EffectType::EFFECT_EXPLOSION:
                    updateExplosion(i);
                    break;
                case EffectType::EFFECT_ROCKET_LAUNCHER:
                    updateRocketLauncher(i);
                    break;
                case EffectType::EFFECT_CONSOLE_RGB:
                    updateConsoleRGB(i);
                    break;
                case EffectType::EFFECT_FADE:
                    updateFade(i);
                    break;
                default:
                    break;
            }
        }
    }
}

void updateCandleFlicker(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    unsigned long now = millis();
    uint16_t interval = map(config.pins[pinIndex].effectSpeed, 1, 255, 200, 20); // Speed control
    
    if (now - effectStates[pinIndex].lastUpdate >= interval) {
        effectStates[pinIndex].lastUpdate = now;
        
        // Generate random flicker values
        uint8_t baseBrightness = config.pins[pinIndex].brightness;
        uint8_t flickerRange = baseBrightness / 4; // 25% flicker range
        uint8_t minBrightness = baseBrightness - flickerRange;
        uint8_t maxBrightness = baseBrightness;
        
        if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
            uint8_t ledCount = config.pins[pinIndex].ledCount;
            
            if (ledCount <= 1) {
                // Single LED - simple orange flicker
                uint8_t newBrightness;
                if (random(100) < 85) {
                    newBrightness = random(baseBrightness - flickerRange/2, baseBrightness + 1);
                } else {
                    newBrightness = random(minBrightness, maxBrightness + 1);
                }
                newBrightness = constrain(newBrightness, minBrightness, maxBrightness);
                setNeoPixelColor(pinIndex, 0xFF6600, newBrightness); // Warm orange
            } else {
                // Multi-LED independent candle flicker
                uint32_t candleColors[] = {0xFF3300, 0xFF4400, 0xFF5500, 0xFF6600, 0xFF7700};
                
                for (uint8_t led = 0; led < ledCount; led++) {
                    // Each LED flickers independently
                    uint8_t ledBrightness;
                    if (random(100) < 85) {
                        ledBrightness = random(baseBrightness - flickerRange/2, baseBrightness + 1);
                    } else {
                        ledBrightness = random(minBrightness, maxBrightness + 1);
                    }
                    ledBrightness = constrain(ledBrightness, minBrightness, maxBrightness);
                    
                    // Slight color variation for each LED
                    uint32_t color = candleColors[random(0, 5)];
                    setNeoPixelLED(pinIndex, led, color, ledBrightness);
                }
            }
        } else {
            // PWM LED - original behavior
            uint8_t newBrightness;
            if (random(100) < 85) {
                newBrightness = random(baseBrightness - flickerRange/2, baseBrightness + 1);
            } else {
                newBrightness = random(minBrightness, maxBrightness + 1);
            }
            newBrightness = constrain(newBrightness, minBrightness, maxBrightness);
            
            setPinBrightness(pinIndex, newBrightness);
            effectStates[pinIndex].currentBrightness = newBrightness;
        }
    }
}

void updateEngineIdle(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    unsigned long now = millis();
    uint16_t interval = map(config.pins[pinIndex].effectSpeed, 1, 255, 800, 200); // Speed control
    
    if (now - effectStates[pinIndex].lastUpdate >= interval) {
        effectStates[pinIndex].lastUpdate = now;
        
        // Gentle engine idle pulse - breath-like pattern
        uint8_t baseBrightness = config.pins[pinIndex].brightness;
        float breathCycle = sin(effectStates[pinIndex].step * 0.1) * 0.3 + 0.7; // 0.4 to 1.0 range
        uint8_t newBrightness = baseBrightness * breathCycle;
        
        setPinBrightness(pinIndex, newBrightness);
        effectStates[pinIndex].currentBrightness = newBrightness;
        effectStates[pinIndex].step++;
        if (effectStates[pinIndex].step >= 63) effectStates[pinIndex].step = 0; // Full sine cycle
    }
}

void updateEngineRev(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    unsigned long now = millis();
    uint16_t interval = map(config.pins[pinIndex].effectSpeed, 1, 255, 100, 20); // Fast revving
    
    if (now - effectStates[pinIndex].lastUpdate >= interval) {
        effectStates[pinIndex].lastUpdate = now;
        
        // Aggressive engine rev - rapid brightness changes
        uint8_t baseBrightness = config.pins[pinIndex].brightness;
        uint8_t variation = random(baseBrightness * 0.6, baseBrightness + 1);
        
        setPinBrightness(pinIndex, variation);
        effectStates[pinIndex].currentBrightness = variation;
    }
}

void updatePulse(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    unsigned long now = millis();
    uint16_t interval = map(config.pins[pinIndex].effectSpeed, 1, 255, 100, 30);
    
    if (now - effectStates[pinIndex].lastUpdate >= interval) {
        effectStates[pinIndex].lastUpdate = now;
        
        // Smooth sine wave pulse
        uint8_t baseBrightness = config.pins[pinIndex].brightness;
        float pulseCycle = sin(effectStates[pinIndex].step * 0.2) * 0.5 + 0.5; // 0 to 1 range
        uint8_t newBrightness = baseBrightness * pulseCycle;
        
        if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
            // For RGB, pulse in the configured color
            setNeoPixelColor(pinIndex, config.pins[pinIndex].color, newBrightness);
        } else {
            setPinBrightness(pinIndex, newBrightness);
        }
        
        effectStates[pinIndex].currentBrightness = newBrightness;
        effectStates[pinIndex].step++;
        if (effectStates[pinIndex].step >= 32) effectStates[pinIndex].step = 0;
    }
}

void updateStrobe(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    unsigned long now = millis();
    uint16_t interval = map(config.pins[pinIndex].effectSpeed, 1, 255, 200, 50);
    
    if (now - effectStates[pinIndex].lastUpdate >= interval) {
        effectStates[pinIndex].lastUpdate = now;
        
        // Sharp on/off strobe
        uint8_t brightness = (effectStates[pinIndex].step % 2 == 0) ? config.pins[pinIndex].brightness : 0;
        
        if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
            setNeoPixelColor(pinIndex, config.pins[pinIndex].color, brightness);
        } else {
            setPinBrightness(pinIndex, brightness);
        }
        
        effectStates[pinIndex].currentBrightness = brightness;
        effectStates[pinIndex].step++;
    }
}

void updateMachineGun(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    unsigned long now = millis();
    
    if (now - effectStates[pinIndex].lastUpdate >= 80) { // Fast machine gun rate
        effectStates[pinIndex].lastUpdate = now;
        
        // Rapid fire pattern - 3 quick flashes, pause, repeat
        uint8_t brightness = 0;
        uint8_t cyclePos = effectStates[pinIndex].step % 8;
        
        if (cyclePos < 3) {
            brightness = config.pins[pinIndex].brightness; // Flash
        } else {
            brightness = config.pins[pinIndex].brightness * 0.1; // Dim between flashes
        }
        
        if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
            uint8_t ledCount = config.pins[pinIndex].ledCount;
            
            if (ledCount <= 1) {
                // Single LED - simple white flash
                setNeoPixelColor(pinIndex, 0xFFFFFF, brightness);
            } else {
                // Multi-LED muzzle flash effect
                clearNeoPixelStrip(pinIndex);
                
                if (cyclePos < 3) {
                    // During flash - create muzzle flash pattern
                    // Bright white center, orange/yellow edges
                    uint8_t flashRange = min(ledCount, (uint8_t)4);
                    
                    for (uint8_t led = 0; led < flashRange; led++) {
                        uint32_t color;
                        uint8_t ledBrightness;
                        
                        if (led == 0) {
                            // Center - bright white
                            color = 0xFFFFFF;
                            ledBrightness = brightness;
                        } else if (led == 1) {
                            // Near center - bright yellow
                            color = 0xFFFF00;
                            ledBrightness = brightness * 0.8;
                        } else {
                            // Edges - orange glow
                            color = 0xFF8800;
                            ledBrightness = brightness * 0.4;
                        }
                        
                        setNeoPixelLED(pinIndex, led, color, ledBrightness);
                    }
                }
            }
        } else {
            setPinBrightness(pinIndex, brightness);
        }
        
        effectStates[pinIndex].currentBrightness = brightness;
        effectStates[pinIndex].step++;
    }
}

void updateFlamethrower(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    unsigned long now = millis();
    
    if (now - effectStates[pinIndex].lastUpdate >= 120) {
        effectStates[pinIndex].lastUpdate = now;
        
        // Flickering flame effect
        uint8_t baseBrightness = config.pins[pinIndex].brightness;
        uint8_t flicker = random(baseBrightness * 0.6, baseBrightness + 1);
        
        if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
            uint8_t ledCount = config.pins[pinIndex].ledCount;
            
            if (ledCount <= 1) {
                // Single LED - simple color flicker
                uint32_t flameColors[] = {0xFF4500, 0xFF6600, 0xFF8800, 0xFFAA00};
                uint32_t color = flameColors[random(0, 4)];
                setNeoPixelColor(pinIndex, color, flicker);
            } else {
                // Multi-LED flame propagation effect
                uint32_t flameColors[] = {0xFF1100, 0xFF3300, 0xFF5500, 0xFF7700, 0xFF9900, 0xFFBB00};
                clearNeoPixelStrip(pinIndex);
                
                // Create flame pattern from base outward
                uint8_t flameLength = random(ledCount * 0.4, ledCount * 0.8);
                for (uint8_t led = 0; led < flameLength && led < ledCount; led++) {
                    // Flames get cooler (more orange/yellow) toward the tip
                    uint8_t colorIndex = map(led, 0, ledCount - 1, 0, 5);
                    uint8_t ledBrightness = map(led, 0, flameLength - 1, baseBrightness, baseBrightness * 0.3);
                    ledBrightness = random(ledBrightness * 0.7, ledBrightness + 1); // Individual flicker
                    
                    setNeoPixelLED(pinIndex, led, flameColors[colorIndex], ledBrightness);
                }
            }
        } else {
            setPinBrightness(pinIndex, flicker);
        }
        
        effectStates[pinIndex].currentBrightness = flicker;
    }
}

void updateTakingHits(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    unsigned long now = millis();
    
    if (now - effectStates[pinIndex].lastUpdate >= 150) {
        effectStates[pinIndex].lastUpdate = now;
        
        // Rapid red flashing damage effect
        uint8_t brightness = (effectStates[pinIndex].step % 2 == 0) ? config.pins[pinIndex].brightness : 0;
        
        if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
            uint8_t ledCount = config.pins[pinIndex].ledCount;
            
            if (ledCount <= 1) {
                // Single LED - simple red flash
                setNeoPixelColor(pinIndex, 0xFF0000, brightness);
            } else {
                // Multi-LED damage ripple effect
                clearNeoPixelStrip(pinIndex);
                
                if (brightness > 0) {
                    // Create damage wave that moves across the strip
                    uint8_t waveCenter = effectStates[pinIndex].step % (ledCount * 2);
                    uint32_t damageColors[] = {0xFF0000, 0xCC0000, 0x990000, 0x660000};
                    
                    for (uint8_t led = 0; led < ledCount; led++) {
                        // Calculate distance from wave center
                        uint8_t distance = abs((int8_t)led - (int8_t)(waveCenter % ledCount));
                        
                        if (distance <= 2) {
                            // Within damage wave
                            uint32_t color = damageColors[min(distance, (uint8_t)3)];
                            uint8_t ledBrightness = map(distance, 0, 2, brightness, brightness / 4);
                            setNeoPixelLED(pinIndex, led, color, ledBrightness);
                        }
                    }
                }
            }
        } else {
            setPinBrightness(pinIndex, brightness);
        }
        
        effectStates[pinIndex].currentBrightness = brightness;
        effectStates[pinIndex].step++;
    }
}

void updateExplosion(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    unsigned long now = millis();
    
    if (now - effectStates[pinIndex].lastUpdate >= 100) {
        effectStates[pinIndex].lastUpdate = now;
        
        // Bright flash that fades quickly
        uint8_t maxSteps = 10;
        uint8_t brightness = 0;
        
        if (effectStates[pinIndex].step < maxSteps) {
            // Fade from max to min
            brightness = config.pins[pinIndex].brightness * (maxSteps - effectStates[pinIndex].step) / maxSteps;
        }
        
        if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
            uint8_t ledCount = config.pins[pinIndex].ledCount;
            
            if (ledCount <= 1) {
                // Single LED - simple white flash
                setNeoPixelColor(pinIndex, 0xFFFFFF, brightness);
            } else {
                // Multi-LED explosion ripple effect
                clearNeoPixelStrip(pinIndex);
                
                if (effectStates[pinIndex].step < maxSteps) {
                    // Create expanding shockwave effect
                    uint8_t center = ledCount / 2;
                    uint8_t maxRadius = ledCount / 2 + 1;
                    uint8_t currentRadius = map(effectStates[pinIndex].step, 0, maxSteps - 1, 0, maxRadius);
                    
                    // White hot center fading to orange edges
                    for (uint8_t led = 0; led < ledCount; led++) {
                        uint8_t distance = abs((int8_t)led - (int8_t)center);
                        
                        if (distance <= currentRadius) {
                            uint32_t color;
                            uint8_t ledBrightness;
                            
                            if (distance == 0) {
                                // Center - bright white
                                color = 0xFFFFFF;
                                ledBrightness = brightness;
                            } else if (distance == 1) {
                                // Near center - yellow
                                color = 0xFFFF00;
                                ledBrightness = brightness * 0.8;
                            } else {
                                // Outer edge - orange/red
                                color = (distance == currentRadius) ? 0xFF4400 : 0xFF8800;
                                ledBrightness = brightness * 0.5;
                            }
                            
                            setNeoPixelLED(pinIndex, led, color, ledBrightness);
                        }
                    }
                }
            }
        } else {
            setPinBrightness(pinIndex, brightness);
        }
        
        effectStates[pinIndex].currentBrightness = brightness;
        effectStates[pinIndex].step++;
        
        if (effectStates[pinIndex].step > maxSteps + 5) {
            effectStates[pinIndex].step = 0; // Reset for next explosion
        }
    }
}

void updateRocketLauncher(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    unsigned long now = millis();
    
    if (now - effectStates[pinIndex].lastUpdate >= 200) {
        effectStates[pinIndex].lastUpdate = now;
        
        // Single bright flash followed by dim trail
        uint8_t cyclePos = effectStates[pinIndex].step % 6;
        uint8_t brightness = 0;
        
        if (cyclePos == 0) {
            brightness = config.pins[pinIndex].brightness; // Launch flash
        } else if (cyclePos < 4) {
            brightness = config.pins[pinIndex].brightness * (4 - cyclePos) / 4; // Fading trail
        }
        
        if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
            setNeoPixelColor(pinIndex, 0xFFAA00, brightness); // Orange rocket
        } else {
            setPinBrightness(pinIndex, brightness);
        }
        
        effectStates[pinIndex].currentBrightness = brightness;
        effectStates[pinIndex].step++;
    }
}

void updateConsoleRGB(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS || config.pins[pinIndex].mode != PinMode::OUTPUT_WS2812B) return;
    
    unsigned long now = millis();
    uint16_t interval = map(config.pins[pinIndex].effectSpeed, 1, 255, 500, 100);
    
    if (now - effectStates[pinIndex].lastUpdate >= interval) {
        effectStates[pinIndex].lastUpdate = now;
        
        uint8_t ledCount = config.pins[pinIndex].ledCount;
        uint8_t actualBrightness = calculateActualBrightness(pinIndex);
        
        if (ledCount <= 1) {
            // Single LED - just cycle colors
            uint32_t colors[] = {0x001100, 0x002200, 0x004400, 0x006600, 0x008800, 0x00AA00};
            uint8_t colorIndex = effectStates[pinIndex].step % 6;
            setNeoPixelColor(pinIndex, colors[colorIndex], actualBrightness);
        } else {
            // Multiple LEDs - create scrolling data effect
            uint32_t baseColors[] = {0x001100, 0x003300, 0x005500, 0x007700, 0x009900, 0x00BB00};
            
            // Clear the strip first
            clearNeoPixelStrip(pinIndex);
            
            // Create a scrolling pattern with varying intensities
            uint8_t offset = effectStates[pinIndex].step % (ledCount * 2);
            
            for (uint8_t led = 0; led < ledCount; led++) {
                // Create a wave pattern that moves across the strip
                int8_t wavePos = (led + offset) % (ledCount * 2);
                if (wavePos < ledCount) {
                    // Forward wave
                    uint8_t intensity = map(wavePos, 0, ledCount - 1, 255, 50);
                    uint8_t colorIndex = (led + (effectStates[pinIndex].step / 3)) % 6;
                    setNeoPixelLED(pinIndex, led, baseColors[colorIndex], 
                                 (intensity * actualBrightness) / 255);
                } else {
                    // Reverse wave (dimmer)
                    uint8_t intensity = map(wavePos - ledCount, 0, ledCount - 1, 50, 10);
                    uint8_t colorIndex = (led + (effectStates[pinIndex].step / 4)) % 6;
                    setNeoPixelLED(pinIndex, led, baseColors[colorIndex], 
                                 (intensity * actualBrightness) / 255);
                }
            }
        }
        
        effectStates[pinIndex].step++;
    }
}

void updateFade(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    unsigned long now = millis();
    uint16_t interval = map(config.pins[pinIndex].effectSpeed, 1, 255, 150, 50);
    
    if (now - effectStates[pinIndex].lastUpdate >= interval) {
        effectStates[pinIndex].lastUpdate = now;
        
        // Smooth fade in/out
        uint8_t maxSteps = 20;
        uint8_t brightness;
        
        if (effectStates[pinIndex].step <= maxSteps) {
            // Fade in
            brightness = config.pins[pinIndex].brightness * effectStates[pinIndex].step / maxSteps;
        } else {
            // Fade out
            brightness = config.pins[pinIndex].brightness * (2 * maxSteps - effectStates[pinIndex].step) / maxSteps;
        }
        
        if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
            setNeoPixelColor(pinIndex, config.pins[pinIndex].color, brightness);
        } else {
            setPinBrightness(pinIndex, brightness);
        }
        
        effectStates[pinIndex].currentBrightness = brightness;
        effectStates[pinIndex].step++;
        
        if (effectStates[pinIndex].step >= 2 * maxSteps) {
            effectStates[pinIndex].step = 0; // Reset cycle
        }
    }
}

void startEffect(uint8_t pinIndex, EffectType effect) {
    if (pinIndex >= MAX_PINS) return;
    
    // Handle static effects immediately
    if (effect == EffectType::EFFECT_STATIC_OFF) {
        stopEffect(pinIndex);
        pinStates[pinIndex] = false;
        if (config.pins[pinIndex].mode == PinMode::OUTPUT_DIGITAL) {
            digitalWrite(config.pins[pinIndex].gpio, LOW);
        } else if (config.pins[pinIndex].mode == PinMode::OUTPUT_PWM) {
            analogWrite(config.pins[pinIndex].gpio, 0);
        } else if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
            setNeoPixelColor(pinIndex, 0x000000, 0);
        }
        Serial.printf("Static OFF applied to pin %d\n", pinIndex);
        return;
    }
    
    if (effect == EffectType::EFFECT_STATIC_ON) {
        stopEffect(pinIndex);
        pinStates[pinIndex] = true;
        if (config.pins[pinIndex].mode == PinMode::OUTPUT_DIGITAL) {
            digitalWrite(config.pins[pinIndex].gpio, HIGH);
        } else if (config.pins[pinIndex].mode == PinMode::OUTPUT_PWM) {
            uint8_t brightness = calculateActualBrightness(pinIndex);
            analogWrite(config.pins[pinIndex].gpio, brightness);
        } else if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
            uint8_t brightness = calculateActualBrightness(pinIndex);
            setNeoPixelColor(pinIndex, config.pins[pinIndex].color, brightness);
        }
        Serial.printf("Static ON applied to pin %d\n", pinIndex);
        return;
    }
    
    // Handle dynamic effects
    config.pins[pinIndex].effect = effect;
    config.pins[pinIndex].effectActive = true;
    
    // Initialize effect state with randomization to prevent sync
    effectStates[pinIndex].lastUpdate = millis() + random(0, 500); // Random 0-500ms offset
    effectStates[pinIndex].step = random(0, 100); // Random starting step
    effectStates[pinIndex].currentBrightness = config.pins[pinIndex].brightness;
    effectStates[pinIndex].direction = random(0, 2) == 0; // Random initial direction
    
    Serial.printf("Started effect %d on pin %d (offset: %dms)\n", 
                 (int)effect, pinIndex, effectStates[pinIndex].lastUpdate - millis());
}

void startAmbientEffect(uint8_t pinIndex, EffectType effect) {
    if (pinIndex >= MAX_PINS || effect == EffectType::EFFECT_NONE) return;
    
    // Set this as the ambient effect (runs indefinitely)
    effectStates[pinIndex].savedAmbientEffect = effect;
    effectStates[pinIndex].hasActiveOverride = false;
    effectStates[pinIndex].effectEndTime = 0; // Ambient effects run forever
    
    startEffect(pinIndex, effect);
    Serial.printf("Started ambient effect %d on pin %d\n", (int)effect, pinIndex);
}

void startTemporaryEffect(uint8_t pinIndex, EffectType effect, unsigned long duration) {
    if (pinIndex >= MAX_PINS || effect == EffectType::EFFECT_NONE) return;
    
    // Save current ambient effect if we haven't already
    if (!effectStates[pinIndex].hasActiveOverride) {
        if (config.pins[pinIndex].effectActive) {
            effectStates[pinIndex].savedAmbientEffect = config.pins[pinIndex].effect;
        } else {
            effectStates[pinIndex].savedAmbientEffect = config.pins[pinIndex].defaultEffect;
        }
        effectStates[pinIndex].hasActiveOverride = true;
        Serial.printf("Saving ambient effect %d for pin %d\n", 
                     (int)effectStates[pinIndex].savedAmbientEffect, pinIndex);
    }
    
    // Set end time for temporary effect
    effectStates[pinIndex].effectEndTime = (duration > 0) ? millis() + duration : 0;
    
    startEffect(pinIndex, effect);
    
    if (duration > 0) {
        Serial.printf("Started temporary effect %d on pin %d for %lums\n", 
                     (int)effect, pinIndex, duration);
    } else {
        Serial.printf("Started permanent effect %d on pin %d\n", (int)effect, pinIndex);
    }
}

void restoreAmbientEffect(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    // Clear the active override flag
    effectStates[pinIndex].hasActiveOverride = false;
    effectStates[pinIndex].effectEndTime = 0;
    
    // Restore the saved ambient effect
    EffectType ambientEffect = effectStates[pinIndex].savedAmbientEffect;
    if (ambientEffect == EffectType::EFFECT_NONE) {
        ambientEffect = config.pins[pinIndex].defaultEffect;
    }
    
    if (ambientEffect != EffectType::EFFECT_NONE) {
        startEffect(pinIndex, ambientEffect);
        Serial.printf("Restored ambient effect %d on pin %d\n", (int)ambientEffect, pinIndex);
    } else {
        stopEffect(pinIndex);
        Serial.printf("No ambient effect to restore on pin %d, stopping\n", pinIndex);
    }
}

void stopEffect(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    // Stop both active and ambient effects
    config.pins[pinIndex].effectActive = false;
    effectStates[pinIndex].hasActiveOverride = false;
    effectStates[pinIndex].effectEndTime = 0;
    
    // Reset effect state
    effectStates[pinIndex].step = 0;
    effectStates[pinIndex].lastUpdate = 0;
    
    // Reset to configured brightness and color for manual control
    if (config.pins[pinIndex].mode == PinMode::OUTPUT_WS2812B) {
        uint8_t brightness = calculateActualBrightness(pinIndex);
        setNeoPixelColor(pinIndex, config.pins[pinIndex].color, brightness);
    } else {
        setPinBrightness(pinIndex, config.pins[pinIndex].brightness);
    }
    
    Serial.printf("Stopped effect on pin %d - manual control active\n", pinIndex);
}

void setPinBrightness(uint8_t pinIndex, uint8_t brightness) {
    if (pinIndex >= MAX_PINS || !config.pins[pinIndex].enabled) return;
    
    PinMode mode = config.pins[pinIndex].mode;
    uint8_t gpio = config.pins[pinIndex].gpio;
    
    // Apply global brightness scaling
    uint8_t scaledBrightness = (brightness * config.globalMaxBrightness) / 255;
    
    switch (mode) {
        case PinMode::OUTPUT_PWM:
            analogWrite(gpio, scaledBrightness);
            break;
        case PinMode::OUTPUT_DIGITAL:
            digitalWrite(gpio, scaledBrightness > 128 ? HIGH : LOW);
            break;
        case PinMode::OUTPUT_WS2812B:
            if (neoPixels[pinIndex] != nullptr) {
                setNeoPixelColor(pinIndex, config.pins[pinIndex].color, scaledBrightness);
            }
            break;
        default:
            break;
    }
}

uint8_t calculateActualBrightness(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return 0;
    
    // Calculate: (globalMax * pinBrightness) / 255
    uint16_t actual = (config.globalMaxBrightness * config.pins[pinIndex].brightness) / 255;
    return (uint8_t)constrain(actual, 0, 255);
}

void updateAllPinBrightness() {
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        if (config.pins[i].enabled && !config.pins[i].effectActive) {
            uint8_t actualBrightness = calculateActualBrightness(i);
            setPinBrightness(i, actualBrightness);
        }
    }
}

// Group Effect Functions

void startGroupEffect(uint8_t group, EffectType effect, uint16_t duration) {
    if (group == 0) return; // Group 0 reserved for no group
    
    int pinsAffected = 0;
    
    // Start effect on all pins in the group
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        if (config.pins[i].enabled && config.pins[i].effectGroup == group) {
            startEffect(i, effect);
            pinsAffected++;
        }
    }
    
    Serial.printf("Started group %d effect %d on %d pins", group, (int)effect, pinsAffected);
    if (duration > 0) {
        Serial.printf(" for %dms", duration);
    }
    Serial.println();
}

void stopGroupEffect(uint8_t group) {
    if (group == 0) return; // Group 0 reserved for no group
    
    int pinsAffected = 0;
    
    // Stop effect on all pins in the group
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        if (config.pins[i].enabled && config.pins[i].effectGroup == group) {
            stopEffect(i);
            pinsAffected++;
        }
    }
    
    Serial.printf("Stopped group %d effects on %d pins\n", group, pinsAffected);
}

// DFPlayer Mini Audio Functions

void setupAudio() {
    if (!config.audioEnabled) {
        Serial.println("Audio system disabled in configuration");
        return;
    }
    
    Serial.printf("🎵 Initializing DFPlayer Mini on GPIO %d (RX) / %d (TX)...\n", DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
    
    // Initialize UART for DFPlayer
    dfPlayerSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
    Serial.println("  ✓ UART initialized at 9600 baud");
    
    // Extended initialization delay and retry logic
    Serial.println("  ⏳ Waiting for DFPlayer Mini to initialize...");
    delay(2000); // Increased delay for more reliable initialization
    
    // Try initializing DFPlayer with retries
    bool initSuccess = false;
    for (int attempt = 1; attempt <= 3; attempt++) {
        Serial.printf("  🔄 Initialization attempt %d/3...\n", attempt);
        
        if (dfPlayer.begin(dfPlayerSerial, true, true)) { // Use detailed feedback
            Serial.println("  ✓ DFPlayer Mini communication established");
            initSuccess = true;
            break;
        } else {
            Serial.printf("  ✗ Attempt %d failed\n", attempt);
            if (attempt < 3) {
                delay(1000); // Wait before retry
            }
        }
    }
    
    if (initSuccess) {
        Serial.println("✓ DFPlayer Mini initialized successfully");
        
        // Small delay before sending commands
        delay(500);
        
        // Set initial volume
        dfPlayer.volume(config.volume);
        delay(200); // Allow command to process
        Serial.printf("✓ Audio volume set to %d/30\n", config.volume);
        
        // Query available files (note: counts ALL files, not just MP3s)
        delay(200);
        int fileCount = dfPlayer.readFileCounts();
        if (fileCount > 0) {
            Serial.printf("✓ Found %d total files on SD card (includes non-MP3s)\n", fileCount);
            Serial.println("  DFPlayer counts ALL files - MP3s should be named 0001.mp3, 0002.mp3, etc.");
        } else {
            Serial.println("⚠ No files found - SD card issue");
            Serial.println("  Expected files: 0001.mp3, 0002.mp3, etc. in root directory");
        }
        
        // Test basic functionality
        delay(200);
        int currentVolume = dfPlayer.readVolume();
        Serial.printf("✓ Current volume readback: %d/30\n", currentVolume);
        
        audioInitialized = true;
        Serial.println("🎵 Audio system ready!");
        
    } else {
        Serial.println("✗ Failed to initialize DFPlayer Mini after 3 attempts");
        Serial.println("🔧 Troubleshooting checklist:");
        Serial.println("   1. Check wiring connections:");
        Serial.printf("      - DFPlayer RX → ESP32 GPIO%d\n", DFPLAYER_TX_PIN);
        Serial.printf("      - DFPlayer TX → ESP32 GPIO%d\n", DFPLAYER_RX_PIN);
        Serial.println("      - DFPlayer VCC → 3.3V or 5V");
        Serial.println("      - DFPlayer GND → GND");
        Serial.println("   2. Verify SD card is inserted and formatted (FAT32)");
        Serial.println("   3. Check audio files are named 0001.mp3, 0002.mp3, etc.");
        Serial.println("   4. Ensure stable power supply");
        Serial.println("   5. Try different DFPlayer module if available");
        audioInitialized = false;
    }
}

void playAudioFile(uint8_t fileNumber) {
    if (!audioInitialized || !config.audioEnabled) {
        Serial.printf("⚠ Audio not ready - cannot play file %d\n", fileNumber);
        return;
    }
    
    Serial.printf("🔊 Playing audio file %03d\n", fileNumber);
    dfPlayer.play(fileNumber);
    
    // Track audio playback start
    audioCurrentlyPlaying = true;
    audioStartTime = millis();
    Serial.printf("🎵 Audio playback tracking started at %lums\n", audioStartTime);
}

void setAudioVolume(uint8_t volume) {
    if (!audioInitialized) return;
    
    volume = constrain(volume, 0, 30); // DFPlayer volume range 0-30
    dfPlayer.volume(volume);
    Serial.printf("🔊 Audio volume set to %d/30\n", volume);
}

void stopAudio() {
    if (!audioInitialized) return;
    
    dfPlayer.stop();
    audioCurrentlyPlaying = false;
    Serial.println("🔇 Audio stopped, playback tracking reset");
}

bool isAudioReady() {
    return audioInitialized && config.audioEnabled;
}

void checkPinConflicts() {
    if (!config.audioEnabled) return;
    
    // Check if any configured pins conflict with DFPlayer pins
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        if (config.pins[i].enabled) {
            uint8_t gpio = config.pins[i].gpio;
            if (gpio == DFPLAYER_RX_PIN || gpio == DFPLAYER_TX_PIN) {
                Serial.printf("⚠ WARNING: Pin %d conflicts with DFPlayer (GPIO %d)\n", i, gpio);
                Serial.printf("  DFPlayer uses GPIO %d (RX) and %d (TX)\n", DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
                Serial.printf("  Disabling pin %d to avoid conflict\n", i);
                config.pins[i].enabled = false;
            }
        }
    }
}

// Audio Mapping Functions

void playEffectAudio(EffectType effect) {
    if (!isAudioReady()) return;
    
    uint8_t fileNumber = 0;
    
    switch (effect) {
        case EffectType::EFFECT_CANDLE_FLICKER:
            fileNumber = config.audioMap.candleFlicker;
            break;
        case EffectType::EFFECT_FADE:
            fileNumber = config.audioMap.fade;
            break;
        case EffectType::EFFECT_PULSE:
            fileNumber = config.audioMap.pulse;
            break;
        case EffectType::EFFECT_STROBE:
            fileNumber = config.audioMap.strobe;
            break;
        case EffectType::EFFECT_ENGINE_IDLE:
            fileNumber = config.audioMap.engineIdle;
            break;
        case EffectType::EFFECT_ENGINE_REV:
            fileNumber = config.audioMap.engineRev;
            break;
        case EffectType::EFFECT_MACHINE_GUN:
            fileNumber = config.audioMap.machineGun;
            break;
        case EffectType::EFFECT_FLAMETHROWER:
            fileNumber = config.audioMap.flamethrower;
            break;
        case EffectType::EFFECT_TAKING_HITS:
            fileNumber = config.audioMap.takingHits;
            break;
        case EffectType::EFFECT_EXPLOSION:
            fileNumber = config.audioMap.explosion;
            break;
        case EffectType::EFFECT_ROCKET_LAUNCHER:
            fileNumber = config.audioMap.rocketLauncher;
            break;
        default:
            return; // No audio mapping for this effect
    }
    
    if (fileNumber > 0) {
        playAudioFile(fileNumber);
        Serial.printf("Playing effect audio: %s -> file %03d\n", 
                      getEffectName(effect).c_str(), fileNumber);
    }
}

void playScenarioAudio(const String& scenario) {
    if (!isAudioReady()) return;
    
    uint8_t fileNumber = 0;
    
    if (scenario.equalsIgnoreCase("engine_idle")) {
        fileNumber = config.audioMap.engineIdle;
    } else if (scenario.equalsIgnoreCase("engine_rev")) {
        fileNumber = config.audioMap.engineRev;
    } else if (scenario.equalsIgnoreCase("machine_gun")) {
        fileNumber = config.audioMap.machineGun;
    } else if (scenario.equalsIgnoreCase("flamethrower")) {
        fileNumber = config.audioMap.flamethrower;
    } else if (scenario.equalsIgnoreCase("taking_hits")) {
        fileNumber = config.audioMap.takingHits;
    } else if (scenario.equalsIgnoreCase("explosion")) {
        fileNumber = config.audioMap.explosion;
    } else if (scenario.equalsIgnoreCase("rocket_launcher")) {
        fileNumber = config.audioMap.rocketLauncher;
    } else if (scenario.equalsIgnoreCase("kill_confirmed")) {
        fileNumber = config.audioMap.killConfirmed;
    }
    
    if (fileNumber > 0) {
        playAudioFile(fileNumber);
        Serial.printf("Playing scenario audio: %s -> file %03d\n", 
                      scenario.c_str(), fileNumber);
    }
}

String getEffectName(EffectType effect) {
    switch (effect) {
        case EffectType::EFFECT_NONE: return "None";
        case EffectType::EFFECT_CANDLE_FLICKER: return "Candle Flicker";
        case EffectType::EFFECT_FADE: return "Fade";
        case EffectType::EFFECT_PULSE: return "Pulse";
        case EffectType::EFFECT_STROBE: return "Strobe";
        case EffectType::EFFECT_ENGINE_IDLE: return "Engine Idle";
        case EffectType::EFFECT_ENGINE_REV: return "Engine Rev";
        case EffectType::EFFECT_MACHINE_GUN: return "Machine Gun";
        case EffectType::EFFECT_FLAMETHROWER: return "Flamethrower";
        case EffectType::EFFECT_TAKING_HITS: return "Taking Hits";
        case EffectType::EFFECT_EXPLOSION: return "Explosion";
        case EffectType::EFFECT_ROCKET_LAUNCHER: return "Rocket Launcher";
        case EffectType::EFFECT_CONSOLE_RGB: return "Console RGB";
        case EffectType::EFFECT_STATIC_ON: return "Static On";
        case EffectType::EFFECT_STATIC_OFF: return "Static Off";
        default: return "Unknown";
    }
}

uint16_t getEffectDuration(EffectType effect) {
    // First check if there's an audio file mapped for this effect
    uint8_t audioFile = 0;
    
    switch (effect) {
        case EffectType::EFFECT_ENGINE_IDLE:
            audioFile = config.audioMap.engineIdle;
            return (audioFile > 0) ? 0xFFFF : 0; // 0xFFFF = sync with audio, 0 = infinite/ambient
        case EffectType::EFFECT_ENGINE_REV:
            audioFile = config.audioMap.engineRev;
            return (audioFile > 0) ? 0xFFFF : DEFAULT_EFFECT_DURATION_LONG; // 0xFFFF = sync with audio
        case EffectType::EFFECT_MACHINE_GUN:
            audioFile = config.audioMap.machineGun;
            return (audioFile > 0) ? 0xFFFF : DEFAULT_EFFECT_DURATION_MEDIUM; // 0xFFFF = sync with audio
        case EffectType::EFFECT_FLAMETHROWER:
            audioFile = config.audioMap.flamethrower;
            return (audioFile > 0) ? 0xFFFF : DEFAULT_EFFECT_DURATION_LONG; // 0xFFFF = sync with audio
        case EffectType::EFFECT_TAKING_HITS:
            audioFile = config.audioMap.takingHits;
            return (audioFile > 0) ? 0xFFFF : DEFAULT_EFFECT_DURATION_SHORT; // 0xFFFF = sync with audio
        case EffectType::EFFECT_EXPLOSION:
            audioFile = config.audioMap.explosion;
            return (audioFile > 0) ? 0xFFFF : DEFAULT_EFFECT_DURATION_MEDIUM; // 0xFFFF = sync with audio
        case EffectType::EFFECT_ROCKET_LAUNCHER:
            audioFile = config.audioMap.rocketLauncher;
            return (audioFile > 0) ? 0xFFFF : DEFAULT_EFFECT_DURATION_MEDIUM; // 0xFFFF = sync with audio
        case EffectType::EFFECT_CANDLE_FLICKER:
            return 0; // Ambient effect - runs indefinitely
        case EffectType::EFFECT_PULSE:
            return DEFAULT_EFFECT_DURATION_MEDIUM;
        case EffectType::EFFECT_STROBE:
            return DEFAULT_EFFECT_DURATION_SHORT;
        case EffectType::EFFECT_FADE:
            return DEFAULT_EFFECT_DURATION_MEDIUM;
        case EffectType::EFFECT_CONSOLE_RGB:
            return 0; // Ambient effect - runs indefinitely
        default:
            return DEFAULT_EFFECT_DURATION_SHORT;
    }
}

bool isAudioStillPlaying() {
    if (!audioInitialized || !config.audioEnabled) {
        return false;
    }
    
    // Use DFPlayer readState() method to check playback status
    // Note: readState() can be unreliable, so we also check timeout
    int state = dfPlayer.readState();
    bool isPlaying = (state == 1 || state == 513); // 1 = playing, 513 = common playing state
    
    // Apply timeout fallback if audio has been "playing" too long
    unsigned long now = millis();
    if (audioCurrentlyPlaying && (now - audioStartTime) > MAX_AUDIO_TIMEOUT) {
        Serial.printf("⚠ Audio timeout reached after %lums, assuming playback finished\n", 
                     now - audioStartTime);
        audioCurrentlyPlaying = false;
        return false;
    }
    
    // If DFPlayer says not playing but we think it should be, double-check with small delay
    if (audioCurrentlyPlaying && !isPlaying) {
        // Give it one more chance in case of temporary UART communication issue
        delay(5);
        state = dfPlayer.readState();
        isPlaying = (state == 1 || state == 513);
        
        if (!isPlaying) {
            Serial.printf("🎵 Audio playback completed (state: %d, duration: %lums)\n", 
                         state, now - audioStartTime);
            audioCurrentlyPlaying = false;
        }
    }
    
    return audioCurrentlyPlaying && isPlaying;
}

void updateAudioBasedEffects() {
    if (!audioCurrentlyPlaying) {
        return; // No audio playing, nothing to check
    }
    
    unsigned long now = millis();
    
    // Only check audio status periodically to avoid UART spam
    if (now - lastAudioCheck < AUDIO_CHECK_INTERVAL) {
        return;
    }
    lastAudioCheck = now;
    
    // Check if audio has finished
    if (!isAudioStillPlaying()) {
        Serial.println("🎵 Audio finished, checking for audio-synced effects to restore");
        
        // Find any effects that were waiting for audio to complete
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (!config.pins[i].enabled || !config.pins[i].effectActive) continue;
            
            // If effect end time is set to our MAX timeout, it's likely an audio-synced effect
            if (effectStates[i].effectEndTime > 0 && 
                (effectStates[i].effectEndTime - now) > (MAX_AUDIO_TIMEOUT - 5000)) {
                
                Serial.printf("🎵 Restoring ambient effect on pin %d after audio completion\n", i);
                restoreAmbientEffect(i);
            }
        }
        
        audioCurrentlyPlaying = false;
    }
}
