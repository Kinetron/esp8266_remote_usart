
#define DEFAULT_SYSTEM_PASSWORD "Yotek901" //Пароль для доступа к системным настройкам через телеграмм бот.

#define INIT_WORD "nofirst"
//If a value has been written in a cell, this value starts from this line.
#define CELL_WRITED_FLAG "wrt"
#define CELL_WRITED_FLAG_LEN 3

//EEPROM map.
#define EEPROM_INIT_WORD_LEN 7 //The value for determining the first run - len of string INIT_WORD "nofirst".
#define EEPROM_CLIENT_SSID_LEN 32 //SSID home wifi len.
#define EEPROM_CLIENT_PASSWORD_LEN 64 //Home wifi password.
#define EEPROM_SYSTEM_PASSWORD_LEN 32 + CELL_WRITED_FLAG_LEN
#define EEPROM_TELEGRAM_CLIENT_ID_LEN 15 + CELL_WRITED_FLAG_LEN

#include <Arduino.h>
#include <string.h>
#include <EEPROM.h>
#include <FastBot.h>

void writeStringEeprom(int beginPos, const String &data);
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

//Read Telegram client id from eeprom.
String readTelegramClientId();