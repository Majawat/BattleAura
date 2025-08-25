#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// Application constants
const char* VERSION = "1.2.0-minimal";
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
             "<h3>Quick Test</h3>"
             "<p>If you can see this page, the basic system is working!</p>"
             "</div>"
             "</body></html>");
    
    server.send(200, "text/html", html);
}

void printSystemInfo() {
    Serial.println();
    Serial.println("System Configuration:");
    Serial.printf("  WiFi AP: %s\n", AP_SSID);
    Serial.printf("  IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  Web Interface: http://%s/\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  Flash size: %d bytes\n", ESP.getFlashChipSize());
}