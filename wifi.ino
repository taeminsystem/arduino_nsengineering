#include <list>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <dummy.h>
#include <stdio.h>

int wifiConnected = 0;
int wifiConnecting = 0;
int wifiDisConnecting = 0;
int wifiDisconnected = 0;
int serverConnected = 0;
int wifiTimeout = 0;
int wifiSetupTimeout = 0;
int wifiResetFlag =0;
int udpServerStarted =0;
WiFiClient client;
std::list<std::string> queue;
WiFiUDP Udp;
unsigned int udpPort = 3456;
#define UDP_MAX_SIZE 256
char packetBuffer[UDP_MAX_SIZE]; //buffer to hold incoming packet
char  ReplyBuffer[UDP_MAX_SIZE];

void closeServer()
{
  client.stop();
  serverConnected = 0;
}

void WiFiEventCB(WiFiEvent_t event)
{
   // Serial.printf("[WiFi-event] event: %d\n", event);
    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        wifiConnected = 1;
        wifiConnecting = 0;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        wifiDisconnected = 1;
        wifiConnected = 0;
        if ( serverConnected == 1 ) {
          closeServer();
        }
        if ( wifiDisConnecting ==1 )
          wifiDisConnecting = 0;
        break;
    }   
}

void WiFiSetup()
{
  // Examples of diffrent ways to register wifi events
  WiFi.onEvent(WiFiEventCB);  
  WiFi.disconnect(true);
  wifiConnecting = 1;
  String pw = getPW();
  if ( pw != "" ) {
    //Serial.println(">>>>>> WiFI has password");
    //Serial.println( getPW() );
    WiFi.begin( getSSID(),getPW());
  }else {
    //Serial.println(">>>>>>> WiFI has no - password");
    //WiFi.begin( getSSID() ,  NULL ) ;
  }
  Serial.println("Wait for WiFi Setup... ");
  delay(3000);
  if ( wifiResetFlag == 1 )
    ESP.restart();
}

void WiFiReSetup()
{
    Serial.println("WiFiReSetup called");
    wifiResetFlag = 1;
    wifiSetupTimeout = 0;
    WiFiSetup();
    // add comment
}

void startUdpServer()
{
  Udp.begin():
  udpServerStarted = 1;
  //add comment
}

void udpProcessing()
{
  int packetSize = Udp.parsePacket();
  if ( packetSize) {
    int len = Udp.read(packetBuffer, UDP_MAX_SIZE);
    if ( len >  0 )
    {
      
    }
  }
}

void WiFiLoop()
{
  if ( wifiConnected == 0 )
  {
    // try to setup wifi ing 
    if ( wifiConnecting == 0 ) {
      wifiConnecting = 1;
      WiFiSetup();
    }
    return;
  }

  if ( udpServerStarted == 0 )
  {
    startUdpServer(udpPort);
    return;
  }

  udpProcessing();
  
}

