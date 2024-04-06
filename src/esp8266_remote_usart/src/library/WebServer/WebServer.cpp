#include "WebServer.h"

//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);
String webServerContent; //web server content
int webStatusCode; //Web server last status code.
String wifi_stations; //Available access points, from scan.

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

void runWebServer()
{
  server.begin();
}

void handleClient()
{
  server.handleClient();
}
