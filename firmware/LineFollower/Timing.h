#pragma once
#include <stdint.h>
struct ControlClock {
  uint32_t next = 0, previous = 0, missed = 0, maxDt = 0;
  void reset(uint32_t now) { previous = now; next = now + 10000; missed = maxDt = 0; }
  bool due(uint32_t now, uint32_t &dt) {
    if ((int32_t)(now - next) < 0) return false;
    uint32_t skipped = (now - next) / 10000;
    missed += skipped; next += (skipped + 1) * 10000;
    dt = now - previous; previous = now;
    if (dt > maxDt) maxDt = dt;
    return true;
  }
};
// Read snapshots atomically outside the interrupt handler.
struct EchoCapture {
  volatile uint8_t state = 0;
  volatile uint32_t rise = 0, width = 0;
  void start() { state = 1; width = 0; }
  void edge(bool high, uint32_t now) {
    if (state == 1 && high) { rise = now; state = 2; }
    else if (state == 2 && !high) { width = now - rise; state = 3; }
  }
};
