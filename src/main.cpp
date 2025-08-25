#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>

// Application constants
const char* VERSION = "0.1.1-dev";
const char* AP_SSID = "BattleAura";  
const char* AP_PASS = "battlesync";
const int AP_CHANNEL = 1;

// Global objects
WebServer server(80);

// Application state
bool systemReady = false;
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 10000; // 10 seconds

// Forward declarations
void setupWiFiAP();
void setupWebServer();
void setupOTA();
void handleRoot();
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
    
    // Initialize WiFi Access Point
    Serial.println("Setting up WiFi Access Point...");
    setupWiFiAP();
    
    // Initialize Web Server  
    Serial.println("Setting up Web Server...");
    setupWebServer();
    
    // Initialize OTA Updates
    Serial.println("Setting up OTA Updates...");
    setupOTA();
    
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
    
    // Handle OTA updates (non-blocking)
    ArduinoOTA.handle();
    
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
             "<h3>OTA Updates</h3>"
             "<p><strong>Hostname:</strong> BattleAura.local</p>"
             "<p><strong>Password:</strong> battlesync</p>"
             "<p>Use Arduino IDE: Tools → Port → Network Port</p>"
             "</div>"
             "<div class=\"status\">"
             "<h3>Quick Test</h3>"
             "<p>If you can see this page, the basic system is working!</p>"
             "</div>"
             "</body></html>");
    
    server.send(200, "text/html", html);
}

void setupOTA() {
    // OTA hostname and password
    ArduinoOTA.setHostname("BattleAura");
    ArduinoOTA.setPassword("battlesync");
    
    // OTA callbacks for status reporting
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }
        Serial.println("Start updating " + type);
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\nUpdate complete");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });
    
    ArduinoOTA.begin();
    Serial.println("✓ OTA updates enabled");
}

void printSystemInfo() {
    Serial.println();
    Serial.println("System Configuration:");
    Serial.printf("  WiFi AP: %s\n", AP_SSID);
    Serial.printf("  IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  Web Interface: http://%s/\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  OTA Hostname: BattleAura.local\n");
    Serial.printf("  OTA Password: battlesync\n");
    Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  Flash size: %d bytes\n", ESP.getFlashChipSize());
}