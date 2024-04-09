/*
 Default IP address 192.168.4.1 for web page.
*/
#include <FastBot.h>
#include <AsyncStream.h>
#include <ESP8266WebServer.h>
#include "src/library/UCS2Converter/UCS2Converter.h"
#include "src/library/SystemSettings/SystemSettings.h"
#include "src/library/WebServer/WebServer.h"
#include "src/library/CommandExecutor/CommandExecutor.h"
#include "src/library/WiFiTools/WiFiTools.h"
#include "src/library/PingWatchdog/PingWatchdog.h"

#include "ext_config.h" //Private param. Bot password, bot id.

#define FIRMWARE_VERSION "1.0.2"

#define BLUE_LED_PIN 1
#define DEBUG_MODE //Disabled password access to bot.
#define TIMER0_DIV_VALUE 80000000L * 10 //Сlock frequency 80MHz, 10sec interrupt.
#define WAIT_PRESS_FW_BTN_ATTEMPTS 8
#define DELAY_WAIT_PRESS_FW_BTN_ATTEMPTS 250

String tgClientId = ""; //Telegram client id.
extern String wifi_stations; //Available access points, from scan.

//Work mode.
//0-lock.
int mode = 0;
int resetModuleFlag = 0; //Flag for begin reboot esp8266.
bool enabledSendModemResponse = true; //Enabled send serial data(from USART) to bot if it need.

FastBot bot(BOT_TOKEN);
AsyncStream<512> serial(&Serial, '\n'); // указали Stream-объект и символ конца

void setup() {
  pinMode(GPIO2, OUTPUT);
  pinMode(GPIO0, OUTPUT);
  digitalWrite(GPIO2, HIGH);
  digitalWrite(GPIO0, HIGH);
  Serial.begin(9600);

  EEPROM.begin(512); //Initialasing EEPROM
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
      blinkBlueLed();
      blinkBlueLed();
      blinkBlueLed();
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
  
  initPingWatchdog();
  
  ESP.wdtEnable(5000);
  tgClientId = readTelegramClientId(); //Read client id for send messages to telegram.
  
  //The modem does not send data unless it is accessed once. Perhaps this is not a problem on the modem, but is a usart problem?
  activateModemPort();

  bot.sendMessage("The device is loaded. FW Version " + String(FIRMWARE_VERSION), tgClientId);
}

void readSerial()
{
  if(serial.available())
  {
    String data = String(serial.buf);
    onModemResponse(data);
    if(enabledSendModemResponse)
    {
      bot.sendMessage(data, tgClientId);
    }    
  }
}

void loop() {

  bool allowSend = false;
#ifdef DEBUG_MODE
  allowSend = true; 
#else
  if(mode == 2) allowSend = true; 
#endif
  
  //If data receive.
  if (allowSend) 
  {
    readSerial();
    onSerialReceived();
  }
  
  bot.tick(); 
}

//Init ap, web server and wait client settings.
void initWebServer()
{
     initAp();
     createWebServer(); // Start the server
     runWebServer();

     while ((WiFi.status() != WL_CONNECTED))
     {
       delay(200);
       handleClient();
     }     
}

//Check if device first run.
bool firstRunCheck()
{
  String initWord = readStringEeprom(0, EEPROM_INIT_WORD_LEN);
  if(initWord == INIT_WORD) return false;

  return true;
}

//Message handler.
void oNnewMsg(FB_msg& msg)
{
  //Update firmware.
  if (msg.OTA && msg.text == FIRMWARE_UPDATE_PASSWORD) 
  {
    int status = bot.update();
    if(status != 1)  bot.sendMessage(String(status), msg.chatID);     
  }

#ifdef DEBUG_MODE
  commandHandler(msg);
  return;
#endif

//The behavior depends on the operating mode.
  switch(mode){
     //Switch bot to waiting for the password.
    case 0:
          if (msg.text.substring(0, 7) == "/unlock")
          {                        
             String pswd = msg.text.substring(8);
             if(pswd == BOT_ACCESS_PASSWORD)
             {
              mode = 1;
              bot.sendMessage("OK", msg.chatID);
             }
             else
             {
               delay(10000); 
             }            
          }
    break;

    //Command handler mode.
    case 1:
       commandHandler(msg);
    break;

    default:
    break;   
   }   
}

//Wait waits for the user to press the button and release it.
bool waitPressFwButton()
{
   pinMode(GPIO0, INPUT);
   pinMode(BLUE_LED_PIN, OUTPUT); 
   int count = 0;
   bool pressed = false;
   
   while(count < WAIT_PRESS_FW_BTN_ATTEMPTS)
   {
     int state = digitalRead(GPIO0);
     if(state == LOW)
     {       
       pressed = true;
       digitalWrite(BLUE_LED_PIN, LOW);
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
       digitalWrite(BLUE_LED_PIN, HIGH);
       return true;
     }
     delay(DELAY_WAIT_PRESS_FW_BTN_ATTEMPTS);
     count ++;
   }

   return false; 
}

void blinkBlueLed()
{
  pinMode(BLUE_LED_PIN, OUTPUT);
  digitalWrite(BLUE_LED_PIN, LOW);
  delay(500);
  digitalWrite(BLUE_LED_PIN, HIGH);
  delay(500);
}

void commandHandler(FB_msg& data)
{
    //Command for sent to usart.
    if(data.text[0] == 'u')
    {
       String atCommand = data.text.substring(2); //Cut 'u' and ' '.
       Serial.println(atCommand);   
    }
    else
    {    
      executeCommand(data.text, data.chatID);
      executeSystemCommands(data.text, data.chatID); //Execute commands to configure the system.
    }
}

void timer0_interrupt_handler(void)
{
  pingHost();

   //Process module reset.
   switch(resetModuleFlag)
   {
    case 1: resetModuleFlag = 2; //Wait 10sec for some time to send answer in telegram.
    break;
    case 2:
      ESP.restart();
    break;
    default: break;
   } 

  timer0_write(ESP.getCycleCount() + TIMER0_DIV_VALUE); //Тактовая частота 80MHz, получаем секунду  
}