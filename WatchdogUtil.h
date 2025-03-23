#ifndef _WATCHDOG_UTIL_KN_H_
#define _WATCHDOG_UTIL_KN_H_

/**
 * Homemade watchdog.
 * If watchdog reset has not been called within the <_reset_limit> seconds, then
 * the system will reset
 */

#include <Ticker.h>

Ticker ticker;

int _heartbeat_count = 0;
int _reset_limit = -1;

void heartbeat() {
  if (_heartbeat_count > _reset_limit) {
    ESP.reset();
  }
  ++_heartbeat_count;
}

void watchdog_start(int reset_limit) {
  if (reset_limit > 0) {
    _reset_limit = reset_limit;
    ticker.attach(1.0, heartbeat);
  }
}

inline void watchdog_reset() { _heartbeat_count = 0; }

#endif
