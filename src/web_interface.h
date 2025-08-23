#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "config.h"
#include "DFRobotDFPlayerMini.h"

// Global web server instance
extern AsyncWebServer server;
extern DFRobotDFPlayerMini dfPlayer;

// Web interface functions
void initWebInterface();
void setupWebRoutes();

// API endpoint handlers
void handleGetConfig(AsyncWebServerRequest *request);
void handleSystemInfo(AsyncWebServerRequest *request);
void handleCheckUpdates(AsyncWebServerRequest *request);
void handlePerformUpdate(AsyncWebServerRequest *request);
void handleFactoryReset(AsyncWebServerRequest *request);

// Utility functions
String serializeConfig();

#endif