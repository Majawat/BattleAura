#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// Application constants
const char* VERSION = "0.3.0-dev";
const char* AP_SSID = "BattleAura";  
const char* AP_PASS = "battlesync";
const int AP_CHANNEL = 1;

// Configuration constants
#define CONFIG_FILE "/config.json"
#define MAX_PINS 8

// Pin mode types
enum class PinMode {
    PIN_DISABLED = 0,
    OUTPUT_DIGITAL = 1,
    OUTPUT_PWM = 2,
    OUTPUT_WS2812B = 3,
    INPUT_DIGITAL = 4,
    INPUT_ANALOG = 5
};

// Pin configuration structure
struct PinConfig {
    uint8_t gpio;
    PinMode mode;
    String name;
    uint8_t audioFile;
    bool enabled;
    uint8_t brightness;
    uint32_t color;
    
    PinConfig() : gpio(0), mode(PinMode::PIN_DISABLED), name("Unused"), 
                  audioFile(0), enabled(false), brightness(255), color(0xFFFFFF) {}
};

// System configuration
struct SystemConfig {
    String deviceName;
    String wifiSSID;
    String wifiPassword;
    bool wifiEnabled;
    uint8_t volume;
    bool audioEnabled;
    PinConfig pins[MAX_PINS];
    
    SystemConfig() : deviceName("BattleAura"), wifiSSID(""), wifiPassword(""),
                     wifiEnabled(false), volume(15), audioEnabled(true) {}
};

// Global configuration
SystemConfig config;
bool configLoaded = false;

// GPIO state tracking
bool pinStates[MAX_PINS] = {false};

// Global objects
WebServer server(80);

// Application state
bool systemReady = false;
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 10000; // 10 seconds

// Forward declarations
void setupSPIFFS();
void loadConfiguration();
void saveConfiguration();
void initializeDefaults();
void setupWiFiAP();
void setupWebServer();
void setupGPIO();
void handleRoot();
void handleUpdate();
void handleUpdateUpload();
void handleLedControl(bool state);
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
    
    // Initialize WiFi Access Point
    Serial.println("Setting up WiFi Access Point...");
    setupWiFiAP();
    
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

void setupWiFiAP() {
    WiFi.mode(WIFI_AP);
    bool result = WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL);
    
    if (result) {
        Serial.printf("✓ WiFi AP created: %s\n", AP_SSID);
        Serial.printf("✓ Password: %s\n", AP_PASS); 
        Serial.printf("✓ IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("✗ Failed to create WiFi AP");
        Serial.println("  Continuing with limited functionality...");
    }
}

void setupWebServer() {
    // Root handler
    server.on("/", HTTP_GET, handleRoot);
    
    // Status endpoint
    server.on("/status", HTTP_GET, []() {
        server.send(200, "text/plain", F("OK"));
    });
    
    // Favicon handler (prevents 404 errors)
    server.on("/favicon.ico", HTTP_GET, []() {
        server.send(204); // No Content
    });
    
    // OTA update page
    server.on("/update", HTTP_GET, handleUpdate);
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
    server.on("/led/toggle", HTTP_GET, []() { handleLedControl(!ledState); });
    
    // 404 handler
    server.onNotFound([]() {
        server.send(404, "text/plain", F("Not found"));
    });
    
    server.begin();
    Serial.println("✓ Web server started on port 80");
}

void handleRoot() {
    String html = F("<!DOCTYPE html>"
                   "<html><head>"
                   "<meta charset=\"UTF-8\">"
                   "<title>BattleAura Control</title>"
                   "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                   "<style>"
                   "body { font-family: Arial; margin: 20px; background: #1a1a2e; color: white; }"
                   ".header { text-align: center; margin-bottom: 20px; }"
                   ".status { background: #16213e; padding: 15px; border-radius: 8px; margin-bottom: 20px; }"
                   ".ready { color: #4CAF50; font-weight: bold; }"
                   "</style>"
                   "</head><body>"
                   "<div class=\"header\">"
                   "<h1>🎯 BattleAura Control</h1>"
                   "<p>Tactical Lighting & Audio System</p>"
                   "</div>"
                   "<div class=\"status\">"
                   "<h3>System Status</h3>");
    
    html += "<p><strong>Version:</strong> ";
    html += VERSION;
    html += "</p>";
    
    html += "<p><strong>Status:</strong> <span class=\"ready\">READY</span></p>";
    
    html += "<p><strong>Free Memory:</strong> ";
    html += ESP.getFreeHeap();
    html += " bytes</p>";
    
    html += "<p><strong>Uptime:</strong> ";
    html += millis() / 1000;
    html += " seconds</p>";
    
    html += F("</div>"
             "<div class=\"status\">"
             "<h3>GPIO Control</h3>"
             "<p><strong>Test LED (GPIO 2):</strong> ");
    html += ledState ? "ON" : "OFF";
    html += F("</p>"
             "<button onclick=\"fetch('/led/on')\" style=\"background:#4CAF50; color:white; border:none; padding:8px 16px; margin:4px; border-radius:4px; cursor:pointer;\">LED ON</button>"
             "<button onclick=\"fetch('/led/off')\" style=\"background:#f44336; color:white; border:none; padding:8px 16px; margin:4px; border-radius:4px; cursor:pointer;\">LED OFF</button>"
             "<button onclick=\"fetch('/led/toggle'); setTimeout(function(){location.reload();}, 100);\" style=\"background:#ff6b35; color:white; border:none; padding:8px 16px; margin:4px; border-radius:4px; cursor:pointer;\">TOGGLE</button>"
             "</div>"
             "<div class=\"status\">"
             "<h3>Firmware Update</h3>"
             "<p><a href=\"/update\" style=\"color: #ff6b35; text-decoration: none;\">Upload New Firmware (.bin file)</a></p>"
             "</div>"
             "</body></html>");
    
    server.send(200, F("text/html; charset=utf-8"), html);
}

void handleUpdate() {
    String html = F("<!DOCTYPE html>"
                   "<html><head>"
                   "<meta charset=\"UTF-8\">"
                   "<title>BattleAura - Firmware Update</title>"
                   "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                   "<style>"
                   "body { font-family: Arial; margin: 20px; background: #1a1a2e; color: white; }"
                   ".header { text-align: center; margin-bottom: 20px; }"
                   ".form { background: #16213e; padding: 20px; border-radius: 8px; max-width: 500px; margin: 0 auto; }"
                   "input[type=file] { width: 100%; padding: 10px; margin: 10px 0; background: #0f172a; border: 1px solid #334155; color: white; border-radius: 4px; }"
                   "input[type=submit] { background: #ff6b35; color: white; border: none; padding: 12px 30px; border-radius: 5px; cursor: pointer; font-size: 16px; }"
                   "input[type=submit]:hover { background: #e55a2b; }"
                   ".warning { background: #7c2d12; padding: 10px; border-radius: 4px; margin: 10px 0; }"
                   "</style>"
                   "</head><body>"
                   "<div class=\"header\">"
                   "<h1>🎯 Firmware Update</h1>"
                   "<p><a href=\"/\" style=\"color: #ff6b35;\">← Back to Control Panel</a></p>"
                   "</div>"
                   "<div class=\"form\">"
                   "<div class=\"warning\">"
                   "<strong>⚠️ Warning:</strong> Only upload firmware.bin files built for ESP32-C3!"
                   "</div>"
                   "<h3>Upload New Firmware</h3>"
                   "<form method='POST' enctype='multipart/form-data'>"
                   "<p><strong>1.</strong> Build firmware: <code>platformio run</code></p>"
                   "<p><strong>2.</strong> Locate file: <code>.pio/build/seeed_xiao_esp32c3/firmware.bin</code></p>"
                   "<p><strong>3.</strong> Select and upload:</p>"
                   "<input type='file' name='update' accept='.bin' required>"
                   "<br><br>"
                   "<input type='submit' value='Upload Firmware'>"
                   "</form>"
                   "</div>"
                   "</body></html>");
    
    server.send(200, F("text/html; charset=utf-8"), html);
}

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
    for (uint8_t i = 0; i < MAX_PINS; i++) {
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
                    Serial.printf("✓ GPIO %d: %s (WS2812B - not implemented yet)\n", gpio, config.pins[i].name.c_str());
                    break;
                default:
                    break;
            }
        }
    }
}

void handleLedControl(bool state) {
    // Find first enabled output pin for compatibility
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        if (config.pins[i].enabled && 
            (config.pins[i].mode == PinMode::OUTPUT_DIGITAL || config.pins[i].mode == PinMode::OUTPUT_PWM)) {
            pinStates[i] = state;
            digitalWrite(config.pins[i].gpio, state ? HIGH : LOW);
            Serial.printf("GPIO %d (%s): %s\n", config.pins[i].gpio, config.pins[i].name.c_str(), state ? "ON" : "OFF");
            server.send(200, "text/plain", state ? F("LED ON") : F("LED OFF"));
            return;
        }
    }
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
    
    // Load pin configurations
    if (doc["pins"].is<JsonArray>()) {
        for (uint8_t i = 0; i < MAX_PINS && i < doc["pins"].size(); i++) {
            config.pins[i].gpio = doc["pins"][i]["gpio"] | 0;
            config.pins[i].mode = static_cast<PinMode>(doc["pins"][i]["mode"] | 0);
            config.pins[i].name = doc["pins"][i]["name"] | "Unused";
            config.pins[i].audioFile = doc["pins"][i]["audioFile"] | 0;
            config.pins[i].enabled = doc["pins"][i]["enabled"] | false;
            config.pins[i].brightness = doc["pins"][i]["brightness"] | 255;
            config.pins[i].color = doc["pins"][i]["color"] | 0xFFFFFF;
        }
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
    
    JsonArray pins = doc["pins"].to<JsonArray>();
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        JsonObject pin = pins.add<JsonObject>();
        pin["gpio"] = config.pins[i].gpio;
        pin["mode"] = static_cast<uint8_t>(config.pins[i].mode);
        pin["name"] = config.pins[i].name;
        pin["audioFile"] = config.pins[i].audioFile;
        pin["enabled"] = config.pins[i].enabled;
        pin["brightness"] = config.pins[i].brightness;
        pin["color"] = config.pins[i].color;
    }
    
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("✗ Failed to save config file");
        return;
    }
    
    size_t bytesWritten = serializeJson(doc, file);
    file.close();
    
    Serial.printf("✓ Configuration saved (%d bytes)\n", bytesWritten);
}

void printSystemInfo() {
    Serial.println();
    Serial.println("System Configuration:");
    Serial.printf("  Device: %s\n", config.deviceName.c_str());
    Serial.printf("  WiFi AP: %s\n", AP_SSID);
    Serial.printf("  IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  Web Interface: http://%s/\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  Firmware Updates: http://%s/update\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  Config File: %s\n", configLoaded ? "Loaded" : "Using defaults");
    Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  Flash size: %d bytes\n", ESP.getFlashChipSize());
}