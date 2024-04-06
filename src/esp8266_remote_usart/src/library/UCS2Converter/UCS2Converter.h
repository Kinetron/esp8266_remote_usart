#include <Arduino.h>
#include <string.h>

//Decode UCS2 to string.
String decodeUCS2(String ucs);

unsigned char HexSymbolToChar(char hex);

//Encodes a string in UCS2.
String encodeUCS2(String str);

//The number of bytes that the character is encoded.
unsigned int getCharSizeUcs2(unsigned char symbol);

//Function for converting a numeric byte value to hexadecimal (HEX).
String byteToHexString(byte value);

//Function for getting the DEC representation of a symbol.
unsigned int symbolToUIntDec(const String& bytes);

//Create a PDU packet and calculates the length of the packet without SCA.
void getPDUPack(String *phone, String *message, String *result, int *PDUlen);

String getDAfield(String *phone, bool fullnum);