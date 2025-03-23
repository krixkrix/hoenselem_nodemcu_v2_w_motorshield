#include <dummy.h>

#ifndef GOOGLE_CONFIG_H
#define GOOGLE_CONFIG_H

#include <ESP8266HTTPClient.h>

#include "WifiUtil.h"

// config is stored in a public google spreadsheet
// note the URL contains the doc ID , the modifier "format=csv", and a specific
// cell range

const char* host = "docs.google.com";
const int httpsPort = 443;
const char* link =
    "/spreadsheets/d/1mWT1SBtN5EKl85kzLBuUofBARWZpKznMYA6NtNMP_4Q/"
    "export?gid=0&format=csv&range=A3:B10";  // The RANGE here is crucial
char* latestError = "none";

/* Testing with curl (remember the quotes):
 *  curl -L
 * "https://docs.google.com/spreadsheets/d/1mWT1SBtN5EKl85kzLBuUofBARWZpKznMYA6NtNMP_4Q/export?gid=0&format=csv&range=A3:B10"
 *
 *  Consider if using google web app would be better
 *  A proof of concept is executed on this URL
 *  curl -L
 * https://script.google.com/macros/s/AKfycbzVvEIAu9txKl-rJPk7JmCums9h9JS-RLfYtAD0MIlFRbKORjs/exec
 */

HTTPClient https;
BearSSL::WiFiClientSecure newSecure;

static char cbuf[50];

class Config {
 public:
  int open_hour = 6;
  int open_minutes = 55;
  int close_hour = 22;
  int close_minutes = 40;
  int time_offset_hours = 2;
  int poll_interval_minutes = 10;
  int force_open = 0;
  int force_close = 0;

  const char* formatted() {
    sprintf(cbuf, "Open: %02d:%02d, Close: %02d:%02d, TZ: %+dh, Poll: %d",
            open_hour, open_minutes, close_hour, close_minutes,
            time_offset_hours, poll_interval_minutes);
    return cbuf;
  }

  void print() { Serial.println(formatted()); }

  bool equals(const Config& other) {
    return open_hour == other.open_hour && open_minutes == other.open_minutes &&
           close_hour == other.close_hour &&
           close_minutes == other.close_minutes &&
           poll_interval_minutes == other.poll_interval_minutes &&
           force_open == other.force_open && force_close == other.force_close &&
           time_offset_hours == other.time_offset_hours;
  }
};

const String& getConfigError() { return latestError; }

bool getGoogleConfig(Config& config) {
  checkWifi();
  latestError = "";

  // do not use fingerprint since it will change over time
  Serial.println(F("HTTPS connect"));
  newSecure.setInsecure();
  https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  https.begin(newSecure, host, httpsPort, link, true);

  int code = https.GET();
  if (code != 200) {
    https.end();
    newSecure.stop();
    Serial.print(F("Https failed, code: "));
    Serial.println(code);
    latestError = "Connect failure";
    return false;
  }

  // example reply
  /*
  open_hour,23
  open_minutes,59
  close_hour,12
  close_minutes,13
  ...
  */
  String payload = https.getString();
  Serial.println(payload);

  https.end();
  newSecure.stop();

  int n = sscanf(payload.c_str(),
                 "open_hour,%d open_minutes,%d close_hour,%d close_minutes,%d "
                 "time_offset_hours,%d poll_interval_minutes,%d force_open,%d "
                 "force_close,%d",
                 &config.open_hour, &config.open_minutes, &config.close_hour,
                 &config.close_minutes, &config.time_offset_hours,
                 &config.poll_interval_minutes, &config.force_open,
                 &config.force_close);

  Serial.print(F("Got params: "));
  Serial.println(n);

  if (n != 8) {
    latestError = "Parse failure";
    return false;
  }

  // don't accept any strange data
  if (config.poll_interval_minutes < 0 || config.open_hour < 1 ||
      config.open_hour > 24 || config.close_hour < 1 ||
      config.close_hour > 24 || config.open_minutes < 0 ||
      config.open_minutes > 59 || config.close_minutes < 0 ||
      config.close_minutes > 59 || config.time_offset_hours > 23 ||
      config.time_offset_hours < -23) {
    latestError = "Bad content";
    return false;
  }

  return true;  // success
}

#endif
