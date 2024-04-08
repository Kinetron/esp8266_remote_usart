**USART converter to telegram**

Firmware for the ESP-01 module based on the ESP8266 chip.
 
It is written in C++ in the Arduino IDE.

The device is designed for remote control of Cisco equipment via a serial port.
The RX and TX outputs are connected to Cisco. Commands for the serial port are sent via telegram messenger.

The module connects to a WiFi network.
This allows you to remotely control the equipment.
This is very necessary when setting up new equipment and various tests.
It can be used to control any equipment via a serial port.

The firmware has been extended to control the GSM modem.

The device has a ping watchdog. By default ping 8.8.8.8 host. If there is no internet for 10 seconds, the device restart.

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

<pre>/lock</pre> - lock receiving commands. Response - "OK".


<pre>/hello </pre> - the test command. Bot will respond "Hi".

<pre>/wifi</pre>  - return SSID and BSSID and RSSI. Response - "home 01:02:03:04:05:06 -60"

<pre>/scan_wifi</pre> - get list available networks. Response - "Tp-link-099  (48%) protected; home (46%) protected"

<pre>/get_wifi_settings</pre> - returns the current settings for connecting to a wifi network. Response - "SSID: home; PSWD:30040011"

<pre>/tg_client_id</pre> - returns telegram clien id for send any data from serial port. Response - "123"; 

<pre>/reboot_8266</pre> - reboot device after 20seconds. Response - "OK".

<pre>/rst_ext</pre> - set low level on GPIO2, wait 0.5sec, set high level.
Response - "OK".

<pre>/rst_ext1</pre> - set low level on GPIO0, wait 0.5sec, set high level. Response - "OK".

<pre>/pdu 07917777140230F2040C9188885419999900001280018153832106D17B594ECF03</pre> - decode from pdu. Response - "+888845919999 10.08.21 18:35:38 Qwerty".

<pre>/ucs 002a003100350030002a0032002a00330032003300390031002a00360039003100370037002a00310023</pre> - decodes the result of the USSD response in UCS2 format.
Response - "150 2 32391 69177 1#".

<pre>/enc_ucs test</pre> - encode string to UCS2. Response - "0074006500730074".


**Modem commands**

These commands can be executed if a gsm modem is connected.  This is converted into a set of AT commands.

<pre>/ussd *161#</pre> - executes a ussd request. Response - "Your number:12345678"

<pre>/send_sms 12345678 hello</pre> - send SMS to number.
 Response - "OK"

<pre>/call 12345678</pre> - call to number. Response - "OK"

<pre>/hang_up</pre> - hang up if you call to number. Response - "OK"

<pre>/read_sms 1</pre> - read received message with a number 1. Response - "Content: 01.01.24 00:15:00 Happy New Year".

<pre>/get_modem_timeout</pre> - timeout for wait execution of the AT command. Response - "25000". Is 25 second.

<pre>/set_modem_timeout 30000</pre> - set timeout for wait execution of the AT command. Response - "OK". After reboot it clear to default value 25 second.


**System settings**
To enter the system settings, you must enter a password. The default password is set in SystemSettings.h.

<pre>/login Yotek901</pre> - gets access to change system settings.
 Response - "OK".

<pre>/login Yotek901</pre> - gets access to change system settings.
 Response - "OK".

<pre>/logout</pre> - exit from system settings.
 Response - "OK".

<pre>/set_ssid home</pre> - set SSID for to connect to a wireless network.
 Response - "OK".

 <pre>/set_wifi_password 12345678</pre> - set SSID for to connect to a wireless network.
 Response - "OK".

 <pre>/set_telegram_client_id 123</pre> - set the ID of the client to which all messages from the serial port will be sent
 Response - "OK".

 <pre>/get_telegram_client_id</pre> - set the ID of the client to which all messages from the serial port will be sent
 Response - "OK".
