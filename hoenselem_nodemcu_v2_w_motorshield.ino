// Project intended for board: "NodeMcu 1.0 (ESP-12E Module)"
#include <ESP8266WiFi.h>
#include <SD.h>
#include <WiFiUdp.h>

#include "Config.h"
#include "DoorControl.h"
#include "LedUtil.h"
#include "TimeClient.h"
#include "WatchdogUtil.h"
#include "WebReporting.h"
#include "WifiUtil.h"
#include "ConfigUpdateTracker.h"

TimeClient timeClient;

// Configuration synced from web
Config config;

// working variables
int minutes_previous = -1;

void logBootStatus(bool ok) {
  const char compile_date[] = __DATE__ " " __TIME__;
  char buf[100];
  sprintf(buf, "%s, Door: %s, Build: %s", config.formatted(), doorStateStr(),
          compile_date);
  web_logger("Boot", ok, buf);
}

void logStatus() {
  char buf[40];
  sprintf(buf, "door: %s, config: %d min ago", doorStateStr(),
          minutesSinceConfigUpdate());
  web_logger("Status", true, buf);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("=========== Chicken door ===========");
  setupLEDs();
  setYellow(HIGH);

  delay(300);
  WiFi.mode(WIFI_OFF);  // Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA);  // Only Station No AP, This line hides the viewing of ESP as wifi hotspot
  delay(300);
  Serial.println();

  keepalive_watchdog_start(90);

  connectWifi();
  checkWifi();

  setYellow(HIGH);
  timeClient.initialize();
  setYellow(LOW);

  timeClient.printTime("No timezone set");
  doorStateInit();

  // TEST CODE TO INJECT TIME TRIGGERS
  if (false) {
    // open in 1 minute
    config.open_hour = timeClient.getHours();
    config.open_minutes = timeClient.getMinutes() + 1;
    // close 2 minutes after open
    config.close_hour = config.open_hour;
    config.close_minutes = config.open_minutes + 2;
  }

  bool ok = configReload(config);
  timeClient.setTimeOffset(config.time_offset_hours * 3600);
  logBootStatus(ok);
}

void loop() {
  keepalive();
  setYellow(HIGH);

  timeClient.update();
  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();

  if (minutes != minutes_previous) {
    minutes_previous = minutes;
    setYellow(LOW);
    timeClient.printTime();

    configReloadIfNeeded(config);

    if (config.force_open) {
      doorOpen();
    } else if (config.force_close) {
      doorClose();
    }

    if (config.open_hour == hours && config.open_minutes <= minutes) {
      doorOpen();
    } else if (config.close_hour == hours && config.close_minutes <= minutes) {
      doorClose();
    }

    // log status every X hours
    if (hours % 4 == 0 && minutes == 0) {
      logStatus();
    }
  }

  setYellow(HIGH);
  if (doorButtonPressed()) {
    Serial.println(F("Button press"));

    if (doorIsMoving()) {
      stopMove();
    } else {
      if (doorIsOpen()) {
        Serial.println(F("door-Is-Open, so close"));
        doorClose();
      } else if (doorIsClosed()) {
        Serial.println(F("door-Is-Closed, so open"));
        doorOpen();
      } else {
        // move opposite latest direction
        if (getDoorLatestDirection() > 0) {
          doorClose();
        } else {
          doorOpen();
        }
      }
    }
    delay(400);
  }

  delay(150);
}
