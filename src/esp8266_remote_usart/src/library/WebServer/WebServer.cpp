#include "WebServer.h"

const char mainPage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8" />
	<meta name="viewport" content="width=device-width, initial-scale=1.0" />
	<title>Device settings</title>
	<style>
		input[type=submit],
		input[type=button],
		button {
			height: 35px;
			font-size: 20px;
			width: 90%;
			max-width: 300px;
			margin: 8px 5px;
			background-color: #4CAF50;
			border: none;
			border-radius: 8px;
			color: white;
			cursor: pointer;
			padding: 0px 10px;
		}
		body {
			font-family: Verdana;
			margin-top: 15px;
		}

		label {
			white-space: nowrap;
			font-size: 20px;
			margin: 0 5px;
		}
		p {
			font-size: 20px;
			border-radius: 10px;
			background-color: #f2f2f2;
			box-shadow: #aaa 0px 0px 10px;
			margin: 15px;
		}

		.access-token-input 
		{
			width: 400px;
		}

		.wifi-stations-block {
			font-size: 20px;
			border-radius: 10px;
			background-color: #ffffff;
			box-shadow: #fafad2 0px 0px 10px;
			margin: 15px;
		}
	</style>
</head>
<body>
<p>
  Wi-Fi settings
</p>
<p>
	Available stations:<br>
		<div class="wifi-stations-block">
	      @stations
		</div>
</p>
<p>
	<form method='get' action='wifi_set'>
		<table>
			<tr>
				<td>
					<label>SSID </label>
				</td>
				<td>
					<input name='ssid' length=32>
				</td>
			</tr>
			<tr>
				<td>
					<label>Password </label>
				</td>
				<td>
						<input name='pass' length=64>
				</td>
			</tr>
		</table>
			<input type='submit' value="Save wifi">
	</form>
</p>
<p>
	Telegram bot settings<br>
</p>
<p>
	<div>
		Warning: Keep your token secure and store it safely, it can be used by anyone to control your bot.
	</div>
	<br>
	<form method='get' action='bot_set'>
			<table>
			<tr>
				<td>
					<label>Access token</label>
				</td>
					<td>
						<input name='token' length=640 class="access-token-input">
				</td>
			</tr>
			<tr>
				<td>
					<label>Сlient id </label>
				</td>
				<td>
					<input name='client_id' length=64>
				</td>
			</tr>
		</table>
		<input type='submit' value="Save bot">
	</form>
</p>
</body>
</html>
)=====";

const char responsePage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
	<title>Device settings</title>
</head>
<body>
<a href="/">To main</a><br>
@response
</body>
</html>
)=====";


//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);
String webServerContent; //web server content
int webStatusCode; //Web server last status code.
String wifi_stations; //Available access points, from scan.
extern int resetModuleFlag; //Flag for begin reboot esp8266.

void createWebServer()
{
    server.on("/", []() {
	  String main = mainPage;
	  main.replace("@stations", wifi_stations);
      server.send(200, "text/html", main);
    });

    server.on("/wifi_set", []() {
      String ssid = server.arg("ssid");
      String pass = server.arg("pass");
	  String result = "";
      
      if(ssid.length() == 0 || pass.length() == 0 || ssid.length() > EEPROM_CLIENT_SSID_LEN 
      || pass.length() >  EEPROM_CLIENT_PASSWORD_LEN) 
      {
		  result = "{\"Error\":\"Empty ssid or password. Or exceeded max string length.\"}";
      }
      else
      {
        if(!saveWifiSettings(ssid, pass)){
           result =  "{\"Error\":\"Error save to flash.\"}";
        }
        else
        {
          result = "{\"Success\":\"Saved to eeprom... Begin reset to boot into new wifi. Wait 10sec.\"}";
		  resetModuleFlag = 1;          
        }
      }

      String page = responsePage;
	  page.replace("@response", result);
      server.send(200, "text/html", page); 
    });

    server.on("/bot_set", []() {
      String token = server.arg("token");
      String clientId = server.arg("client_id");
	  String result = "OK";
	  
	  if(!saveTelegramClientId(clientId))
	  {
	    String result = "Error!";	
	  }
	  
	  String page = responsePage;
	  page.replace("@response", result);
      server.send(200, "text/html", page);      
    });
}

void runWebServer()
{
  server.begin();
}

void handleClient()
{
  server.handleClient();
}
