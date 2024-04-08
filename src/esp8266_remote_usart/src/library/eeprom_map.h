#define INIT_WORD "nofirst"
//If a value has been written in a cell, this value starts from this line.
#define CELL_WRITED_FLAG "wrt"
#define CELL_WRITED_FLAG_LEN 3

//EEPROM map.
#define EEPROM_INIT_WORD_LEN 7 //The value for determining the first run - len of string INIT_WORD "nofirst".
#define EEPROM_CLIENT_SSID_LEN 32 //SSID home wifi len.
#define EEPROM_CLIENT_PASSWORD_LEN 64 //Home wifi password.
#define EEPROM_TELEGRAM_CLIENT_ID_LEN 15 + CELL_WRITED_FLAG_LEN
#define EEPROM_TELEGRAM_BOT_TOKEN_LEN 64 + CELL_WRITED_FLAG_LEN
#define EEPROM_SYSTEM_PASSWORD_LEN 64 + CELL_WRITED_FLAG_LEN