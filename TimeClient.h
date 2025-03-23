#ifndef TIMECLIENT_H
#define TIMECLIENT_H

#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;

/**
 * NTP setup
 */
const char* poolServerName = "0.dk.pool.ntp.org";
const int UPDATE_INTERVAL_HOURS = 5;

class TimeClient : public NTPClient {
 public:
  TimeClient() : NTPClient(ntpUDP, poolServerName) {}

  void initialize() {
    setUpdateInterval(UPDATE_INTERVAL_HOURS * 60 * 60 * 1000);
    begin();

    // sometimes the first timestamp is wrong...? Update after a small delay
    for (int i = 0; i < 1; i++) {
      delay(100);
      update();
    }
  }

  void printTime() {
    Serial.print(F("Time: "));
    Serial.println(getFormattedTime());
  }

  void printTime(const char* msg) {
    Serial.print(F("Time: "));
    Serial.print(getFormattedTime());
    Serial.print(F(" "));
    Serial.println(msg);
  }
};

#endif  // TIMECLIENT_H
