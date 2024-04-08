#include "Modem.h"

extern String modemResponse;
extern bool getModemResponse; //=1 if the modem has answered.
extern int modemCommandTimeout; //Timeout for waiting for a command response.
extern void readSerial();
extern bool enabledSendModemResponse;
extern FastBot bot;
extern String tgClientId; //Telegram client id.

String modemResponse = "";
bool getModemResponse = false; //=1 if the modem has answered.
int modemCommandTimeout = MODEM_TIMEOUT; //Timeout for waiting for a command response.

PDU pduDecoder = PDU(1024); //UTF8_BUFFSIZE

//Save the modem's response over the serial port
void onModemResponse(String data)
{
  modemResponse = data;
  modemResponse.trim();
  getModemResponse = true;
}

//Event handler for receiving data on the serial port.
void onSerialReceived()
{
  //SMS has been received: +CMTI: "SM",4
  if(modemResponse.startsWith("+CMTI:"))
  {
    int index = modemResponse.lastIndexOf(',');
    String smsNumber = modemResponse.substring(index + 1);
    bot.sendMessage(readSMS(smsNumber), tgClientId);
  } 	
}

bool sendATCommand(String cmd, bool waiting, String response) 
{
  getModemResponse = false;
  modemResponse = "";
  Serial.println(cmd);
  if (waiting) //If necessary, wait for a response.
  {
    int timeout = 0;
    while (timeout < modemCommandTimeout) 
    {
      delay(DELAY_WAIT_AT_COMMAND);
      readSerial();
      
      int rPos = modemResponse.indexOf("\r");
      if(rPos != -1)
      {
        modemResponse = modemResponse.substring(0, rPos);
      }
    
      if(modemResponse == response)
      {
        return true;  
      }

      //Wait OK.
      if(getModemResponse)
      {
        timeout = 0;
        getModemResponse = false;        
      }

      timeout+= DELAY_WAIT_AT_COMMAND;
    };

    if(!getModemResponse) return false;
  }

  return true;
}

bool sendAtCommandWithoutSendingToBot(String cmd)
{
  enabledSendModemResponse = false;
  bool result = sendATCommand(cmd, true, "OK");
  enabledSendModemResponse = true;
  return result;
}

bool waitModemResponse(String startsWith)
{
  getModemResponse = false;

  long timeout = 0;
  while (timeout < MODEM_TIMEOUT) 
  {
    delay(DELAY_WAIT_AT_COMMAND);
    readSerial();
  
    if(modemResponse.startsWith(startsWith) || startsWith.length() == 0)
    {
      return true;        
    }

    timeout+= DELAY_WAIT_AT_COMMAND; 
  }

  return false;
}

//Check if string is hex.
bool isHexString(String str)
{
   for(int i = 0; i< str.length(); i++)
   {
     if(!isHexadecimalDigit(str[i])) return false;
   }

    return true;
}

bool waitPduModemResponse()
{
  getModemResponse = false;

  long timeout = 0;
  while (timeout < MODEM_TIMEOUT) 
  {
    delay(DELAY_WAIT_AT_COMMAND);
    readSerial();
  
    if(isHexString(modemResponse))
    {
      return true;        
    }

    timeout+= DELAY_WAIT_AT_COMMAND; 
  }

  return false;
}

String readSMS(String smsNumber)
{
  enabledSendModemResponse = false;
  sendATCommand("AT+CMGR=" + smsNumber, false, "");
  if(!waitModemResponse("+CMGR")) //Modem response +CMGR: 1,"",37
  {
    return "ERROR READ:" + modemResponse;
  }
  
  if(!waitPduModemResponse())
  {
    return "ERROR PDU:" + modemResponse;
  }
     
  String pdu = modemResponse;
  if(!waitModemResponse("OK")) //Modem response OK.
  {
    return "ERROR END:" + modemResponse;
  }
  
  enabledSendModemResponse = true;
  
  return decodePdu(pdu.c_str());
}

String decodePdu(String pdu)
{
  int result = pduDecoder.decodePDU(pdu.c_str()); 
  if (result < 0) return "ERROR " + String(result); 

  //The number of characters that did not fit into the buffer.
  String overflow = "";
  if(pduDecoder.getOverflow() > 0)
  {
    overflow = "overflow bufer " +  String(pduDecoder.getOverflow());
  }

  String data = "Content: " + String(pduDecoder.getSender()) + " " + convertPduTimeFormat(String(pduDecoder.getTimeStamp()))
    + " " + String(pduDecoder.getText()) + 
    " " + overflow;

  return data;
}

//Convert YYMM DD HH MM SS to DD.MM.YY HH:MM:SS.
String convertPduTimeFormat(String str)
{
  String result =  str.substring(4, 6) + "."
                  + str.substring(2, 4) + "."
                  + str.substring(0, 2) + " "
                  + str.substring(6, 8) + ":"
                  + str.substring(8, 10) + ":"
                  + str.substring(10, 12);
  return result; 
}

String executeUssd(String ussd)
{	
  if(!sendATCommand("AT+CUSD=1,\""+ ussd + "\"", true, "OK")) return "ERROR";    
  if(!waitModemResponse("+CUSD:")) return "ERROR";
 
  //If the response contains quotation marks, it means there is a message (a guard against "empty" USSD responses).
  if (modemResponse.indexOf("\"") > -1) 
  { 
    String msg = modemResponse.substring(modemResponse.indexOf("\"") + 1);
    msg = msg.substring(0, msg.indexOf("\""));
    return decodeUCS2(msg);      
  }

  return "ERROR"; 
}

void sendSMS(String phone, String message)
{
   //Preparing the PDU package. To save memory, we will use pointers and links.
  String *ptrphone = &phone;
  String *ptrmessage = &message;

  String PDUPack;
  String *ptrPDUPack = &PDUPack; 

  int PDUlen = 0; //Variable for storing the length of a PDU packet without SCA.
  int *ptrPDUlen = &PDUlen;

  getPDUPack(ptrphone, ptrmessage, ptrPDUPack, ptrPDUlen);

  const String cmd[3][2] = 
  {
    { "AT+CMGF=0", "OK"}, //Turning on the PDU mode. 
    { "AT+CMGS=" + (String)PDUlen, ">"}, //We send the length of the PDU packet.
    { PDUPack + (String)((char)26), "OK"} //After the PDU package, we send Ctrl+Z.
  };

  int timeout = modemCommandTimeout;
  modemCommandTimeout = modemCommandTimeout * 2;
  for(int i = 0; i < 3; i++)
  {
    if(!sendATCommand(cmd[i][0], true, cmd[i][1]))
    {
      bot.sendMessage("Timeout send AT command.", tgClientId);
      modemCommandTimeout = timeout;
      return;
    }
  }

  modemCommandTimeout = timeout;
}

//The modem does not send data unless it is accessed once. Perhaps this is not a problem on the modem, but is a usart problem?
void activateModemPort()
{
  enabledSendModemResponse = false;
  sendATCommand("AT", true, "OK");
  enabledSendModemResponse = true;	
}