// Project intended for board: "NodeMcu 1.0 (ESP-12E Module)"
#include <SD.h>

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Config.h"
#include "WebReporting.h"
#include "WifiUtil.h"
#include "LedUtil.h"
#include "DoorControl.h"
#include "WatchdogUtil.h"
#include "TimeClient.h"

TimeClient timeClient;

// Configuration synced from web
Config config;
Config configTmp;

// working variables
int minutes_previous = -1;
unsigned long unix_latest_config_update = 0;
int sequential_config_failures = 0;

const char compile_date[] = __DATE__ " " __TIME__;

void setup()
{
  Serial.begin(115200);
  Serial.println("=========== Chicken door ===========");
  setupLEDs();
  setYellow(HIGH);
  
  delay(500);
  WiFi.mode(WIFI_OFF);  // Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA);  // Only Station No AP, This line hides the viewing of ESP as wifi hotspot
  delay(500);
  Serial.println();
  Serial.println();

  /// initialise watchdog to restart the device after N seconds, e.g. if https client hangs as we often see
  watchdog_start(90);
  
  connectWifi();
  checkWifi();
  
  setYellow(HIGH);
  timeClient.initialize();
  setYellow(LOW);

  timeClient.printTime();
  doorStateInit();


  // TEST CODE TO INJECT TIME TRIGGERS
  /*
  config.open_hour = timeClient.getHours();
  config.close_hour = config.open_hour;
  config.open_minutes = timeClient.getMinutes() + 1;
  config.close_minutes = config.open_minutes + 2;
  */

  bool ok = getGoogleConfig(config);
  timeClient.setTimeOffset(config.time_offset_hours*3600);

  char buf[100];
  sprintf(buf, "%s, Door: %s, Build: %s", config.formatted(), doorStateStr(), compile_date);
   
  web_logger("Boot", ok, buf);
  if (ok) 
  {
    unix_latest_config_update = timeClient.getEpochTime();
  }
}

unsigned long minutesSinceConfigUpdate() 
{
  return (timeClient.getEpochTime() - unix_latest_config_update)/60;
}

bool configIsTooOld() 
{
  return minutesSinceConfigUpdate() > 60;
}

void loop() 
{
  watchdog_reset();
  setYellow(HIGH);

  timeClient.update();
  int minutes = timeClient.getMinutes();
  int hours = timeClient.getHours();

  if (minutes != minutes_previous)
  {
    setYellow(LOW);
    minutes_previous = minutes;

    timeClient.printTime();

    // update config with configured interval
    if (config.poll_interval_minutes > 0 && (minutes % config.poll_interval_minutes == 0 || configIsTooOld()))
    {
      // get config
      if (getGoogleConfig(configTmp))
      {
        unix_latest_config_update = timeClient.getEpochTime();
        sequential_config_failures = 0;
        if (!config.equals(configTmp))
        {
          config = configTmp;
          timeClient.setTimeOffset(config.time_offset_hours*3600);
          web_logger("Config", true, config.formatted());
        }
      }
      else {
        sequential_config_failures++;
        Serial.print(F("Config failed: "));
        Serial.println(getConfigError().c_str());
        if (sequential_config_failures % 10 == 0)
        {
          web_logger("Config", false, getConfigError().c_str());
        }
      }
    }

    if (config.force_open) 
    {
      doorOpen();
    }
    else if (config.force_close) 
    {
      doorClose();
    }
    
    if (config.open_hour == hours && config.open_minutes <= minutes) 
    {
      doorOpen();
    }
    else if (config.close_hour == hours && config.close_minutes <= minutes) 
    {
      doorClose();
    }

    // log status every X hours
    if (hours%4 == 0 && minutes == 0)
    {
      char buf[40];
      sprintf(buf, "door: %s, config: %d min ago", doorStateStr(), minutesSinceConfigUpdate());
      web_logger("Status", true, buf);
    }
  }

  setYellow(HIGH);
  if (doorButtonPressed())
  {
    Serial.println(F("Button press"));

    if (doorIsMoving())
    {
      stopMove();
    }
    else 
    {
      if (doorIsOpen())
      {
        Serial.println(F("door-Is-Open, so close"));
        doorClose();
      }
      else if (doorIsClosed())
      {
        Serial.println(F("door-Is-Closed, so open"));
        doorOpen();
      }
      else 
      {
        // move opposite latest direction
        if (getDoorLatestDirection() > 0) 
        {
          doorClose();
        }
        else
        {
          doorOpen();
        }
      }
    }
    delay(400);
  }

  delay(150);
}
