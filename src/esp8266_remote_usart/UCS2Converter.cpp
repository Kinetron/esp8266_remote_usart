#include "UCS2Converter.h"

//Decode UCS2 to string.
String decodeUCS2(String ucs)
{                   
  String result = "";
  unsigned char tmp[5] = ""; //Array for storing the result.
  for (int i = 0; i < ucs.length() - 3; i += 4) {       //We go through 4 encoding characters each. 
    unsigned long code = (((unsigned int)HexSymbolToChar(ucs[i])) << 12) +    //We get the UNICODE character code from the HEX representation.
                         (((unsigned int)HexSymbolToChar(ucs[i + 1])) << 8) +
                         (((unsigned int)HexSymbolToChar(ucs[i + 2])) << 4) +
                         ((unsigned int)HexSymbolToChar(ucs[i + 3]));
    if (code <= 0x7F) {                     //in accordance with the number of bytes, we form a character.
      tmp[0] = (char)code;                              
      tmp[1] = 0;                           //the final zero
    } else if (code <= 0x7FF) {
      tmp[0] = (char)(0xC0 | (code >> 6));
      tmp[1] = (char)(0x80 | (code & 0x3F));
      tmp[2] = 0;
    } else if (code <= 0xFFFF) {
      tmp[0] = (char)(0xE0 | (code >> 12));
      tmp[1] = (char)(0x80 | ((code >> 6) & 0x3F));
      tmp[2] = (char)(0x80 | (code & 0x3F));
      tmp[3] = 0;
    } else if (code <= 0x1FFFFF) {
      tmp[0] = (char)(0xE0 | (code >> 18));
      tmp[1] = (char)(0xE0 | ((code >> 12) & 0x3F));
      tmp[2] = (char)(0x80 | ((code >> 6) & 0x3F));
      tmp[3] = (char)(0x80 | (code & 0x3F));
      tmp[4] = 0;
    }
    result += String((char*)tmp); // Добавляем полученный символ к результату
  }
  return (result);
}

unsigned char HexSymbolToChar(char hex) {
  if      ((hex >= 0x30) && (hex <= 0x39)) return (hex - 0x30);
  else if ((hex >= 'A') && (hex <= 'F'))   return (hex - 'A' + 10);
  else                                 return (0);
}

//Encodes a string in UCS2.
String encodeUCS2(String str)
{
  String output = ""; //A variable for storing the result.

  //Iterate through all bytes in the input string.
  for (int k = 0; k < str.length(); k++) 
  {                            
    byte actualChar = (byte)str[k]; //Get first byte.
    unsigned int charSize = getCharSizeUcs2(actualChar); //Get the length of the character - the number of bytes.

    //The maximum character length in UTF-8 is 6 bytes plus a trailing zero, for a total of 7.
    char symbolBytes[charSize + 1];                                 //Declaring an array according to the received size.
    for (int i = 0; i < charSize; i++)  symbolBytes[i] = str[k + i];  //We write to the array all the bytes that encode the character.
    symbolBytes[charSize] = '\0';                                   //Adding the final 0. 

    unsigned int charCode = symbolToUIntDec(symbolBytes);              //We get the DEC representation of a character from a set of bytes. 
    if (charCode > 0)  {                                            //If everything is correct, we convert it to a HEX string.
      //It remains to convert each of the 2 bytes to HEX format, convert it to a string and assemble it into a pile.
      output += byteToHexString((charCode & 0xFF00) >> 8) +
                byteToHexString(charCode & 0xFF);
    }
    k += charSize - 1; //Move the pointer to the beginning of a new character. 
  }
  return output;
}

//The number of bytes that the character is encoded.
unsigned int getCharSizeUcs2(unsigned char symbol) 
{
// According to the UTF-8 encoding rules, the total size of the character is calculated using the highest bits of the first octet
// 1 0xxxxxxx - the highest bit is zero (ASCII code is the same as UTF-8) - a character from the ASCII system, encoded in one byte
// 2 110xxxxx - the two highest bits of the unit - the character is encoded in two bytes
// 3 1110xxxx - 3 bytes, etc.
// 4 11110xxx
// 5 111110xx
// 6 1111110x

  if (symbol < 128) return 1; //If the first byte is from the ASCII system, then it is encoded with one byte.

 // Next, you need to calculate how many units in the higher bits are up to the first zero - this will be the number of bytes per character.
 // Using a mask, we exclude the highest bits in turn, until it reaches zero.
  for (int i = 1; i <= 7; i++) {
    if (((symbol << i) & 0xFF) >> 7 == 0) {
      return i;
    }
  }
  return 1;
}

//Function for converting a numeric byte value to hexadecimal (HEX).
String byteToHexString(byte value)
{ 
  String hex = String(value, HEX);
  if (hex.length() == 1) hex = "0" + hex;
  hex.toUpperCase();
  return hex;
}

//Function for getting the DEC representation of a symbol.
unsigned int symbolToUIntDec(const String& bytes)
{  
  unsigned int charSize = bytes.length(); //The number of bytes that the character is encoded with.
  unsigned int result = 0;
  if (charSize == 1)
  {
    return bytes[0]; //If the character is encoded in one byte, we immediately send it.
  }
  else
  {
    unsigned char actualByte = bytes[0];
	//For the first byte, we leave only the significant part 1110XXXX - remove at the beginning 1110, leave XXXX
	//The number of units at the beginning coincides with the number of bytes that encode the character - remove them
	//For example (for a size of 2 bytes), take the mask 0xFF (11111111) - shift it (>>) by the number of unnecessary bit (3 - 110) - 00011111
    result = actualByte & (0xFF >> (charSize + 1)); // Было 11010001, далее 11010001&(11111111>>(2+1))=10001
    //Each next byte starts with 10 XXX XXX - we only need 6 bits from each subsequent byte
	//And since there is only 1 byte left, we reserve space for it:
    result = result << (6 * (charSize - 1)); // Было 10001, далее 10001<<(6*(2-1))=10001000000

    //Now for each next bit, remove the unnecessary 10XXXXXX bits, and add the remaining ones to the result according to the location.
    for (int i = 1; i < charSize; i++)
	{
      actualByte = bytes[i];
      if ((actualByte >> 6) != 2) return 0; //If the byte does not start with 10, it means an error - exit.
	  //To continue the example, a significant part of the next byte is taken
      //For example, at 10011111, we remove 10 with a mask (bits at the beginning), 11111 remains
      //Now we shift them by 2-1-1 = 0, you do not need to shift them, just add them to their place
      result |= ((actualByte & 0x3F) << (6 * (charSize - 1 - i)));
	  //There was result=1000000, actual bytes=10011111. Mask actualByte & 0x3F (10011111&111111=11111), no need to shift
      //Now we "dock" to result: result|11111 (1000000/11111=10001011111)
    }
    return result;
  }
}

//Create a PDU packet and calculates the length of the packet without SCA.
void getPDUPack(String *phone, String *message, String *result, int *PDUlen)
{
  //We will add the SCA field at the very end, after calculating the length of the PDU packet.
  *result += "01";                                //Field PDU-type - байт 00000001b
  *result += "00";                                //Field MR (Message Reference)
  *result += getDAfield(phone, true);             //Field DA
  *result += "00";                                //Field PID (Protocol Identifier)
  *result += "08";                                //Field DCS (Data Coding Scheme)
  //*result += "";                                //Field VP (Validity Period) - not use.

  String msg = encodeUCS2(*message);            //Convert to UCS2.

  *result += byteToHexString(msg.length() / 2);   // Field UDL (User Data Length). Divide by 2, since in the UCS2 string each encoded character is represented by 2 bytes.
  *result += msg;

  *PDUlen = (*result).length() / 2;               //We get the length of the PDU packet without the SCA field.
  *result = "00" + *result;                       //Add field SCA.
}

String getDAfield(String *phone, bool fullnum) {
  String result = "";
  for (int i = 0; i <= (*phone).length(); i++) 
  {  //We leave only the numbers.
    if (isDigit((*phone)[i])) {
      result += (*phone)[i];
    }
  }
  int phonelen = result.length();                 //The number of digits in the phone.
  if (phonelen % 2 != 0) result += "F";           //If the number of digits is odd, add F.

  for (int i = 0; i < result.length(); i += 2) //We rearrange the characters in the number in pairs.
  {
    char symbol = result[i + 1];
    result = result.substring(0, i + 1) + result.substring(i + 2);
    result = result.substring(0, i) + (String)symbol + result.substring(i);
  }

  result = fullnum ? "91" + result : "81" + result; //Adding the format of the recipient's number, the PR field.
  result = byteToHexString(phonelen) + result;    //Adding the length of the number, the PL field.

  return result;
}