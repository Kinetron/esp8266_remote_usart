#include <Arduino.h>
#include <string.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include "../SystemSettings/SystemSettings.h"

void createWebServer();
void runWebServer();
void handleClient();