#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "config.h"
#include "DFRobotDFPlayerMini.h"

// Global web server and WebSocket instances
extern AsyncWebServer server;
extern AsyncWebSocket ws;
extern DFRobotDFPlayerMini dfPlayer;

// Web interface functions
void initWebInterface();
void setupWebRoutes();
void setupWebSocket();

// API endpoint handlers
void handleGetConfig(AsyncWebServerRequest *request);
void handleSystemInfo(AsyncWebServerRequest *request);
void handleCheckUpdates(AsyncWebServerRequest *request);
void handleFactoryReset(AsyncWebServerRequest *request);

// WebSocket handlers
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void broadcastToWebSocket(const String& message);
void broadcastEffectStarted(const String& effectId);
void broadcastConfigUpdated();

// Utility functions
String serializeConfig();
bool parseJsonBody(AsyncWebServerRequest *request, JsonDocument& doc);

#endif