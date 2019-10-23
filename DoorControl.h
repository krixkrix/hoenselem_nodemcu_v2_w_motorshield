#ifndef DOOR_CONTROL_H
#define DOOR_CONTROL_H

#include "WatchdogUtil.h"

// PINs motor
const int MotorADirPin = D3;
const int MotorASpeedPin = D1;
const int Speed = 1023;  // MaxSpeed 1023 is around 4sec/rev

// PINs motor inputs
const int LimitBotPin = D6;
const int LimitTopPin = D7;
const int ButtonPin = D8;

// door states

bool isMoving = false;
int latestDirection = -1;  // set latest direction down, so the first button-initiated move will be up

const int DoorTimeoutMs = 60000;

// door input resolution (limit switches, buttons, LEDs...)
const int LoopSleep = 5;

void startMove(int direction) 
{
  digitalWrite(MotorADirPin, direction == 1);
  analogWrite(MotorASpeedPin, Speed);
  isMoving = true;
  latestDirection = direction;
}

void stopMove()
{
  digitalWrite(MotorASpeedPin, LOW);
  isMoving = false;
}

bool doorIsMoving() 
{
  return isMoving;
}

int getDoorLatestDirection(){
  return latestDirection;
}

bool doorIsClosed()
{
  return digitalRead(LimitBotPin) == 1;
}
bool doorIsOpen()
{
  return digitalRead(LimitTopPin) == 1;
}


void doorStateInit() 
{
  // initial settings for motors off and direction forward
  pinMode(MotorASpeedPin, OUTPUT);
  pinMode(MotorADirPin, OUTPUT);
 
  digitalWrite(MotorASpeedPin, LOW);
  digitalWrite(MotorADirPin, HIGH);
  
  latestDirection = 1;
  stopMove();
}

bool doorButtonPressed()
{
  return digitalRead(ButtonPin) == 1;
}

bool doorOpen() 
{
  if (doorIsOpen())
  {
    Serial.println(F("already open"));
    return true;
  }
  Serial.println(F("opening door"));
  startMove(1);
  for (int dt = 0; dt < DoorTimeoutMs; dt+=LoopSleep) 
  {
    if (dt%500 == 0) 
    {
      Serial.print("o"); 
      toggleYellow();
    }
    if (doorIsOpen())
    {
      delay(10);  // hesitate to ensure switch is properly engaged
      break;
    }
    if (doorButtonPressed() && dt > 500) 
    {
      // cancel the move
      Serial.println(F("Cancel open"));
      break;
    }
    delay(LoopSleep);
    watchdog_reset();
  }
  stopMove();
  setYellow(LOW);

  if (!doorIsOpen())
  {
    report_door_open(false, F("ERROR: open switch NOT active"));
    return false;
  }
  if (doorIsClosed())
  {
    report_door_open(false, F("ERROR: closed switch active"));
    return false;
  }
  
  report_door_open(true, F("Door now open"));
  delay(500);
  return true;
}

bool doorClose() 
{
  if (doorIsClosed())
  {
    Serial.println(F("already closed"));
    return true;
  }
  
  Serial.println(F("closing door"));
  startMove(-1);
  for (int dt = 0; dt < DoorTimeoutMs; dt+=LoopSleep) 
  {
    if (dt%500 == 0) 
    {
      Serial.print("c");
      toggleYellow();
    }
    if (doorIsClosed())
    {
      delay(10);  // hesitate to ensure switch is properly engaged
      break;
    }
    if (doorButtonPressed() && dt > 500)
    {
      // cancel the move
      Serial.println(F("Cancel close"));
      break;
    }
    delay(LoopSleep);
    watchdog_reset();
  }
  stopMove();
  setYellow(LOW);

  if (!doorIsClosed())
  {
    report_door_closed(false, F("ERROR: closed switch NOT active"));
    return false;
  }
  if (doorIsOpen())
  {
    report_door_closed(false, F("ERROR: open switch active"));
    return false;
  }
  
  report_door_closed(true, F("Door now closed"));
  delay(500);
  return true;
}

const char* doorStateStr()
{
  if (doorIsClosed())
  {
    return "closed";
  }
  else if (doorIsOpen()) 
  {
    return "open";
  }
  else {
    return "unknown";
  }
}

#endif
