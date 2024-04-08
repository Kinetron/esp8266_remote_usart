**USART converter to telegram**

Firmware for the ESP-01 module based on the ESP8266 chip.
 
It is written in C++ in the Arduino IDE.
The device is designed for remote control of Cisco equipment via a serial port.
The RX and TX outputs are connected to Cisco. Commands for the serial port are sent via telegram.
The module connects to a wifi network.
This allows you to remotely control the equipment.
It can be used to control any equipment via a serial port.

The firmware has been extended to control the GSM modem.

**Firmware assembly**.

To build the firmware, you need to install the libraries:
FastBot(https://github.com/GyverLibs/FastBot)
PDUlib(https://github.com/mgaman/PDUlib)
esp8266-ping(https://github.com/bluemurder/esp8266-ping)

In ext_config.h set the bot access password and the bot token.


**Initial setup**

After flashing the board, the device will switch to the wifi settings standby mode.
It is necessary to connect to the "esp8266_rt" network
Enter the wifi network parameters and the client ID.

If the client ID is set, the bot will send the data received via USART to the chat with this user.


If the bot has started and connected to the wif i network, a message will be sent to the chat.

**List of available commands:**

<pre>/unlock 9080706050 </pre> - get access to the bot. 9080706050 - password.
It work if <pre>#define DEBUG_MODE</pre> the line is commented
<pre>/hello </pre> - the test command. Bot will respond "Hi".

