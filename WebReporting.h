#ifndef WEB_REPORTING_H
#define WEB_REPORTING_H

#include "HttpsDataManager.h"
#include "WifiUtil.h"

// declare new maker event with (maker key, event name)
HttpsDataManager event("bl6Mm_2AoLXuaTuRyFJlrR");

bool web_reporting = true;

void web_logger(const char* eventname, bool success, const char* msg)
{
  Serial.print(eventname);
  Serial.print(": ");
  Serial.print(success ? F("ok") : F("fail"));
  Serial.print(": ");
  Serial.println(msg);

  if (!web_reporting)
  {
    Serial.println(F("Skip www reporting"));
    return;
  }

  // log message
  event.setValue(1, eventname);
  event.setValue(2, success ? F("ok") : F("fail"));
  event.setValue(3, msg);
  if (event.connect())
  {
    event.post("chickendoor");
  }
  else 
  {
    Serial.println(F("Failed to post to web"));
  }

  // special notification when error
  if (!success)
  {
    if (event.connect())
    {
      event.post("chickendoor_error");
    }
    else 
    {
      Serial.println(F("Failed to post the data"));
    }
  }
}

void report_door_closed(bool ok, const char* msg)
{
  web_logger("Door close", ok, msg);
}

void report_door_open(bool ok, const char* msg)
{
  web_logger("Door open", ok, msg);
}

#endif  // WEB_REPORTING_H
