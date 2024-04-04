#include <ESP8266WiFi.h>
#include <FastBot.h>
#include <AsyncStream.h>
#include <pdulib.h>
#include <Pinger.h>
#include <Ticker.h>
#include "ext_config.h" //Private param. Bot password,id, wifi password etc.

Ticker sec;

extern "C"
{
  #include <lwip/icmp.h> // needed for icmp packet definitions
}

#define GPIO2 2
#define GPIO0 0
#define TIMER0_DIV_VALUE 80000000L * 10 //Сlock frequency 80MHz, 10sec interrupt.
#define PING_IP "8.8.8.8"

String lastChatId = ""; //Telegram chat id.
PDU pduDecoder = PDU(1024); //UTF8_BUFFSIZE

bool led_on = false; //флаг состояния светодиода

//Work mode.
//0-lock.
int mode = 0;
bool beginReset = 0; //Поступила команда перезагрузки esp8266.

FastBot bot(BOT_TOKEN);
AsyncStream<512> serial(&Serial, '\n'); // указали Stream-объект и символ конца
Pinger pinger; // Set global to avoid object removing after setup() routine.

void setup() {
  pinMode(2, OUTPUT);
  pinMode(0, OUTPUT);
  digitalWrite(GPIO2, HIGH);
  digitalWrite(GPIO0, HIGH);
  Serial.begin(9600);
  connectWiFi();
  
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

void connectWiFi() {
 delay(2000);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (millis() > 15000) ESP.restart();
  }
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
      if (msg == "/gav_gav")
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

