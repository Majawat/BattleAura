#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

const char* VERSION = "1.0.1-rebuild";
const char* AP_NAME = "BattleAura";
const char* AP_PASS = "battlesync";

WebServer server(80);
bool systemReady = false;

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>BattleAura Control</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<style>";
  html += "body { font-family: Arial; background: #1a1a2e; color: white; margin: 20px; }";
  html += ".header { text-align: center; margin-bottom: 30px; }";
  html += ".card { background: rgba(255,255,255,0.1); padding: 20px; margin: 10px 0; border-radius: 10px; }";
  html += ".btn { background: #ff6b35; color: white; border: none; padding: 10px 20px; border-radius: 5px; margin: 5px; cursor: pointer; }";
  html += ".btn:hover { background: #e55a2b; }";
  html += ".status { display: flex; justify-content: space-around; margin-bottom: 20px; }";
  html += ".status div { background: rgba(255,255,255,0.1); padding: 15px; border-radius: 8px; }";
  html += "</style></head><body>";
  html += "<div class=\"header\"><h1>BattleAura Control</h1>";
  html += "<p>Tactical Lighting & Audio System</p></div>";
  html += "<div class=\"status\">";
  html += "<div><strong>Version:</strong> " + String(VERSION) + "</div>";
  html += "<div><strong>Status:</strong> Ready</div>";
  html += "<div><strong>WiFi:</strong> AP Mode</div>";
  html += "</div>";
  html += "<div class=\"card\"><h3>Test Controls</h3>";
  html += "<button class=\"btn\" onclick=\"fetch('/test')\">Test LED</button>";
  html += "<button class=\"btn\" onclick=\"fetch('/audio')\">Test Audio</button>";
  html += "</div>";
  html += "<div class=\"card\"><h3>Pin 2 Control</h3>";
  html += "<button class=\"btn\" onclick=\"fetch('/pin2/on')\">Pin 2 ON</button>";
  html += "<button class=\"btn\" onclick=\"fetch('/pin2/off')\">Pin 2 OFF</button>";
  html += "</div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleStatus() {
  server.send(200, "text/plain", "OK");
}

void handleTest() {
  Serial.println("Test button pressed");
  server.send(200, "text/plain", "Test executed");
}

void handleAudio() {
  Serial.println("Audio test button pressed");
  server.send(200, "text/plain", "Audio test executed");
}

void handlePin2On() {
  digitalWrite(2, HIGH);
  Serial.println("Pin 2 ON");
  server.send(200, "text/plain", "Pin 2 ON");
}

void handlePin2Off() {
  digitalWrite(2, LOW);
  Serial.println("Pin 2 OFF");
  server.send(200, "text/plain", "Pin 2 OFF");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("");
  Serial.println("================================");
  Serial.println("BattleAura Starting...");
  Serial.print("Version: "); Serial.println(VERSION);
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("================================");
  
  // Setup test pin
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  Serial.println("Pin 2 configured as output");
  
  // Start WiFi AP
  Serial.println("Starting WiFi AP...");
  WiFi.mode(WIFI_AP);
  bool apResult = WiFi.softAP(AP_NAME, AP_PASS);
  
  if (apResult) {
    Serial.printf("WiFi AP started: %s\n", AP_NAME);
    Serial.printf("Password: %s\n", AP_PASS);
    Serial.printf("IP Address: %s\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("FAILED to start WiFi AP");
    return;
  }
  
  // Setup web server
  Serial.println("Starting web server...");
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/test", handleTest);
  server.on("/audio", handleAudio);
  server.on("/pin2/on", handlePin2On);
  server.on("/pin2/off", handlePin2Off);
  
  server.begin();
  Serial.println("Web server started on port 80");
  
  systemReady = true;
  Serial.println("");
  Serial.println("🎉 SYSTEM READY! 🎉");
  Serial.println("Connect to WiFi: " + String(AP_NAME));
  Serial.println("Password: " + String(AP_PASS));
  Serial.println("Open browser to: http://" + WiFi.softAPIP().toString());
  Serial.println("");
}

void loop() {
  if (!systemReady) {
    delay(1000);
    return;
  }
  
  server.handleClient();
  
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 30000) { // Every 30 seconds
    Serial.printf("Status: Running, Free heap: %d bytes\n", ESP.getFreeHeap());
    lastStatus = millis();
  }
  
  delay(10);
}