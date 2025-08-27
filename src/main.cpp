#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>
#include "webfiles.h"

// Application constants
const char* VERSION = "2.0.0-dev";
const char* AP_SSID = "BattleAura";  
const char* AP_PASS = "battlesync";
const int AP_CHANNEL = 1;

// Configuration constants
#define CONFIG_FILE "/config.json"

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
    
    EffectState() : lastUpdate(0), step(0), currentBrightness(0), direction(true) {}
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
String getEffectName(EffectType effect);
EffectType mapEffectNameToType(const String& effectName, PinMode mode);
void handleLedControl(bool state);
void initializeNeoPixel(uint8_t pinIndex);
void setNeoPixelColor(uint8_t pinIndex, uint32_t color, uint8_t brightness = 255);
void setNeoPixelState(uint8_t pinIndex, bool state);
void updateEffects();
void updateCandleFlicker(uint8_t pinIndex);
void startEffect(uint8_t pinIndex, EffectType effect);
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
void checkPinConflicts();
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
    String hostname = config.deviceName.length() > 0 ? config.deviceName : "battlearua";
    
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
    server.on("/styles.css", HTTP_GET, []() { handleEmbeddedFile("/styles.css"); });
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
    
    // Legacy status endpoint
    server.on("/status", HTTP_GET, []() {
        server.send(200, "text/plain", F("OK"));
    });
    
    // Favicon handler (prevents 404 errors)
    server.on("/favicon.ico", HTTP_GET, []() {
        server.send(204); // No Content
    });
    
    // Configuration page
    server.on("/config", HTTP_GET, []() { handleEmbeddedFile("/config.html"); });
    server.on("/config", HTTP_POST, handleConfigSave);
    
    // OTA update page
    server.on("/update", HTTP_GET, []() { handleEmbeddedFile("/update.html"); });
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
    
    server.on("/audio/status", HTTP_GET, []() {
        String status = "{";
        status += "\"ready\": " + String(isAudioReady() ? "true" : "false") + ",";
        status += "\"enabled\": " + String(config.audioEnabled ? "true" : "false") + ",";
        status += "\"volume\": " + String(config.volume);
        status += "}";
        server.send(200, "application/json", status);
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
    
    // RGB LED test endpoints
    server.on("/rgb/test", HTTP_GET, []() {
        // Test RGB LEDs with rainbow colors
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (config.pins[i].enabled && config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                // Cycle through rainbow colors
                uint32_t colors[] = {0xFF0000, 0xFF8000, 0xFFFF00, 0x00FF00, 0x0000FF, 0x8000FF, 0xFF00FF};
                for (int c = 0; c < 7; c++) {
                    setNeoPixelColor(i, colors[c], 128);
                    delay(300);
                }
                setNeoPixelColor(i, 0x000000, 0); // Turn off
                Serial.printf("RGB test completed on pin %d (GPIO %d)\n", i, config.pins[i].gpio);
                server.send(200, "text/plain", F("RGB Test Complete"));
                return;
            }
        }
        server.send(404, "text/plain", F("No RGB LEDs configured"));
    });
    
    server.on("/rgb/red", HTTP_GET, []() {
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (config.pins[i].enabled && config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                // Stop any active effects on this pin
                config.pins[i].effectActive = false;
                config.pins[i].color = 0xFF0000; // Save the color
                uint8_t brightness = calculateActualBrightness(i);
                setNeoPixelColor(i, 0xFF0000, brightness);
                Serial.printf("RGB Red set on pin %d (effects stopped, brightness: %d)\n", i, brightness);
                server.send(200, "text/plain", F("RGB Red"));
                return;
            }
        }
        server.send(404, "text/plain", F("No RGB LEDs configured"));
    });
    
    server.on("/rgb/green", HTTP_GET, []() {
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (config.pins[i].enabled && config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                // Stop any active effects on this pin
                config.pins[i].effectActive = false;
                config.pins[i].color = 0x00FF00; // Save the color
                uint8_t brightness = calculateActualBrightness(i);
                setNeoPixelColor(i, 0x00FF00, brightness);
                server.send(200, "text/plain", F("RGB Green"));
                return;
            }
        }
        server.send(404, "text/plain", F("No RGB LEDs configured"));
    });
    
    server.on("/rgb/blue", HTTP_GET, []() {
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (config.pins[i].enabled && config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                // Stop any active effects on this pin
                config.pins[i].effectActive = false;
                config.pins[i].color = 0x0000FF; // Save the color
                uint8_t brightness = calculateActualBrightness(i);
                setNeoPixelColor(i, 0x0000FF, brightness);
                server.send(200, "text/plain", F("RGB Blue"));
                return;
            }
        }
        server.send(404, "text/plain", F("No RGB LEDs configured"));
    });
    
    server.on("/rgb/white", HTTP_GET, []() {
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (config.pins[i].enabled && config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                // Stop any active effects on this pin
                config.pins[i].effectActive = false;
                config.pins[i].color = 0xFFFFFF; // Save the color
                uint8_t brightness = calculateActualBrightness(i);
                setNeoPixelColor(i, 0xFFFFFF, brightness);
                server.send(200, "text/plain", F("RGB White"));
                return;
            }
        }
        server.send(404, "text/plain", F("No RGB LEDs configured"));
    });
    
    server.on("/rgb/off", HTTP_GET, []() {
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (config.pins[i].enabled && config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                // Stop any active effects on this pin
                config.pins[i].effectActive = false;
                config.pins[i].color = 0x000000; // Save the color (off)
                setNeoPixelColor(i, 0x000000, 0); // Off
                server.send(200, "text/plain", F("RGB Off"));
                return;
            }
        }
        server.send(404, "text/plain", F("No RGB LEDs configured"));
    });
    
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
        
        // Calculate actual brightness using global max
        uint8_t actualBrightness = calculateActualBrightness(pin);
        
        if (config.pins[pin].mode == PinMode::OUTPUT_WS2812B) {
            // Stop effects and set brightness
            config.pins[pin].effectActive = false;
            setNeoPixelColor(pin, config.pins[pin].color, actualBrightness);
        } else if (config.pins[pin].mode == PinMode::OUTPUT_PWM) {
            analogWrite(config.pins[pin].gpio, actualBrightness);
        }
        
        // Save configuration
        saveConfiguration();
        
        server.send(200, "text/plain", "Pin " + String(pin) + " brightness: " + String(brightness) + " (actual: " + String(actualBrightness) + ")");
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
            config.pins[pinIndex].effectActive = false;
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
        config.pins[pinIndex].effectActive = false;
        
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
        uint16_t duration = server.hasArg("duration") ? server.arg("duration").toInt() : 0;
        
        // Find all pins matching this type
        bool foundPins = false;
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (!config.pins[i].enabled || !config.pins[i].type.equalsIgnoreCase(type)) continue;
            
            foundPins = true;
            EffectType effect = mapEffectNameToType(effectName, config.pins[i].mode);
            
            if (effect != EffectType::EFFECT_NONE) {
                startEffect(i, effect);
                if (duration > 0) {
                    // Set timeout to stop effect
                    // Note: In a full implementation, you'd want a timer system here
                }
                Serial.printf("Type effect: %s.%s applied to pin %d (GPIO %d)\n", 
                             type.c_str(), effectName.c_str(), i, config.pins[i].gpio);
            }
        }
        
        if (!foundPins) {
            server.send(404, "text/plain", "No enabled pins found for type: " + type);
        } else {
            server.send(200, "text/plain", "Effect " + effectName + " applied to type: " + type);
        }
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
            
        } else if (effectName == "off") {
            // Turn off all effects
            for (uint8_t i = 0; i < MAX_PINS; i++) {
                if (config.pins[i].enabled) {
                    stopEffect(i);
                    if (config.pins[i].mode == PinMode::OUTPUT_WS2812B) {
                        setNeoPixelColor(i, 0x000000, 0); // Off
                    } else if (config.pins[i].mode == PinMode::OUTPUT_PWM) {
                        analogWrite(config.pins[i].gpio, 0); // Off
                    }
                }
            }
            server.send(200, "text/plain", F("All effects stopped"));
            
        } else {
            server.send(400, "text/plain", "Unknown global effect: " + effectName);
        }
    });
    
    // 404 handler
    server.onNotFound([]() {
        server.send(404, "text/plain", F("Not found"));
    });
    
    server.begin();
    Serial.println("✓ Web server started on port 80");
}

// Helper function to map effect names to EffectType based on pin capabilities
EffectType mapEffectNameToType(const String& effectName, PinMode mode) {
    if (effectName == "off") return EffectType::EFFECT_NONE;
    if (effectName == "on") return EffectType::EFFECT_STATIC_ON;
    
    // Engine effects
    if (effectName == "idle") return EffectType::EFFECT_ENGINE_IDLE;
    if (effectName == "rev") return EffectType::EFFECT_ENGINE_REV;
    
    // Weapon effects  
    if (effectName == "fire1" || effectName == "fire") return EffectType::EFFECT_MACHINE_GUN;
    if (effectName == "fire2") return EffectType::EFFECT_FLAMETHROWER;
    
    // Candle effects
    if (effectName == "flicker") return EffectType::EFFECT_CANDLE_FLICKER;
    if (effectName == "bright") return EffectType::EFFECT_STATIC_ON;
    
    // Console effects (RGB only)
    if (mode == PinMode::OUTPUT_WS2812B) {
        if (effectName == "active") return EffectType::EFFECT_CONSOLE_RGB;
        if (effectName == "alert") return EffectType::EFFECT_PULSE;
    }
    
    // Generic effects
    if (effectName == "pulse") return EffectType::EFFECT_PULSE;
    if (effectName == "fade") return EffectType::EFFECT_FADE;
    
    return EffectType::EFFECT_NONE;
}

void handleRoot() {
    handleEmbeddedFile("/index.html");
}

void handleEmbeddedFile(const String& path) {
    // Look for the file in embedded files array
    for (size_t i = 0; i < embeddedFilesCount; i++) {
        if (String(embeddedFiles[i].path) == path) {
            server.send(200, embeddedFiles[i].contentType, embeddedFiles[i].data);
            return;
        }
    }
    
    // File not found
    Serial.printf("Embedded file not found: %s\n", path.c_str());
    server.send(404, "text/plain", "File not found");
}

// Legacy function for compatibility
void handleStaticFile(const String& path, const String& contentType) {
    handleEmbeddedFile(path);
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
    Serial.println("=== Starting Default Effects ===");
    
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        if (config.pins[i].enabled && config.pins[i].defaultEffect != EffectType::EFFECT_NONE) {
            Serial.printf("Starting default effect %d on pin %d (GPIO %d)\n", 
                         (int)config.pins[i].defaultEffect, i, config.pins[i].gpio);
            startEffect(i, config.pins[i].defaultEffect);
        }
    }
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
    
    // Set up one default pin for testing
    config.pins[0].gpio = 2;
    config.pins[0].mode = PinMode::OUTPUT_DIGITAL;
    config.pins[0].name = "Test LED";
    config.pins[0].enabled = true;
    config.pins[0].brightness = 255;
    config.pins[0].color = 0xFF6600;
    
    Serial.println("✓ Default configuration initialized");
}

void loadConfiguration() {
    initializeDefaults();  // Always start with defaults
    
    if (!SPIFFS.exists(CONFIG_FILE)) {
        Serial.println("⚠ Config file not found - using defaults");
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
        Serial.printf("✗ Failed to parse config: %s - using defaults\n", error.c_str());
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

void handleConfig() {
    String html = F("<!DOCTYPE html>"
                   "<html><head>"
                   "<meta charset=\"UTF-8\">"
                   "<title>BattleAura - Configuration</title>"
                   "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                   "<style>"
                   "body { font-family: Arial; margin: 20px; background: #1a1a2e; color: white; }"
                   ".header { text-align: center; margin-bottom: 20px; }"
                   ".section { background: #16213e; padding: 20px; border-radius: 8px; margin-bottom: 20px; }"
                   ".form-row { margin: 10px 0; display: flex; align-items: center; }"
                   ".form-row label { min-width: 140px; font-weight: bold; }"
                   "input, select { padding: 8px; margin-left: 10px; background: #0f172a; border: 1px solid #334155; color: white; border-radius: 4px; flex: 1; max-width: 200px; }"
                   "input[type=range] { max-width: 150px; }"
                   "input[type=checkbox] { width: auto; flex: none; }"
                   "input[type=submit] { background: #4CAF50; color: white; border: none; padding: 12px 30px; border-radius: 5px; cursor: pointer; font-size: 16px; margin-top: 20px; }"
                   "input[type=submit]:hover { background: #45a049; }"
                   ".pin-config { border: 1px solid #334155; padding: 15px; margin: 10px 0; border-radius: 8px; background: #0f1419; }"
                   ".pin-header { background: #1e293b; padding: 10px; border-radius: 4px; margin-bottom: 15px; }"
                   ".type-examples { font-size: 11px; color: #94a3b8; margin-left: 150px; }"
                   ".brightness-display { min-width: 40px; text-align: center; color: #94a3b8; }"
                   "</style>"
                   "</head><body>"
                   "<div class=\"header\">"
                   "<h1>⚙️ Modular Device Configuration</h1>"
                   "<p><a href=\"/\" style=\"color: #ff6b35;\">← Back to Control Panel</a></p>"
                   "<p style=\"font-size: 14px; color: #94a3b8;\">Configure pins by type and group for modular effect control</p>"
                   "</div>"
                   "<form method=\"POST\">"
                   "<div class=\"section\">"
                   "<h3>🔧 System Settings</h3>");
    
    html += F("<div class=\"form-row\">"
             "<label>Device Name:</label>"
             "<input type=\"text\" name=\"deviceName\" value=\"");
    html += config.deviceName;
    html += F("\" maxlength=\"32\" placeholder=\"Tank, Beast, Dreadnought...\">"
             "</div>"
             "<div class=\"form-row\">"
             "<label>Audio Volume:</label>"
             "<input type=\"range\" name=\"volume\" value=\"");
    html += config.volume;
    html += F("\" min=\"0\" max=\"30\" oninput=\"document.getElementById('volDisplay').innerText = this.value\">"
             "<span class=\"brightness-display\" id=\"volDisplay\">");
    html += config.volume;
    html += F("</span>"
             "</div>"
             "<div class=\"form-row\">"
             "<label>Audio Enabled:</label>"
             "<input type=\"checkbox\" name=\"audioEnabled\"");
    if (config.audioEnabled) html += F(" checked");
    html += F("></div></div>");
    
    html += F("<div class=\"section\">"
             "<h3>📡 WiFi Settings</h3>"
             "<div class=\"form-row\">"
             "<label>WiFi SSID:</label>"
             "<input type=\"text\" name=\"wifiSSID\" value=\"");
    html += config.wifiSSID;
    html += F("\" maxlength=\"32\">"
             "</div>"
             "<div class=\"form-row\">"
             "<label>WiFi Password:</label>"
             "<input type=\"password\" name=\"wifiPassword\" value=\"");
    html += config.wifiPassword;
    html += F("\" maxlength=\"64\">"
             "</div>"
             "<div class=\"form-row\">"
             "<label>WiFi Enabled:</label>"
             "<input type=\"checkbox\" name=\"wifiEnabled\"");
    if (config.wifiEnabled) html += F(" checked");
    html += F("></div></div>");
    
    // Pin configurations - completely redesigned for modular system
    html += F("<div class=\"section\">"
             "<h3>🎯 Modular Pin Configuration</h3>"
             "<p style=\"font-size: 13px; color: #94a3b8; margin-bottom: 20px;\">"
             "Configure each pin with type and group for automatic effect targeting. "
             "Same firmware works across different miniatures through configuration.</p>");
    
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        html += F("<div class=\"pin-config\">"
                 "<div class=\"pin-header\">"
                 "<h4>🔌 Pin Slot ");
        html += i + 1;
        
        // Show current assignment in header if configured
        if (config.pins[i].enabled && config.pins[i].type.length() > 0) {
            html += F(" - <span style=\"color: #ff6b35;\">");
            html += config.pins[i].type;
            if (config.pins[i].group.length() > 0) {
                html += F(" (");
                html += config.pins[i].group;
                html += F(")");
            }
            html += F("</span>");
        }
        
        html += F("</h4></div>");
        
        // Basic pin setup
        html += F("<div class=\"form-row\">"
                 "<label>GPIO Pin:</label>"
                 "<input type=\"number\" name=\"gpio");
        html += i;
        html += F("\" value=\"");
        html += config.pins[i].gpio;
        html += F("\" min=\"0\" max=\"21\">"
                 "</div>"
                 "<div class=\"form-row\">"
                 "<label>Pin Mode:</label>"
                 "<select name=\"mode");
        html += i;
        html += F("\">");
        
        // Pin mode options
        const char* modes[] = {"Disabled", "Digital Output", "PWM Output", "WS2812B RGB", "Digital Input", "Analog Input"};
        for (int m = 0; m < 6; m++) {
            html += F("<option value=\"");
            html += m;
            html += F("\"");
            if (static_cast<int>(config.pins[i].mode) == m) html += F(" selected");
            html += F(">");
            html += modes[m];
            html += F("</option>");
        }
        
        html += F("</select>"
                 "</div>"
                 "<div class=\"form-row\">"
                 "<label>Pin Name:</label>"
                 "<input type=\"text\" name=\"name");
        html += i;
        html += F("\" value=\"");
        html += config.pins[i].name;
        html += F("\" maxlength=\"16\" placeholder=\"Descriptive name...\">"
                 "</div>"
                 "<div class=\"form-row\">"
                 "<label>Enabled:</label>"
                 "<input type=\"checkbox\" name=\"enabled");
        html += i;
        html += F("\"");
        if (config.pins[i].enabled) html += F(" checked");
        html += F("></div>");
        
        // NEW: Type and Group fields for modular system
        html += F("<div class=\"form-row\">"
                 "<label>Pin Type:</label>"
                 "<input type=\"text\" name=\"type");
        html += i;
        html += F("\" value=\"");
        html += config.pins[i].type;
        html += F("\" maxlength=\"20\" placeholder=\"Engine, Weapon, Candle, Console...\">"
                 "</div>"
                 "<div class=\"type-examples\">Examples: Engine, Weapon, Candle, Console, Brazier, Spotlight</div>"
                 "<div class=\"form-row\">"
                 "<label>Pin Group:</label>"
                 "<input type=\"text\" name=\"group");
        html += i;
        html += F("\" value=\"");
        html += config.pins[i].group;
        html += F("\" maxlength=\"20\" placeholder=\"Engine1, Weapon1, Candles...\">"
                 "</div>"
                 "<div class=\"type-examples\">Examples: Engine1, Engine2, Weapon1, Weapon2, MainCandles, SpotLights</div>");
        
        // Per-pin brightness control
        html += F("<div class=\"form-row\">"
                 "<label>Brightness:</label>"
                 "<input type=\"range\" name=\"brightness");
        html += i;
        html += F("\" value=\"");
        html += config.pins[i].brightness;
        html += F("\" min=\"0\" max=\"255\" oninput=\"document.getElementById('brightness");
        html += i;
        html += F("').innerText = this.value\">"
                 "<span class=\"brightness-display\" id=\"brightness");
        html += i;
        html += F("\">");
        html += config.pins[i].brightness;
        html += F("</span>"
             "</div>");
        
        // LED count for WS2812B strips
        html += F("<div class=\"form-row\">"
                 "<label>LED Count:</label>"
                 "<input type=\"number\" name=\"ledCount");
        html += i;
        html += F("\" value=\"");
        html += config.pins[i].ledCount;
        html += F("\" min=\"1\" max=\"100\" placeholder=\"For WS2812B strips\">"
                 "</div>"
                 "<div class=\"type-examples\">Number of LEDs in strip (WS2812B only)</div>"
                 "</div>");
    }
    
    html += F("</div>"
             "<input type=\"submit\" value=\"💾 Save Modular Configuration\">"
             "</form>"
             "<div style=\"margin-top: 30px; padding: 15px; background: #0f1419; border-radius: 8px; font-size: 13px; color: #94a3b8;\">"
             "<h4>💡 Configuration Tips:</h4>"
             "<ul style=\"margin: 10px 0;\">"
             "<li><strong>Types</strong> define what the pin controls (Engine, Weapon, etc.)</li>"
             "<li><strong>Groups</strong> allow multiple pins of same type (Engine1, Engine2)</li>"
             "<li><strong>Effects</strong> will target types, not individual pins</li>"
             "<li><strong>Same firmware</strong> works for Tank, Beast, Dreadnought through config</li>"
             "</ul></div>"
             "</body></html>");
    
    server.send(200, F("text/html; charset=utf-8"), html);
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
            config.pins[i].gpio = constrain(server.arg(gpioArg).toInt(), 0, 21);
        }
        
        if (server.hasArg(modeArg)) {
            config.pins[i].mode = static_cast<PinMode>(server.arg(modeArg).toInt());
        }
        
        if (server.hasArg(nameArg)) {
            config.pins[i].name = server.arg(nameArg);
        }
        
        // NEW: Handle type and group fields
        if (server.hasArg(typeArg)) {
            config.pins[i].type = server.arg(typeArg);
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

void handleAudioMapping() {
    String html = F("<!DOCTYPE html>"
                   "<html><head>"
                   "<meta charset=\"UTF-8\">"
                   "<title>BattleAura - Audio Mapping</title>"
                   "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                   "<style>"
                   "body { font-family: Arial; margin: 20px; background: #1a1a2e; color: white; }"
                   ".header { text-align: center; margin-bottom: 20px; }"
                   ".section { background: #16213e; padding: 20px; border-radius: 8px; margin-bottom: 20px; }"
                   ".mapping-row { margin: 10px 0; display: flex; align-items: center; }"
                   ".mapping-row label { width: 200px; display: inline-block; }"
                   ".mapping-row input { flex: 1; padding: 5px; margin-left: 10px; background: #0f1626; color: white; border: 1px solid #555; border-radius: 3px; }"
                   ".btn { padding: 10px 20px; margin: 5px; color: white; text-decoration: none; border-radius: 5px; display: inline-block; }"
                   ".btn-primary { background: #4CAF50; }"
                   ".btn-secondary { background: #ff6b35; }"
                   "</style>"
                   "</head><body>");
    
    html += F("<div class=\"header\">"
             "<h1>🎵 Audio Mapping Configuration</h1>"
             "<p>Map audio files to effects and scenarios</p>"
             "</div>");
    
    html += F("<div class=\"section\">"
             "<h3>Current Audio Mapping</h3>");
    
    // Effect mappings
    html += F("<div class=\"mapping-row\">"
             "<label>Candle Flicker:</label>"
             "<span>File ");
    html += config.audioMap.candleFlicker;
    html += F("</span></div>");
    
    html += F("<div class=\"mapping-row\">"
             "<label>Fade:</label>"
             "<span>File ");
    html += config.audioMap.fade;
    html += F("</span></div>");
    
    html += F("<div class=\"mapping-row\">"
             "<label>Pulse:</label>"
             "<span>File ");
    html += config.audioMap.pulse;
    html += F("</span></div>");
    
    html += F("<div class=\"mapping-row\">"
             "<label>Strobe:</label>"
             "<span>File ");
    html += config.audioMap.strobe;
    html += F("</span></div>");
    
    // Scenario mappings
    html += F("<h4>Scenario Mappings:</h4>");
    
    html += F("<div class=\"mapping-row\">"
             "<label>Engine Idle:</label>"
             "<span>File ");
    html += config.audioMap.engineIdle;
    html += F("</span></div>");
    
    html += F("<div class=\"mapping-row\">"
             "<label>Engine Rev:</label>"
             "<span>File ");
    html += config.audioMap.engineRev;
    html += F("</span></div>");
    
    html += F("<div class=\"mapping-row\">"
             "<label>Machine Gun:</label>"
             "<span>File ");
    html += config.audioMap.machineGun;
    html += F("</span></div>");
    
    html += F("<div class=\"mapping-row\">"
             "<label>Flamethrower:</label>"
             "<span>File ");
    html += config.audioMap.flamethrower;
    html += F("</span></div>");
    
    html += F("<div class=\"mapping-row\">"
             "<label>Taking Hits:</label>"
             "<span>File ");
    html += config.audioMap.takingHits;
    html += F("</span></div>");
    
    html += F("<div class=\"mapping-row\">"
             "<label>Explosion:</label>"
             "<span>File ");
    html += config.audioMap.explosion;
    html += F("</span></div>");
    
    html += F("<div class=\"mapping-row\">"
             "<label>Rocket Launcher:</label>"
             "<span>File ");
    html += config.audioMap.rocketLauncher;
    html += F("</span></div>");
    
    html += F("<div class=\"mapping-row\">"
             "<label>Kill Confirmed:</label>"
             "<span>File ");
    html += config.audioMap.killConfirmed;
    html += F("</span></div>");
    
    html += F("</div>");
    
    // Test controls
    html += F("<div class=\"section\">"
             "<h3>Test Audio</h3>"
             "<p>Play scenarios to test your mapping:</p>");
    
    html += F("<a href=\"/audio/scenario?name=engine_idle\" class=\"btn btn-primary\">🏎️ Engine Idle</a>");
    html += F("<a href=\"/audio/scenario?name=engine_rev\" class=\"btn btn-primary\">🚗 Engine Rev</a>");
    html += F("<a href=\"/audio/scenario?name=machine_gun\" class=\"btn btn-primary\">🔫 Machine Gun</a>");
    html += F("<a href=\"/audio/scenario?name=flamethrower\" class=\"btn btn-primary\">🔥 Flamethrower</a>");
    html += F("<a href=\"/audio/scenario?name=explosion\" class=\"btn btn-primary\">💥 Explosion</a>");
    html += F("<a href=\"/audio/stop\" class=\"btn btn-secondary\">⏹️ Stop Audio</a>");
    
    html += F("</div>");
    
    // Navigation
    html += F("<div class=\"section\">"
             "<a href=\"/config\" class=\"btn btn-secondary\">⚙️ Back to Configuration</a>"
             "<a href=\"/\" class=\"btn btn-primary\">🏠 Control Panel</a>"
             "</div>");
    
    html += F("</body></html>");
    
    server.send(200, F("text/html; charset=utf-8"), html);
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
        String hostname = config.deviceName.length() > 0 ? config.deviceName : "battlearua";
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

// Effects System Functions

void updateEffects() {
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        if (config.pins[i].enabled && config.pins[i].effectActive) {
            switch (config.pins[i].effect) {
                case EffectType::EFFECT_CANDLE_FLICKER:
                    updateCandleFlicker(i);
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
        
        // Create realistic candle flicker pattern
        uint8_t newBrightness;
        if (random(100) < 85) {
            // 85% chance of subtle variation
            newBrightness = random(baseBrightness - flickerRange/2, baseBrightness + 1);
        } else {
            // 15% chance of dramatic flicker
            newBrightness = random(minBrightness, maxBrightness + 1);
        }
        
        // Ensure brightness stays within bounds
        newBrightness = constrain(newBrightness, minBrightness, maxBrightness);
        
        setPinBrightness(pinIndex, newBrightness);
        effectStates[pinIndex].currentBrightness = newBrightness;
    }
}

void startEffect(uint8_t pinIndex, EffectType effect) {
    if (pinIndex >= MAX_PINS) return;
    
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

void stopEffect(uint8_t pinIndex) {
    if (pinIndex >= MAX_PINS) return;
    
    config.pins[pinIndex].effectActive = false;
    
    // Reset to original brightness
    setPinBrightness(pinIndex, config.pins[pinIndex].brightness);
    
    Serial.printf("Stopped effect on pin %d\n", pinIndex);
}

void setPinBrightness(uint8_t pinIndex, uint8_t brightness) {
    if (pinIndex >= MAX_PINS || !config.pins[pinIndex].enabled) return;
    
    PinMode mode = config.pins[pinIndex].mode;
    uint8_t gpio = config.pins[pinIndex].gpio;
    
    switch (mode) {
        case PinMode::OUTPUT_PWM:
            analogWrite(gpio, brightness);
            break;
        case PinMode::OUTPUT_DIGITAL:
            digitalWrite(gpio, brightness > 128 ? HIGH : LOW);
            break;
        case PinMode::OUTPUT_WS2812B:
            if (neoPixels[pinIndex] != nullptr) {
                setNeoPixelColor(pinIndex, config.pins[pinIndex].color, brightness);
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
        // TODO: Implement duration timer
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
    
    // Initialize UART for DFPlayer
    dfPlayerSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
    delay(1000); // Wait for DFPlayer to initialize
    
    // Initialize DFPlayer Mini
    if (dfPlayer.begin(dfPlayerSerial)) {
        Serial.println("✓ DFPlayer Mini initialized successfully");
        
        // Set initial volume
        dfPlayer.volume(config.volume);
        Serial.printf("✓ Audio volume set to %d/30\n", config.volume);
        
        // Query available files
        int fileCount = dfPlayer.readFileCounts();
        Serial.printf("✓ Found %d audio files on SD card\n", fileCount);
        
        audioInitialized = true;
    } else {
        Serial.println("✗ Failed to initialize DFPlayer Mini");
        Serial.println("  Check connections and SD card");
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
    Serial.println("🔇 Audio stopped");
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