
#define DEFAULT_SYSTEM_PASSWORD "Yotek901" //Пароль для доступа к системным настройкам через телеграмм бот.

#include <Arduino.h>
#include <string.h>
#include <EEPROM.h>
#include <FastBot.h>
#include "../eeprom_map.h"

void writeStringEeprom(int beginPos, const String &data);
void writeCharEeprom(int beginPos, char *data, int len);
void readCharEeprom(int beginPos, int len, char *array);
void eepromClear(int beginPos, int endPos);
String readStringEeprom(int beginPos, int len);

//Save wifi settings to eeprom. 
bool saveWifiSettings(const String &ssid, const String &password);

//Execute commands to configure the system.
void executeSystemCommands(String msg, String chatID);

//EEPROM adress for setting.
int getSystemPasswordAddress();

//EEPROM adress for setting.
int getTelegramClientIdAddress();

//EEPROM adress for setting.
int getBotTokenAddress();

//Сomplements the string with zeros to the required length
char* addZeroToString(const int len, String str);

String readTelegramClientId();
String prepareSystemValue(String value);

//Save the ID of the client to whom the messages will be sent.
bool saveTelegramClientId(String clientId);
bool saveBotToken(String token);
//Return SSID and BSSID and RSSI: 00:1A:70:DE:C1:68 RSSI: -68 dBm
String getWiFiStatus();
String readBotToken();