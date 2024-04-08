#include <Arduino.h>
#include <string.h>
#include <FastBot.h>
#include "../UCS2Converter/UCS2Converter.h"
#include "../Modem/Modem.h"
#include "../SystemSettings/SystemSettings.h"
#include "../WiFiTools/WiFiTools.h"

#define GPIO2 2
#define GPIO0 0

void executeCommand(String msg, String chatID);