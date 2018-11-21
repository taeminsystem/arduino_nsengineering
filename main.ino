#include <ETH.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiSTA.h>
#include <WiFiType.h>
#include <WiFiUdp.h>

#include <SPIFFS.h>
#include <SSD1306.h>
#include <ADS1231.h>
#include <ADS1231.h>
#include <dummy.h>
// ssd1306
SSD1306 oled(0x3c,21,22);
#define FORMAT_SPIFFS_IF_FAILED true
// touch pin setting
const byte touch1Pin = 13;
const byte touch2Pin = 12;
const byte touch3Pin = 14;
const byte touch4Pin = 27;
// buzzer pin setting
const byte buzzerPin = 32;
// pin setting
const byte fullPointPin = 2;
const byte zeroPointPin = 15;

//  motor pin setting
const byte pulsePin = 16;
const byte directionPin = 26;
const byte holdPin = 25;
const byte pwmPin = 33;

volatile int _motorPulseCounter = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// motor related function
void initMotor();
void motorPulseInterruptHandler(volatile int *counter);
void zeroPointFoundHandler();
void fullPointFoundHandler();

void pulseCounterClear()
{
  _motorPulseCounter = 0;
}

int getPulseCounter()
{
  return _motorPulseCounter;
}

void IRAM_ATTR handlePulseInterrupt() 
{
  portENTER_CRITICAL_ISR(&mux);
  _motorPulseCounter++;
  portEXIT_CRITICAL_ISR(&mux);
  motorPulseInterruptHandler(&_motorPulseCounter);
}

void IRAM_ATTR zeroPointPinInterrupt() 
{
  zeroPointFoundHandler(); 
}

void IRAM_ATTR fullPointPinInterrupt() 
{
  fullPointFoundHandler();
}

void WiFiLoop();
void wifiLoopThread(void *arg)
{
  while(1)
  {
    delay(50);
    WiFiLoop();
  }
}

ADS1231 ads1231(5,18,19,17);
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  ads1231.begin(false,true);
  
  Serial.println("Load setting...");
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
        Serial.println("SPIFFS Mount Failed");
        return;
  }
  loadSetting(SPIFFS);
  
  pinMode(pulsePin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pulsePin), handlePulseInterrupt, FALLING); 
  pinMode( fullPointPin, INPUT );
  attachInterrupt(digitalPinToInterrupt(fullPointPin),  fullPointPinInterrupt, RISING);
  pinMode( zeroPointPin, INPUT );
  attachInterrupt(digitalPinToInterrupt(zeroPointPin), zeroPointPinInterrupt, RISING);

  pinMode(directionPin, OUTPUT );
  pinMode(holdPin, OUTPUT );
  pinMode(pwmPin, OUTPUT );
  digitalWrite(holdPin , 0 ) ;
  digitalWrite(directionPin, 1) ;        
  initMotor();

  pinMode(buzzerPin, OUTPUT );
  digitalWrite( buzzerPin, 0 );
  oled.init();
  oled.setFont(ArialMT_Plain_24);
  //oled.drawString(0,0,"Hello World");
  //oled.display();
  xTaskCreate(wifiLoopThread, "WiFi Loop" , 8192 , NULL , 1, NULL );
}

long guageValue;
long guageOffset;
int readByte;
int menuFlag = 0;
long minValue =0;
long maxValue =0;
int positionRatio=0;
int offset;
int command=0;
char positionInput[4];
int inputIndex=0;
void resetMenuState()
{
  inputIndex = 0;
  menuFlag = 0;
  command = 0;
}
void setGuageOffset()
{
  ads1231.read(guageValue);
  guageOffset = guageValue;
  Serial.print("Guage offset: ");
  Serial.println(guageOffset);
  //maxValue = guageOffset;
}
void printMenu()
{
  Serial.println("C: Wifi-Connect, W: Wifi SSID, P: Wifi Password, Z: zero, U: up, D: down, S: setup, H: hold, Number");
}

void printValue()
{
  if ( menuFlag == 1 ) 
    return;
    
  if ( guageValue < minValue )
    minValue = guageValue ;
  if ( guageValue > maxValue )
    maxValue = guageValue ;
    
  Serial.printf("%d,%ld,%ld,%ld\r\n" , positionRatio,guageValue - guageOffset ,  minValue- guageOffset, maxValue-guageOffset );
}

void positionUp()
{
  if ( positionRatio == 100 )
    return;
  ++positionRatio;
  Serial.printf("Moving Point: %d\r\n", positionRatio);
  printMenu();
}

void positionDown()
{
  if ( positionRatio == 0 )
    return;
    
  --positionRatio;
  Serial.printf("Moving Point: %d\r\n", positionRatio);
  printMenu();
}

void positionSetup()
{
  Serial.printf("Motor moving --> %d \r\n" , positionRatio);
  setMotorMovingPosition( positionRatio);
  resetMenuState();
}

void positionHold()
{
  minValue = 9999999;
  maxValue = -9999999;
  Serial.println("Reset strain guage min,max");
  setGuageOffset();
  printValue();
  resetMenuState();
}

void positionZero()
{
  //setMotorMovingPosition( 0);
  moveToZero();
  positionRatio = 0;
  minValue = 0;
  maxValue = 0;
  //guageOffset = 0;
  resetMenuState();
}

void positionNumber(int num)
{
  positionInput[inputIndex] = (char) num;
  ++inputIndex;
}

void execCommand()
{
  
  switch(command )
  {
    case '0':
      if ( inputIndex <4 )
      {
        positionInput[inputIndex] = 0;
        positionRatio = atoi(positionInput) ;
        //ret = atoi(positionInput) ;
        Serial.print("converted to" );
        Serial.println(positionRatio);
        positionSetup( );
      } 
      resetMenuState();
      break;
    default:
      break;
  }
}

void printTouch();
void touchLoop();
void drawOLED();
void buzzerLoop();
long lastPrint = 0;
long current = 0;

char ssid[128];
char password[128];
// eatingState 1 : wifi ssid , 2: wifi password
int eatingState =0;
int eatingIndex = 0;
char *eatingBuffer=0;
void eating(byte b)
{
   Serial.printf("%c", b);
  if ( b != '\r' && b != '\n' ) {
    eatingBuffer[eatingIndex++] = b;
    return;
  }

  // input done;
 
  eatingBuffer[eatingIndex++] = 0;
  if  ( eatingBuffer == ssid )
  {
    Serial.print("Inputed SSID : " );
    Serial.println(ssid);
  }
  else if ( eatingBuffer == password )
  {
    Serial.print("Inputed Password : " );
    Serial.println(password);
  }
  
  eatingState =0;
}

void connectWiFi()
{
  std::string _ssid= ssid;
  std::string _pw = password;
  saveWIFI(_ssid, _pw);
  WiFiReSetup();
  Serial.println("Reboot for Wifi Setup in 5seconds" );
}

void loop()
{

  long val=0;
  int ret;
  ret = ads1231.read(guageValue);
  //Serial.println(val);
  motorLoop();
  if ( Serial.available() > 0 ) 
  {
    readByte = Serial.read();
    if ( eatingState == 0 ) {
    switch ( readByte ) 
    {
      case 'm':
      case 'M':
        printMenu();
        menuFlag = 1;
        command = 'm';
        break;
      case 'u':
      case 'U':
        if ( menuFlag == 0 )
         break;
        positionUp();
        command = 'u';
        break;
      case 'd':
      case 'D':
       if ( menuFlag == 0 )
         break;
        positionDown();
        command = 'd';
        break;
      case 's':
      case 'S':
       if ( menuFlag == 0 )
         break;
        positionSetup();
        command = 's';
        break;
      case 'z':
      case 'Z':
       if ( menuFlag == 0 )
         break;
        positionZero();
        command = 'z' ;
        break;
      case 'h':
      case 'H':
       if ( menuFlag == 0 )
         break;
        positionHold();
        command = 'h';
        break;
      case 'W':
      case 'w':
        Serial.println("Type your WIFI SSID ");
        eatingState =1;
        eatingBuffer = ssid;
        eatingIndex =0;
        break;
        
      case 'P':
      case 'p':
        Serial.println("Type your WIFI Password ");
        eatingState = 2;
        eatingIndex =0;
        eatingBuffer = password;
        break;

      case 'C':
      case 'c':
        connectWiFi();
        break;
        
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
       if ( menuFlag == 0 )
        return;
        positionNumber(readByte);
        Serial.print(readByte-'0');
        //++inputIndex;
        command = '0';
        break;
      case '\r':
      case '\n':
        // for flush 
        while ( Serial.available() > 0 )
          Serial.read();
        menuFlag =0;
        execCommand();
        resetMenuState();
        Serial.println("");
        break;
    } // end of switch
    }
    else
    {
      // eating state
      eating(readByte);
    }
  } //end of if
  if ( lastPrint == 0 )
    lastPrint = millis();

  current = millis();
  if ( current - lastPrint > 500 ) {
    lastPrint = current ;
    printValue();
  } 
  //printTouch();
  touchLoop();
  drawOLED();
  buzzerLoop();
}
