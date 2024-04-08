#include "PingWatchdog.h"

extern "C"
{
  #include <lwip/icmp.h> // needed for icmp packet definitions
}

Pinger pinger; // Set global to avoid object removing after setup() routine.

void initPingWatchdog()
{
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
}

void pingHost()
{
  //Check connection. 
  if(pinger.Ping(PING_IP) == false)
  {
    ESP.restart();
  } 	
}