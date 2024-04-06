#include "SystemSettings.h"
extern FastBot bot;

bool enableChangeSystemSettings = false; //Allows to change system settings using the commands of the telegram bot.

void writeStringEeprom(int beginPos, const String &data)
{
    int pos = 0;
    for (int i = beginPos; i < beginPos + data.length(); ++i)
    {
      EEPROM.write(i, data[pos]);
      pos ++;
    }
}

void eepromClear(int beginPos, int endPos)
{
  for (int i = beginPos; i < endPos; ++i)
  {
    EEPROM.write(i, 0);
  }
}

String readStringEeprom(int beginPos, int len)
{
  String str;
  for (int i = beginPos; i < len; ++i)
  {
    str += char(EEPROM.read(i));
  }
  return str;
}

//Save wifi settings to eeprom. 
bool saveWifiSettings(const String &ssid, const String &password)
{
    //Clearing eeprom
    int endDataPos = EEPROM_INIT_WORD_LEN + EEPROM_CLIENT_SSID_LEN + EEPROM_CLIENT_PASSWORD_LEN;
    eepromClear(0, endDataPos);

    writeStringEeprom(0, INIT_WORD); //Set flag - device no first run.
    writeStringEeprom(EEPROM_INIT_WORD_LEN, ssid); //Writing wifi ssid.
    writeStringEeprom(EEPROM_INIT_WORD_LEN + EEPROM_CLIENT_SSID_LEN, password); //Writing wifi pass.
    
    if(!EEPROM.commit()){
        Serial.println("Error write to flash.");
        return false;
    }; 

    return true;
}

//Execute commands to configure the system.
void executeSystemCommands(String msg, String chatID)
{
  if (msg.substring(0, 6) == "/login")
  {
	String userPassword = msg.substring(7);
    String eepromPassword = readStringEeprom(getSystemPasswordAddress(), EEPROM_SYSTEM_PASSWORD_LEN);
		
	//The default password has been changed?
	if(eepromPassword.substring(0, CELL_WRITED_FLAG_LEN) != CELL_WRITED_FLAG) //No change;
	{
	  eepromPassword = DEFAULT_SYSTEM_PASSWORD; 	
	}
	
	if(eepromPassword == userPassword)
	{
	  enableChangeSystemSettings = true; //Allow to change the settings.
	  bot.sendMessage("OK", chatID);
	}
  }
  else if(enableChangeSystemSettings && msg.substring(0, 9) == "/set_ssid")
  {
	String ssid = msg.substring(10);
	eepromClear(EEPROM_INIT_WORD_LEN, EEPROM_CLIENT_SSID_LEN);
	writeStringEeprom(EEPROM_INIT_WORD_LEN, ssid); //Writing wifi ssid.
	 if(!EEPROM.commit()){
        bot.sendMessage("ERROR", chatID);
        return;
    };
	
	bot.sendMessage("OK", chatID);
  }
  else if(enableChangeSystemSettings && msg.substring(0, 18) == "/set_wifi_password")
  {
	String password = msg.substring(19);
	eepromClear(EEPROM_INIT_WORD_LEN + EEPROM_CLIENT_SSID_LEN, EEPROM_CLIENT_PASSWORD_LEN);
	writeStringEeprom(EEPROM_INIT_WORD_LEN + EEPROM_CLIENT_SSID_LEN, password); //Writing wifi pass
	 if(!EEPROM.commit()){
        bot.sendMessage("ERROR", chatID);
        return;
    };
	
	bot.sendMessage("OK", chatID);
  }
  else if(msg.substring(0, 18) == "/get_wifi_settings")
  {
	String ssid = readStringEeprom(EEPROM_INIT_WORD_LEN, EEPROM_CLIENT_SSID_LEN); 
    String password = readStringEeprom(EEPROM_INIT_WORD_LEN + EEPROM_CLIENT_SSID_LEN, EEPROM_CLIENT_SSID_LEN + EEPROM_CLIENT_PASSWORD_LEN);
	bot.sendMessage("SSID:" + ssid +";PSWD:" + password, chatID);
  }
  else if(enableChangeSystemSettings && msg.substring(0, 23) == "/set_telegram_client_id")
  {
	String clientId = msg.substring(24);
	eepromClear(getTelegramClientIdAddress(), EEPROM_TELEGRAM_CLIENT_ID_LEN);
	if(!EEPROM.commit()){
        bot.sendMessage("ERROR", chatID);
        return;
    };
	String str = CELL_WRITED_FLAG + clientId;
	writeStringEeprom(getTelegramClientIdAddress(), str);
	if(!EEPROM.commit()){
        bot.sendMessage("ERROR", chatID);
        return;
    };
	
	bot.sendMessage("OK" + clientId, chatID);
  }
  else if(enableChangeSystemSettings && msg.substring(0, 23) == "/get_telegram_client_id")
  {
	bot.sendMessage(readTelegramClientId(), chatID);
  }
}

//EEPROM adress for setting.
int getSystemPasswordAddress()
{
  return EEPROM_INIT_WORD_LEN +
         EEPROM_CLIENT_SSID_LEN +
		 EEPROM_CLIENT_PASSWORD_LEN;
}

//EEPROM adress for setting.
int getTelegramClientIdAddress()
{
  return EEPROM_INIT_WORD_LEN +
         EEPROM_CLIENT_SSID_LEN +
		 EEPROM_CLIENT_PASSWORD_LEN +
		 EEPROM_SYSTEM_PASSWORD_LEN;	
}

//Read Telegram client id from eeprom.
String readTelegramClientId()
{
  String id = readStringEeprom(getTelegramClientIdAddress(), EEPROM_TELEGRAM_CLIENT_ID_LEN);
  return "X"+id+"#";
  if(id.substring(0, CELL_WRITED_FLAG_LEN) != CELL_WRITED_FLAG) return "";
  return id.substring(CELL_WRITED_FLAG_LEN);    
}