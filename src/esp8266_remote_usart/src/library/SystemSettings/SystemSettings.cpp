#include "SystemSettings.h"
extern FastBot bot;

bool enableChangeSystemSettings = false; //Allows to change system settings using the commands of the telegram bot.

void writeStringEeprom(int beginPos, const String &data)
{
    int pos = 0;
    for (int i = beginPos; i < beginPos + data.length(); ++i)
    {
      EEPROM.write(i, data[pos]);
	  yield();
      pos ++;
    }
}

void writeCharEeprom(int beginPos, char *data, int len)
{
    int pos = 0;
    for (int i = beginPos; i < beginPos + len; ++i)
    {
      EEPROM.write(i, data[pos]);
	  yield();
      pos ++;
    }
}

void eepromClear(int beginPos, int endPos)
{
  for (int i = beginPos; i < endPos; ++i)
  {
    EEPROM.write(i, 0);
	yield();
  }
}

String readStringEeprom(int beginPos, int len)
{
  String str;
  for (int i = beginPos; i < len; ++i)
  {
    str += char(EEPROM.read(i));
	yield();
  }
  return str;
}

void readCharEeprom(int beginPos, int len, char *array)
{
  int pos = 0;
  for (int i = beginPos; i < len; ++i)
  {
    array[pos]= char(EEPROM.read(i));
	pos++;
	yield();
  }
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
  else if(enableChangeSystemSettings && msg.substring(0, 23) == "/set_telegram_client_id")
  {
	String clientId = msg.substring(24);
    if(saveTelegramClientId(clientId))
	{
	  bot.sendMessage("OK", chatID);		
	}
	else
	{
	  bot.sendMessage("ERROR", chatID);
	}		
  }  
  else if(enableChangeSystemSettings && msg.substring(0, 23) == "/get_telegram_client_id")
  {
    bot.sendMessage(readTelegramClientId(), chatID); 
  }
  else if(enableChangeSystemSettings && msg.substring(0, 14) == "/set_bot_token")
  {
	String token = msg.substring(15);
    if(saveBotToken(token))
	{
	  bot.sendMessage("OK", chatID);		
	}
	else
	{
	  bot.sendMessage("ERROR", chatID);
	}		
  }
  else if(enableChangeSystemSettings && msg.substring(0, 14) == "/get_bot_token")
  {
	bot.sendMessage(readBotToken(), chatID);		
  }
}

//EEPROM adress for setting.
int getSystemPasswordAddress()
{
  return EEPROM_INIT_WORD_LEN +
         EEPROM_CLIENT_SSID_LEN +
		 EEPROM_CLIENT_PASSWORD_LEN +
		 EEPROM_TELEGRAM_CLIENT_ID_LEN;
}

//EEPROM adress for setting.
int getTelegramClientIdAddress()
{
  return EEPROM_INIT_WORD_LEN +
         EEPROM_CLIENT_SSID_LEN +
		 EEPROM_CLIENT_PASSWORD_LEN;
}

//EEPROM adress for setting.
int getBotTokenAddress()
{
  return EEPROM_INIT_WORD_LEN +
         EEPROM_CLIENT_SSID_LEN +
		 EEPROM_CLIENT_PASSWORD_LEN+
		 EEPROM_TELEGRAM_CLIENT_ID_LEN;
}

String prepareSystemValue(String value)
{
  if(value.substring(0, CELL_WRITED_FLAG_LEN) != CELL_WRITED_FLAG) return "";
  return value.substring(CELL_WRITED_FLAG_LEN); 
}

String readTelegramClientId()
{
  char data[EEPROM_TELEGRAM_CLIENT_ID_LEN]; 	
  EEPROM.get(getTelegramClientIdAddress(), data); 	
	 		
  return prepareSystemValue(data);
}

//Сomplements the string with zeros to the required length
char* addZeroToString(int len, String str)
{
  char *data = (char *) malloc (len);

  int pos = 0;
  for(int i = 0; i < EEPROM_TELEGRAM_CLIENT_ID_LEN; ++i)
  {
	if(pos < str.length())
	{
	  data[i] = str[pos];
      pos ++;		  
	}
	else
	{
	  data[i] = 0;	
	}
  }
  
  return data;  
}

//Save the ID of the client to whom the messages will be sent.
bool saveTelegramClientId(String clientId)
{
  char* data = addZeroToString(EEPROM_TELEGRAM_CLIENT_ID_LEN, CELL_WRITED_FLAG + clientId);
  writeCharEeprom(getTelegramClientIdAddress(), data, EEPROM_TELEGRAM_CLIENT_ID_LEN);
	
  if(!EEPROM.commit()){
	  free(data);
      return false;
  };
    
  free(data);
  return true;
}

//Return SSID and BSSID and RSSI:  Abc 00:1A:70:DE:C1:68 RSSI: -68 dBm
String getWiFiStatus()
{
   return WiFi.SSID() + " " + WiFi.BSSIDstr() + " " + WiFi.RSSI();
}

bool saveBotToken(String token)
{
  char* data = addZeroToString(EEPROM_TELEGRAM_BOT_TOKEN_LEN, CELL_WRITED_FLAG + token);
  writeCharEeprom(getBotTokenAddress(), data, EEPROM_TELEGRAM_CLIENT_ID_LEN);
	
  if(!EEPROM.commit()){
	  free(data);
      return false;
  };

  free(data);
  return true;
}

String readBotToken()
{
  char data[EEPROM_TELEGRAM_BOT_TOKEN_LEN]; 	
  EEPROM.get(getBotTokenAddress(), data); 	
	 		
  return prepareSystemValue(data);	
}