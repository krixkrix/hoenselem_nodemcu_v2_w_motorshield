#ifndef _WATCHDOG_UTIL_KN_H_
#define _WATCHDOG_UTIL_KN_H_

/**
 * Homemade watchdog.
 * If watchdog reset has not been called within the <_seconds_stalled_limit> seconds, then
 * the system will reset
 * I have seen that the https client could hangs and stall the device. 
 * This prevent the device from hanging indefinitely.
 */

#include <Ticker.h>

Ticker ticker;

int _seconds_since_keepalive = 0;
int _seconds_stalled_limit = -1;

void timer_callback() {
  if (_seconds_since_keepalive > _seconds_stalled_limit) {
    // we have not received a keepalive signal for too long, so reset
    ESP.reset();
  }
  ++_seconds_since_keepalive;
}

void keepalive_watchdog_start(int device_stalled_threshold) {
  if (device_stalled_threshold > 0) {
    _seconds_stalled_limit = device_stalled_threshold;
    ticker.attach(1.0, timer_callback);
  }
}

inline void keepalive() { 
  // should be called by application code to signal that the device is still alive
  _seconds_since_keepalive = 0; 
}

#endif
