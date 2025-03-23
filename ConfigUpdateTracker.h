#ifndef CONFIG_UPDATE_TRACKER_H
#define CONFIG_UPDATE_TRACKER_H

#include <ESP8266WiFi.h>
#include "TimeClient.h"
#include "Config.h"
#include "WebReporting.h"

extern TimeClient timeClient;

Config configTmp;
unsigned long unix_latest_config_update;
int sequential_config_failures;

unsigned long minutesSinceConfigUpdate() {
  return (timeClient.getEpochTime() - unix_latest_config_update) / 60;
}

bool configNeedsReload(Config& config) {
  int minutes = timeClient.getMinutes();
  bool configIsTooOld = minutesSinceConfigUpdate() > 60; 
  return (config.poll_interval_minutes > 0 &&
          (minutes % config.poll_interval_minutes == 0 || configIsTooOld));
}

bool configReload(Config& config) {
  // read config from web
  if (getGoogleConfig(configTmp)) {
    unix_latest_config_update = timeClient.getEpochTime();
    sequential_config_failures = 0;
    if (!config.equals(configTmp)) {
      config = configTmp;
      timeClient.setTimeOffset(config.time_offset_hours * 3600);
      web_logger("Config", true, config.formatted());
    }
    return true;
  } else {
    // count failures and report if failing repeatedly
    sequential_config_failures++;
    Serial.print(F("Config failed: "));
    Serial.println(getConfigError().c_str());
    if (sequential_config_failures % 10 == 0) {
      web_logger("Config", false, getConfigError().c_str());
    }
    return false;
  }
}

bool configReloadIfNeeded(Config& config) {
  if (configNeedsReload(config)) {
    return configReload(config);
  }
  return true;
}

#endif // CONFIG_UPDATE_TRACKER_H