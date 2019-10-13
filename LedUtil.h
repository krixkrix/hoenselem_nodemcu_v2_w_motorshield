#ifndef LED_UTIL_H
#define LED_UTIL_H


// PINs
const int YellowLED = D5;  

void setupLEDs()
{
  pinMode(YellowLED, OUTPUT);
  digitalWrite(YellowLED, LOW);
}

void toggleYellow()
{
  digitalWrite(YellowLED, !digitalRead(YellowLED));
}

void setYellow(int on)
{
    digitalWrite(YellowLED, on);
}

#endif
