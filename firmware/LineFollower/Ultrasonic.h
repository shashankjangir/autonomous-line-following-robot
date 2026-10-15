#pragma once
#include <Arduino.h>
#include <util/atomic.h>
#include "Timing.h"

class Ultrasonic {
  uint8_t trigPin;
  uint8_t echoPin;
  long lastDistanceCm = -1;
  unsigned long lastPingMs = 0;
  unsigned long started = 0, lastValid = 0;
  bool pending = false;
  EchoCapture capture;

public:
  Ultrasonic(uint8_t trig, uint8_t echo) : trigPin(trig), echoPin(echo) {}

  void begin() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    digitalWrite(trigPin, LOW);
    // Uno D12 = PB4/PCINT4. The sketch owns PCINT0_vect.
    PCIFR = _BV(PCIF0);
    PCMSK0 |= _BV(PCINT4);
    PCICR |= _BV(PCIE0);
  }

  // Returns true if a fresh ping was performed, false if returning cached reading
  bool update(unsigned long now, unsigned long intervalMs = 60) {
    bool complete = false;
    uint32_t width = 0;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (capture.state == 3) {
        width = capture.width; capture.state = 0; complete = true;
      }
    }
    if (complete) {
      pending = false;
      lastDistanceCm = width >= 116 && width <= 15000 ? (long)(width / 58) : -1;
      if (lastDistanceCm >= 0) lastValid = now;
    }
    if (pending && micros() - started >= 15000) {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { capture.state = 0; }
      pending = false; lastDistanceCm = -1;
    }
    if (pending || now - lastPingMs < intervalMs) return complete;
    lastPingMs = now;
    started = micros(); pending = true;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { capture.start(); }

    // Trigger pulse
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    return true;
  }

  void edge(bool high, unsigned long nowUs) { capture.edge(high, nowUs); }
  bool fresh(unsigned long now) const { return lastDistanceCm >= 0 && now - lastValid <= 200; }

  long getDistanceCm() const {
    return lastDistanceCm;
  }
};
