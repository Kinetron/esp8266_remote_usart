#include <Arduino.h>
#include <string.h>
#include <Pinger.h>

#define PING_IP "8.8.8.8"

void initPingWatchdog();
void pingHost();