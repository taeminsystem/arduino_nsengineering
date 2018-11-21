#define TOUCH_NUM 4
#define THRESHOLD 50
#define TOUCH_BLOCK_TIME 1500
#define TOUCH_FIRST_TIMEOUT 300
#define BUZZER_ON_TIME  300

long touchFirst[TOUCH_NUM];
long touchTimeout[TOUCH_NUM];
int touchCnt[TOUCH_NUM];
long touchPin[TOUCH_NUM] = {13,12,14,27};
int drawMode = 0;
int buzzerTimeout = 0;
int buzzerOnFlag = 0;

void buzzerOn()
{
    buzzerOnFlag =1 ;
    buzzerTimeout = millis() + BUZZER_ON_TIME;
    digitalWrite(buzzerPin, 1 );
}

void buzzerOff()
{
  buzzerOnFlag = 0;
  digitalWrite(buzzerPin, 0 );
}

void printTouch()
{
  char buf[32];
  int ret = touchRead(touch4Pin);
  Serial.print("Touch ");
  Serial.println(ret);
  sprintf(buf, "touch1 : %d" , ret );
  oled.clear();
  oled.drawString(0,0,buf);
  oled.display();
}

void touchHandler(int i )
{
  switch(i)
  {
    case 0:
      // mode handler
      drawMode = ++drawMode % 4;
      break;
    case 1:
      // down handler
      positionDown();
      positionSetup();
      break;
    case 2:
      // up handler
      positionUp();
      positionSetup();
      break;
    case 3:
      // hold handler
      positionHold();
      break;  
  }
  
}

void buzzerLoop()
{
  long cur = millis();
  if ( buzzerOnFlag == 1 )
  {
    if ( cur > buzzerTimeout )
    {
      buzzerOff();
    }
  }
}

void doTouch(int i , long cur)
{
  touchHandler(i);
  //Serial.print("Touch :");
  //Serial.println(ret);
  touchTimeout[i] = cur + TOUCH_BLOCK_TIME ;
  buzzerOn();
  touchCnt[i] = 0;
}

void touchLoop()
{
  int ret;
  long cur = millis();
  for ( int i = 0 ; i < TOUCH_NUM ; i ++ )
  {
    ret = touchRead(touchPin[i]);
    if ( ret > 0 && ret < THRESHOLD  && touchTimeout[i] < cur)
    {
      if ( touchFirst[i] == 0 ) 
      {
        //Serial.println("First touch");
        // first touch
        touchFirst[i] = cur;
        ++touchCnt[i];
      }
      else
      {
          if ( touchFirst[i] + TOUCH_FIRST_TIMEOUT < cur )
          {
            // noise touch
            //Serial.println("Noise touch");
            touchFirst[i] = 0;
            touchCnt[i] = 0;
          }
          else
          {
            //consecutive touch 
            //Serial.println("Consecutive touch");
            touchCnt[i]++;
            touchFirst[i] = cur + TOUCH_FIRST_TIMEOUT;
            if ( touchCnt[i] > 2 )
              doTouch(i,cur);
          }
      }
    }
    else 
    {
      // not touched
      if ( touchFirst[i] != 0 && touchFirst[i] + TOUCH_FIRST_TIMEOUT > cur )
      {
        //Serial.print("Cancel touch ");
        //Serial.println(ret);
        touchFirst[i] = 0;
        touchCnt[i] = 0;
      }
    }
    
  } // end of for  
}

void drawOLED()
{
  char buf[32];
  oled.clear();
  switch(drawMode)
  {
      case 0:
        // load mode
        oled.drawString(0,0," Load" );
        sprintf(buf, "%ld" ,guageValue - guageOffset );
        oled.drawString(0,25, buf );
        break;
      case 1:
        // max
        oled.drawString(0,0," Max" );
        sprintf(buf, " %ld" , maxValue - guageOffset);
        oled.drawString(0,25,buf);
        break;
      case 2:
        oled.drawString(0,0," Min" );
        sprintf(buf, " %ld" , minValue - guageOffset);
        oled.drawString(0,25,buf);
        break;
      case 3:
        oled.drawString(0,0," Motor" );
        sprintf(buf, "   %d" , positionRatio);
        oled.drawString(0,25,buf);
        break;
  }
  oled.display();
}

