// Project intended for board: "NodeMcu 1.0 (ESP-12E Module)"
#include <SD.h>

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Config.h"
#include "IftttReporting.h"
#include "WifiUtil.h"
#include "LedUtil.h"
#include "DoorControl.h"
#include "WatchdogUtil.h"

WiFiUDP ntpUDP;

/**
 * NTP setup
 */
const char* poolServerName = "0.dk.pool.ntp.org";
const int UPDATE_INTERVAL_HOURS = 5;
NTPClient timeClient(ntpUDP, poolServerName);

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
  setupLEDs();
  setYellow(HIGH);
  
  Serial.begin(115200);
  delay(500);
  WiFi.mode(WIFI_OFF);  // Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA);  // Only Station No AP, This line hides the viewing of ESP as wifi hotspot
  delay(500);
  Serial.println();
  Serial.println();

  /// initialise our own watchdog to restart after N seconds, e.g. if https client hangs as we often see
  watchdog_start(90);
  
  connectWifi();
  checkWifi();
  
  setYellow(HIGH);
  timeClient.setUpdateInterval(UPDATE_INTERVAL_HOURS*60*60*1000);
  timeClient.begin();
  setYellow(LOW);

  timeClient.forceUpdate();

  // sometimes the first timestamp is wrong...?
  for (int i=0; i<2; i++)
  {
    Serial.println();
    Serial.print(F("Time is: "));
    Serial.println(timeClient.getFormattedTime());
    delay(200);  
    timeClient.update();
  }

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
   
  ifttt_webhook("Boot", ok, buf);
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
  if (minutes != minutes_previous) 
  {
    setYellow(LOW);
    minutes_previous = minutes;

    int hours = timeClient.getHours();
    
    Serial.print(F("Time is: "));
    Serial.println(timeClient.getFormattedTime());

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
          ifttt_webhook("Config", true, config.formatted());
        }
      }
      else {
        sequential_config_failures++;
        Serial.print(F("Config failed: "));
        Serial.println(getConfigError().c_str());
        if (sequential_config_failures % 10 == 0)
        {
          ifttt_webhook("Config", false, getConfigError().c_str());
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
      ifttt_webhook("Status", true, buf);
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
