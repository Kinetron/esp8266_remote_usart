#include <Arduino.h>
#include <string.h>
#include <pdulib.h>
#include <FastBot.h>
#include "../UCS2Converter/UCS2Converter.h"

#define DELAY_WAIT_AT_COMMAND 50
#define MODEM_TIMEOUT 25000

//Save the modem's response over the serial port
void onModemResponse(String data);
//Event handler for receiving data on the serial port.
void onSerialReceived();

bool sendATCommand(String cmd, bool waiting, String response);
bool waitModemResponse(String startsWith);
String readSMS(String smsNumber);
bool waitPduModemResponse();

//Check if string is hex.
bool isHexString(String str);

bool waitPduModemResponse();

String decodePdu(String pdu);
String convertPduTimeFormat(String str);
bool sendAtCommandWithoutSendingToBot(String cmd);
String executeUssd(String ussd);
void sendSMS(String phone, String message);

//The modem does not send data unless it is accessed once. Perhaps this is not a problem on the modem, but is a usart problem?
void activateModemPort();