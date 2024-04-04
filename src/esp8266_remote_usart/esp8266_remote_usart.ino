/*
 Default IP address 192.168.4.1 for web page.
*/
#include <ESP8266WiFi.h>
#include <FastBot.h>
#include <AsyncStream.h>
#include <pdulib.h>
#include <Pinger.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "ext_config.h" //Private param. Bot password, bot id.

extern "C"
{
  #include <lwip/icmp.h> // needed for icmp packet definitions
}

#define DEFAULT_SSID "esp8266_rt"
#define GPIO2 2
#define GPIO0 0
#define TIMER0_DIV_VALUE 80000000L * 10 //Сlock frequency 80MHz, 10sec interrupt.
#define PING_IP "8.8.8.8"
#define EEPROM_INIT_WORD_LEN 7 //The value for determining the first run - len of string INIT_WORD "nofirst".
#define INIT_WORD "nofirst"
#define EEPROM_CLIENT_SSID_LEN 32 //SSID home wifi len.
#define EEPROM_CLIENT_PASSWORD_LEN 64 //Home wifi password.
#define WIFI_СONNECTION_ATTEMPTS 30 //Max attempts count when device reboot.
#define WAIT_PRESS_FW_BTN_ATTEMPTS 8
#define DELAY_WAIT_PRESS_FW_BTN_ATTEMPTS 250

String lastChatId = ""; //Telegram chat id.
String webServerContent; //web server content
String wifi_stations; //Available access points, from scan.
int webStatusCode; //Web server last status code.
PDU pduDecoder = PDU(1024); //UTF8_BUFFSIZE

bool led_on = false; //флаг состояния светодиода

//Work mode.
//0-lock.
int mode = 0;
bool beginReset = 0; //Поступила команда перезагрузки esp8266.

FastBot bot(BOT_TOKEN);
AsyncStream<512> serial(&Serial, '\n'); // указали Stream-объект и символ конца
Pinger pinger; // Set global to avoid object removing after setup() routine.
//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);

void setup() {
  pinMode(GPIO2, OUTPUT);
  pinMode(GPIO0, OUTPUT);
  digitalWrite(GPIO2, HIGH);
  digitalWrite(GPIO0, HIGH);
  Serial.begin(9600);

  EEPROM.begin(1000); //Initialasing EEPROM
  delay(250);
  if(firstRunCheck())
  {
    initWebServer();
    Serial.println("init web");
    return;
  }

  String ssid = readStringEeprom(EEPROM_INIT_WORD_LEN, EEPROM_CLIENT_SSID_LEN); 
  String password = readStringEeprom(EEPROM_INIT_WORD_LEN + EEPROM_CLIENT_SSID_LEN, EEPROM_CLIENT_SSID_LEN + EEPROM_CLIENT_PASSWORD_LEN);
  
  if(!connectWiFi(ssid.c_str(), password.c_str()))
  {
    if(waitPressFwButton()) //Check if enabled set wifi settings mode.
    {      
      initWebServer();
      return;
    }    
    
    ESP.restart();
  }
  
  bot.attach(oNnewMsg);
  noInterrupts();
  timer0_isr_init(); //Timer interrupts.
  timer0_attachInterrupt(timer0_interrupt_handler); //настраиваем прерывание (привязка к функции)
  timer0_write(ESP.getCycleCount() + TIMER0_DIV_VALUE); //Тактовая частота 80MHz, получаем секунду
  interrupts();

  pinger.OnReceive([](const PingerResponse& response)
  {    
    // Return true to continue the ping sequence.
    // If current event returns false, the ping sequence is interrupted.
    return true;
  });

  pinger.OnEnd([](const PingerResponse& response)
  {
    // Evaluate lost packet percentage
    float loss = 100;
    if(response.TotalReceivedResponses > 0)
    {
      loss = (response.TotalSentRequests - response.TotalReceivedResponses) * 100 / response.TotalSentRequests;
    }
    
  if((int)loss == 100)
    {
      ESP.restart();
    }

     return true;
  });

  ESP.wdtEnable(5000);
}

void loop() {
//if (mode == 2 && Serial.available()) 
   if (serial.available()) {     // если данные получены
     bot.sendMessage(String(serial.buf), lastChatId);
  }
  bot.tick();

  //Поступила команда перезагрузки модуля.
  if(beginReset == 1)
  {
     delay(1000);
     //ESP.restart();
     beginReset = 0;
  }
}

//Init ap, web server and wait client settings.
void initWebServer()
{
     initAp();
     createWebServer(); // Start the server
     server.begin();

     while ((WiFi.status() != WL_CONNECTED))
     {
       delay(200);
       server.handleClient();
     }     
}

//Check if device first run.
bool firstRunCheck()
{
  String initWord = readStringEeprom(0, EEPROM_INIT_WORD_LEN);
  if(initWord == INIT_WORD) return false;

  return true;
}

//Enable AP.
void initAp()
{
  WiFi.mode(WIFI_STA);
  delay(500);
  wifi_stations = scanNetworks(); //Get list available networks.
  
  WiFi.softAP(DEFAULT_SSID, "");
}

String scanNetworks(){
  WiFi.disconnect();
  delay(100);

  int networks = WiFi.scanNetworks();
 
  if (networks == 0)
  {
     return "No networks found";
  }

  String stations = "<ol>";
  for (int i = 0; i < networks; ++i)
  {
    // Print SSID and RSSI for each network found
    stations += "<li>";
    stations += WiFi.SSID(i);
    stations += " (";
    stations += WiFi.RSSI(i);
 
    stations += ")";
    stations += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    stations += "</li>";
  }
  stations += "</ol>";
  return stations;
}

// обработчик сообщений
void oNnewMsg(FB_msg& msg) {
 lastChatId = msg.chatID;
commandHandler(msg.text, msg.chatID);
return;

//Поведение в зависимости от режима работы.
   switch(mode){
     //Перевести бот в режим приема пароля.
     case 0:
          if (msg.text == "/unlock")
          {
            mode = 1; 
          }
     break;

    //Режим приема пароля активации. 
     case 1:
          if (msg.text != ACTIVATION_PASSWORD)
          {
            mode = 0;             
          }
          else
          {
            mode = 2;
            bot.sendMessage("OK", msg.chatID);
          }
         
     break;

     //Режим обработки команд.
     case 2:
       commandHandler(msg.text, msg.chatID);
     break;     
   }   
}

bool connectWiFi(const char *ssid, const char *password) {
  delay(2000);
  WiFi.begin(ssid, password);
  int attempts = 0; 
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    if(attempts > WIFI_СONNECTION_ATTEMPTS)
    {
      return false;
    }
    attempts ++;
  }
  return true;
}

//Wait waits for the user to press the button and release it.
bool waitPressFwButton()
{
   pinMode(GPIO0, INPUT);
   int count = 0;
   bool pressed = false;
   
   while(count < WAIT_PRESS_FW_BTN_ATTEMPTS)
   {
     int state = digitalRead(GPIO0);
     if(state == LOW)
     {       
       pressed = true;
       digitalWrite(GPIO0, LOW);
       Serial.println("Press fw btn");
       break;
     }
     delay(DELAY_WAIT_PRESS_FW_BTN_ATTEMPTS);
     count ++;
   }   
   
   if(pressed == false) return false;
   count = 0;
   

   while(count < WAIT_PRESS_FW_BTN_ATTEMPTS)
   {
     int state = digitalRead(GPIO0);
     if(state == HIGH)
     {
       Serial.println("Release fw btn");
       digitalWrite(GPIO0, HIGH);
       return true;
     }
     delay(DELAY_WAIT_PRESS_FW_BTN_ATTEMPTS);
     count ++;
   }

   return false; 
}

//Обрабатывает команды.
void commandHandler(String msg, String chatID)
{
    //Обработка команд предназначенных для трансляции через usart.
    if(msg[0] == 'u')
    {
       String atCommand = msg.substring(2); //Обрезаем u пробел в начале строки
       Serial.println(atCommand);   
    }
    else{
        executionNoUsartCommand(msg, chatID);
    }
}

//Выполняет внутренние команды, не требующие передачи по usart.
void executionNoUsartCommand(String msg, String chatID)
{
      if (msg == "/hello")
        bot.sendMessage("Hi people!", chatID);
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
      else if (msg == "/reset_8266")
      {
        bot.sendMessage("OK", chatID);
        beginReset = 1;
      }
      //Заблокировать прием команд.
      else if (msg == "/lock")
      {
         mode = 0;
      }
      else if(msg.substring(0, 4) == "/pdu") //Конвертация сообщений из UCS2.
      {
        String pdu = msg.substring(5);
        int result = pduDecoder.decodePDU(pdu.c_str()); 
        if (result < 0) 
        {
          bot.sendMessage("ERROR " + String(result) + msg.substring(4), chatID);
          return;
        }

        //Количество символов не влезщих в буфер
        String overflow = "";
        if(pduDecoder.getOverflow() > 0)
        {
          overflow = "overflow bufer " +  String(pduDecoder.getOverflow());
        }

        String data = String(pduDecoder.getSender()) + " " + String(pduDecoder.getTimeStamp())
         + " " + String(pduDecoder.getText()) + 
        " " + overflow;
        bot.sendMessage(data, chatID);     
      }
}

void timer0_interrupt_handler(void)
{
   if (led_on)
   {
     digitalWrite(GPIO0, HIGH);
     led_on = false;
   }
   else
   { 
    digitalWrite(GPIO0, LOW);
    led_on = true;
   }
   //Check connection. 
   if(pinger.Ping(PING_IP) == false)
   {
     ESP.restart();
   } 
  timer0_write(ESP.getCycleCount() + TIMER0_DIV_VALUE); //Тактовая частота 80MHz, получаем секунду  
}

String readStringEeprom(int beginPos, int endPos)
{
  String str;
  for (int i = beginPos; i < endPos; ++i)
  {
    str += char(EEPROM.read(i));
  }
  return str;
}

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

void createWebServer()
{
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      webServerContent = "<!DOCTYPE HTML>\r\n<html>Esp remote settings ";
      webServerContent += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      webServerContent += ipStr;
      webServerContent += "<p>";
      webServerContent += wifi_stations;
      webServerContent += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
      webServerContent += "</html>";
      server.send(200, "text/html", webServerContent);
    });

    server.on("/scan", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
 
      webServerContent = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", webServerContent);
    });
 
    server.on("/setting", []() {
      String ssid = server.arg("ssid");
      String pass = server.arg("pass");
      
      if(ssid.length() == 0 || pass.length() == 0 || ssid.length() > EEPROM_CLIENT_SSID_LEN 
      || pass.length() >  EEPROM_CLIENT_PASSWORD_LEN) 
      {
        webServerContent = "{\"Error\":\"Empty ssid or password. Or exceeded max string length.\"}";
        webStatusCode = 404;
      }
      else
      {
        if(!saveWifiSettings(ssid, pass)){
           webServerContent = "{\"Error\":\"Error save to flash.\"}";
           webStatusCode = 500;
        }
        else
        {
          webServerContent = "{\"Success\":\"Saved to eeprom... Begin reset to boot into new wifi.\"}";
          webStatusCode = 200;
          delay(2000);
          ESP.reset();
        }
      }

      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(webStatusCode, "application/json", webServerContent); 
    });
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