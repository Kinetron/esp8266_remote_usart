#include "CommandExecutor.h"
extern int resetModuleFlag;
extern int mode;
extern bool enabledSendModemResponse;
extern FastBot bot;
extern String modemResponse;
extern int modemCommandTimeout; 

void executeCommand(String msg, String chatID)
{
	if (msg == "/hello")
	{
		bot.sendMessage("Hi people!", chatID);
	}
	else if (msg == "/tg_client_id")
	{
		bot.sendMessage(chatID, chatID);
	}
	else if (msg == "/rst_ext")
	{
		digitalWrite(GPIO2, LOW);
		delayMicroseconds(500000);
		digitalWrite(GPIO2, HIGH);
		bot.sendMessage("OK", chatID);
	}
	else if (msg == "/rst_ext1")
	{
		digitalWrite(GPIO0, LOW);
		delayMicroseconds(500000);
		digitalWrite(GPIO0, HIGH);
		bot.sendMessage("OK", chatID);
	}
	else if (msg == "/reboot_8266")
	{
		bot.sendMessage("OK", chatID);
		resetModuleFlag = 1;
	}
	else if (msg == "/lock") //Lock receiving commands.
	{
		bot.sendMessage("OK", chatID);
		mode = 0;
	}
    else if(msg.substring(0, 4) == "/pdu") //Convert from UCS2.
    {
        String pdu = msg.substring(5);
        bot.sendMessage(decodePdu(pdu), chatID);     
    } 
	else if (msg.substring(0, 4) == "/ucs") //Decode ussd response from UCS2.
	{
		String ucs = msg.substring(5);
		bot.sendMessage(decodeUCS2(ucs), chatID);
	}
	else if (msg.substring(0, 8) == "/enc_ucs") //Encode to UCS2.
	{
		String str = msg.substring(9);
		bot.sendMessage(encodeUCS2(str), chatID);
	}
	else if (msg.substring(0, 9) == "/send_sms")
	{
		String body = msg.substring(10);
		int index = body.lastIndexOf(' ');
		String phone = body.substring(0, index);
		String message = body.substring(index + 1);
		sendSMS(phone, message);
	}
	else if (msg.substring(0, 5) == "/ussd") //Execute USSD.
	{
		String ussd = msg.substring(6);
		enabledSendModemResponse = false;
		bot.sendMessage(executeUssd(ussd), chatID);
		enabledSendModemResponse = true;
	}
	else if (msg.substring(0, 5) == "/call") //Call to number.
	{
		String phone = msg.substring(6);
		if (!sendAtCommandWithoutSendingToBot("ATD " + phone + ";"))
		{
			bot.sendMessage("ERROR " + modemResponse, chatID);
		}
		else
		{
			bot.sendMessage("OK", chatID);
		}
	}
	else if (msg.substring(0, 8) == "/hang_up")
	{
		if (!sendAtCommandWithoutSendingToBot("ATH0"))
		{
			bot.sendMessage("ERROR " + modemResponse, chatID);
		}
		else
		{
			bot.sendMessage("OK", chatID);
		}
	}
	else if (msg.substring(0, 9) == "/read_sms")
	{
		String smsNumber = msg.substring(10);
		bot.sendMessage(readSMS(smsNumber), chatID);
	}
	else if (msg.substring(0, 18) == "/get_modem_timeout")
	{
		bot.sendMessage(String(modemCommandTimeout), chatID);
	}
	else if (msg.substring(0, 18) == "/set_modem_timeout")
	{
		String timeout = msg.substring(19);
		modemCommandTimeout = timeout.toInt();
		bot.sendMessage("OK", chatID);
	}
	else if(msg.substring(0, 18) == "/get_wifi_settings")
    {
	  String ssid = readStringEeprom(EEPROM_INIT_WORD_LEN, EEPROM_CLIENT_SSID_LEN); 
      String password = readStringEeprom(EEPROM_INIT_WORD_LEN + EEPROM_CLIENT_SSID_LEN, EEPROM_CLIENT_SSID_LEN + EEPROM_CLIENT_PASSWORD_LEN);
	  bot.sendMessage("SSID:" + ssid +";PSWD:" + password, chatID);
    }
	else if(msg.substring(0, 5) == "/wifi") //Return SSID and BSSID and RSSI
    {
      bot.sendMessage(getWiFiStatus(), chatID);
    }
	else if(msg.substring(0, 10) == "/scan_wifi")
    {
	   String stations = scanNetworks(); //Get list available networks.
      bot.sendMessage(stations, chatID);
    }	
}