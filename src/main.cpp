#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

// Application constants
const char* VERSION = "0.2.0-dev";
const char* AP_SSID = "BattleAura";  
const char* AP_PASS = "battlesync";
const int AP_CHANNEL = 1;

// GPIO configuration
const int TEST_LED_PIN = 2;  // GPIO 2 for testing
bool ledState = false;

// Global objects
WebServer server(80);

// Application state
bool systemReady = false;
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 10000; // 10 seconds

// Forward declarations
void setupWiFiAP();
void setupWebServer();
void setupGPIO();
void handleRoot();
void handleUpdate();
void handleUpdateUpload();
void handleLedControl();
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
        server.send(200, "text/plain", "OK");
    });
    
    // Favicon handler (prevents 404 errors)
    server.on("/favicon.ico", HTTP_GET, []() {
        server.send(204); // No Content
    });
    
    // OTA update page
    server.on("/update", HTTP_GET, handleUpdate);
    server.on("/update", HTTP_POST, handleUpdateUpload);
    
    // GPIO control endpoints
    server.on("/led/on", HTTP_GET, []() { handleLedControl(true); });
    server.on("/led/off", HTTP_GET, []() { handleLedControl(false); });
    server.on("/led/toggle", HTTP_GET, []() { handleLedControl(!ledState); });
    
    // 404 handler
    server.onNotFound([]() {
        server.send(404, "text/plain", "Not found");
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
    
    server.send(200, "text/html", html);
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
    
    server.send(200, "text/html", html);
}

void handleUpdateUpload() {
    HTTPUpload& upload = server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Starting firmware update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
            server.send(500, "text/plain", "Update failed to start");
            return;
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
            server.send(500, "text/plain", "Update write failed");
            return;
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("Update complete: %u bytes\n", upload.totalSize);
            server.send(200, "text/html", 
                "<html><body style='font-family:Arial; background:#1a1a2e; color:white; text-align:center; padding:50px;'>"
                "<h1>✅ Update Successful!</h1>"
                "<p>Device will restart in 3 seconds...</p>"
                "<script>setTimeout(function(){ window.location='/'; }, 5000);</script>"
                "</body></html>");
            delay(1000);
            ESP.restart();
        } else {
            Update.printError(Serial);
            server.send(500, "text/plain", "Update failed to complete");
        }
    }
}

void setupGPIO() {
    pinMode(TEST_LED_PIN, OUTPUT);
    digitalWrite(TEST_LED_PIN, LOW);
    ledState = false;
    Serial.printf("✓ GPIO %d configured as output\n", TEST_LED_PIN);
}

void handleLedControl(bool state) {
    ledState = state;
    digitalWrite(TEST_LED_PIN, ledState ? HIGH : LOW);
    Serial.printf("GPIO %d: %s\n", TEST_LED_PIN, ledState ? "ON" : "OFF");
    server.send(200, "text/plain", ledState ? "LED ON" : "LED OFF");
}

void printSystemInfo() {
    Serial.println();
    Serial.println("System Configuration:");
    Serial.printf("  WiFi AP: %s\n", AP_SSID);
    Serial.printf("  IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  Web Interface: http://%s/\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  Firmware Updates: http://%s/update\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  GPIO Control: GPIO %d (Test LED)\n", TEST_LED_PIN);
    Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  Flash size: %d bytes\n", ESP.getFlashChipSize());
}