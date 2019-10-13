#ifndef IFTTT_REPORTING_H
#define IFTTT_REPORTING_H

#include "DataMaker.h"
#include "WifiUtil.h"

// curl test expression
// curl --header "Content-Type: application/json" --request POST --data '{"value1":"xyz","value2":"another value"}' http://maker.ifttt.com/trigger/chickendoor/with/key/bl6Mm_2AoLXuaTuRyFJlrR

// declare new maker event with (maker key, event name)
DataToMaker event("bl6Mm_2AoLXuaTuRyFJlrR");

void ifttt_webhook(const char* eventname, bool success, const char* msg)
{
  Serial.print(eventname);
  Serial.print(": ");
  Serial.print(success ? F("ok") : F("fail"));
  Serial.print(": ");
  Serial.println(msg);

  connectWifi();

  // log message
  event.setValue(1, eventname);
  event.setValue(2, success ? F("ok") : F("fail"));
  event.setValue(3, msg);
  if (event.connect())
  {
    event.post(F("chickendoor"));
  }
  else 
  {
    Serial.println(F("Failed to connect IFTTTr"));
  }

  // special notification when error
  if (!success)
  {
    if (event.connect())
    {
      event.post(F("chickendoor_error"));
    }
    else 
    {
      Serial.println(F("Failed to connect IFTTT2"));
    }
  }
}

void report_door_closed(bool ok, const char* msg)
{
  ifttt_webhook(F("Door close"), ok, msg);
}

void report_door_open(bool ok, const char* msg)
{
  ifttt_webhook(F("Door open"), ok, msg);
}

#endif
