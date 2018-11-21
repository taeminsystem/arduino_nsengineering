// motor speed pwm control variable 
// direction low -> zero , high -> full 
// 1% , 80 pulse
#define RATIO_TO_PULSE  80

extern int positionRatio;
extern long guageOffset;
int freq = 5000; // pwm 주파수
int pwmChannel = 0;
int resolution = 8;
int dutyCount=16; // basic counter
int startMotorDuty=0;  //모터가 시작할때 듀티
int incMotorDuty= 10; //3 가속 한단계 증가 듀티
int decMotorDuty= -10; //-3 감속
int currentDuty = 0;

// motor control variable
int motorTickTarget = 0;
int cwDirectionFlag= 0;  // 1 : cw-direction , 0 : ccw-direction
int zeroDetectedFlag = 0;
int fullDetectedFlag = 0;

enum MotorControlStatus { NO_MOVING , NO_ACCEL_MOVING, INC_ACCEL_MOVING , DEC_ACCEL_MOVING } ;
int motorControlMode=NO_MOVING; // 0 : idle, 1: no acceleration, 2 : increaseing acceleration, 3: descreaing acceleration

volatile int targetPositionCounter = 0;
volatile int currentPositionCounter = 0;
volatile int movingPositionSetFlag = 0;
int motorDirection = 1;   // 1 : cw   -1 : ccw

int numberOfInterrupts = 0;
int targetMotorControlCounter = 0;
int stageMotorCounter = 0;

// increase motor speed
int decMotorStepCounter = 5 ; // 5개 모터 카운터가 들어오면 다음단계 감속
int decMotorStage = 0;
int maxDecMotorStage = 8; // 총 8단계
// decrease motor speed
int incMotorStepCounter = 5 ; // 가속
int incMotorStage= 0;
int maxIncMotorStage = 8;
void setGuageOffset();
 enum MotorStatus { HOLDED , STARTED ,MOVING_TO_ZERO,  ZERO_DETECTED, FULL_DETECTED, MOVING_TO_SET_POSITION } ;  
 int motorStatus = STARTED;

void holdBrake()
{
  digitalWrite(holdPin,1);
}
void releaseBrake()
{
  digitalWrite(holdPin, 0 );
}
void stopMotor()
{
  currentDuty = 0;
  ledcWrite( pwmChannel, currentDuty ) ;
  holdBrake();
}


void zeroPointStopHandler()
{
  currentPositionCounter = 0;
  motorStatus = ZERO_DETECTED;
  motorControlMode = NO_MOVING;
  setGuageOffset();
  Serial.print("zero stop " ) ;
  Serial.println( currentPositionCounter );
}

void zeroPointFoundHandler()
{
  //zeroDetectedFlag = 1;
  //Serial.println("Zero detected");
  //decMotor();
  stopMotor();
  zeroPointStopHandler();
}

void fullPointStopHandler()
{
  motorStatus =FULL_DETECTED;
   motorControlMode = NO_MOVING;
}
void fullPointFoundHandler()
{
  //fullDetectedFlag = 1;
  //decMotor();
  stopMotor();
  fullPointStopHandler();
  //currentPositionCounter ;
}

void setMotorMovingPosition( int ratio)
{
  int pulse = ratio * RATIO_TO_PULSE;
  if ( abs( pulse - currentPositionCounter )< 10  )
    return;
  setMovingPosition( pulse );
}

// zero-detected called.
void setMovingPosition(int pos)
{
  
  movingPositionSetFlag = 1;
 
  //currentPositionCounter = 0;
  Serial.printf("%d --> %d\r\n" , currentPositionCounter, pos ) ; 
  if ( pos < currentPositionCounter )
  {
    motorDirection = -1;
    targetPositionCounter = pos + ( maxDecMotorStage * decMotorStepCounter );
  }
  else {
    motorDirection = 1;
    targetPositionCounter = pos - ( maxDecMotorStage * decMotorStepCounter );
  }

  Serial.print("target position counter : " );
  Serial.println( targetPositionCounter ) ; 
  Serial.print("current position counter: " ) ;
  Serial.println( currentPositionCounter ) ; 
  Serial.print("Direction " );
  Serial.println( motorDirection );
  motorStatus = MOVING_TO_SET_POSITION;
  startMotor();
}

void movingPointStopHandler()
{
  motorControlMode = NO_MOVING;
  motorStatus = HOLDED;
}

void startMotor()
{
  
  currentDuty = startMotorDuty;
  //ledcWrite(pwmChannel, currentDuty);
  if ( motorDirection == 1 )
    digitalWrite(directionPin, 1 );
  else if ( motorDirection == -1 )
    digitalWrite( directionPin, 0 );
  releaseBrake();
  incMotor();
  Serial.println("*** motor start ***");
}



int incMotor()
{
   if ( motorControlMode != INC_ACCEL_MOVING )
   {
      incMotorStage = 0;
      motorControlMode = INC_ACCEL_MOVING ;
   }
   else
   {
      ++incMotorStage;
      if ( incMotorStage == (maxIncMotorStage) )
      {
        motorControlMode = NO_ACCEL_MOVING;
        targetMotorControlCounter = 0;
        return 0;
      }
   }
   targetMotorControlCounter = incMotorStepCounter;
   currentDuty += incMotorDuty;
   ledcWrite( pwmChannel, currentDuty );
   return 1;
}

int decMotor()
{
 
 // Serial.print("current duty ");
 //Serial.print(currentDuty );
  if ( motorControlMode != DEC_ACCEL_MOVING )
   {
      decMotorStage = 0;
      motorControlMode = DEC_ACCEL_MOVING ;
   }
   else
   {
      ++decMotorStage;
      if ( decMotorStage == (maxDecMotorStage) )
      {
        motorControlMode = NO_MOVING;
        targetMotorControlCounter = 0;
        //stopMotor();
        return 0;
      }
      currentDuty += decMotorDuty;
   }

  Serial.printf("duty :%d , position %d stage %d\r\n", currentDuty, currentPositionCounter,decMotorStage );
  targetMotorControlCounter = decMotorStepCounter;
  
  //if ( currentDuty <= 0 ) {
    //currentDuty = 0;
   // return 0;
  //}
  ledcWrite( pwmChannel, currentDuty );
  return 1;
}

void motorStoppedHandler()
{
  Serial.print("motor stop handler ");

  Serial.println(currentPositionCounter );
  if ( zeroDetectedFlag == 1 )
  {
    zeroDetectedFlag = 0;
    zeroPointStopHandler(); 
  }
  else if ( fullDetectedFlag == 1 )
  {
    fullDetectedFlag = 0;
    fullPointStopHandler();
  }
  else if ( movingPositionSetFlag == 1 )
  {
    movingPositionSetFlag = 0;
    movingPointStopHandler();
  }  
  //holdBrake();
  stopMotor();
  motorControlMode = NO_MOVING;
}

 void motorPulseInterruptHandler(volatile int *counter)
 {
  int ret;
 //Serial.print("*");
  if ( targetMotorControlCounter > 0 ) 
    -- targetMotorControlCounter;
  if ( targetMotorControlCounter == 0 ) 
  {
    switch( motorControlMode )
    {
      case INC_ACCEL_MOVING:
        ret = incMotor();
        break;
      case DEC_ACCEL_MOVING:
        ret = decMotor();
        if ( ret == 0 ) 
        {
          // motor stopped
          motorStoppedHandler();
        }
        break;
      default :
        break;
    }   
  }

  //if ( motorStatus == MOVING_TO_ZERO )
   // return;
  
  if ( motorDirection < 0 ) 
    --currentPositionCounter;
  else
    ++currentPositionCounter; 

    //if ( currentPositionCounter < 0 ) 
      //Serial.println(currentPositionCounter );

    if ( motorStatus == MOVING_TO_SET_POSITION )
    {
      if ( currentPositionCounter == targetPositionCounter ) 
        decMotor();
    }
 }

 void initMotor()
 {
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(pwmPin, pwmChannel);
 }

 void movingToZeroPoint()
 {
    motorDirection = -1;
    startMotor();
 }

 void moveToZero()
 {
  motorDirection = -1;
  motorStatus = MOVING_TO_ZERO;
  releaseBrake();
  startMotor();
 }

 unsigned long enterTime ;
 int prevMotorStatus =HOLDED;
 void motorLoop()
 {
    unsigned long current;
    switch ( motorStatus )
    {
      case HOLDED:
        prevMotorStatus = HOLDED;
        break;
      case STARTED:
        movingToZeroPoint();
        motorStatus = MOVING_TO_ZERO ;
        prevMotorStatus = STARTED;
        break;
      case MOVING_TO_ZERO :
        prevMotorStatus = MOVING_TO_ZERO ;
        break;
      case ZERO_DETECTED:
        if ( prevMotorStatus != ZERO_DETECTED )
          enterTime = millis();
        prevMotorStatus = ZERO_DETECTED;
        current = millis(); 
        if ( current - enterTime > (1*1000 ) )
        {
          motorStatus = HOLDED;
            
        }
        break;

      case MOVING_TO_SET_POSITION:
      //
        break;     
    }
 }
