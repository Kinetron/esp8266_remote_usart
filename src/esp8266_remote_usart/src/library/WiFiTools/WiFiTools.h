#include <Arduino.h>
#include <string.h>
#include <ESP8266WiFi.h>

#define WIFI_СONNECTION_ATTEMPTS 30 //Max attempts count connect to WiFi.
#define DEFAULT_SSID "esp8266_rt"

bool connectWiFi(const char *ssid, const char *password);
//Enable AP.
void initAp();
String scanNetworks();