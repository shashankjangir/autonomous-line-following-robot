#pragma once
#include <math.h>

struct SensorModel {
  float background = 0, line = 0, ambient = 0;
  bool valid() const { return fabsf(line - background) >= 100.0f; }
  float strength(float raw, bool adaptive) {
    const float span = line - background;
    if (!valid()) return 0;
    float value = (raw - background - (adaptive ? ambient : 0)) / span;
    // Per-channel gate. Bounds limit drift but cannot guarantee correct
    // classification under arbitrary lighting or saturation.
    if (adaptive && value < 0.15f && value > -0.25f) {
      const float limit = fabsf(span) * 0.15f;
      float target = raw - background;
      if (target > limit) target = limit;
      if (target < -limit) target = -limit;
      ambient += 0.002f * (target - ambient);
    }
    return value < 0 ? 0 : (value > 1 ? 1 : value);
  }
};

struct Drive { int left, right; bool lineSeen; float error; };

struct Controller {
  float previous = 0, derivative = 0;
  unsigned long lostMs = 0;
  bool hadLine = false;
  bool timedOut = false;
  void reset() { previous = derivative = 0; lostMs = 0; hadLine = timedOut = false; }
  static int clamp(float value) {
    return (int)(value < -255 ? -255 : (value > 255 ? 255 : value));
  }
  Drive step(const float s[3], unsigned long dtMs) {
    const float sum = s[0] + s[1] + s[2];
    const bool seen = s[0] > 0.5f || s[1] > 0.5f || s[2] > 0.5f;
    if (timedOut) return {0, 0, seen, previous};
    if (!seen) {
      lostMs += dtMs;
      if (!hadLine || lostMs >= 800) {
        timedOut = true;
        return {0, 0, false, previous};
      }
      const int direction = previous >= 0 ? 1 : -1;
      return {direction * 85, -direction * 85, false, previous};
    }
    const float error = (s[2] - s[0]) / sum;
    const float dt = dtMs > 0 ? dtMs / 1000.0f : 0.01f;
    const float rate = hadLine && lostMs == 0 ? (error - previous) / dt : 0;
    derivative += 0.25f * (rate - derivative);
    if (lostMs) derivative = 0;
    previous = error;
    hadLine = true;
    lostMs = 0;
    const float base = 140 - 65 * fabsf(error);
    const float correction = 115 * error + 2.0f * derivative;
    return {clamp(base + correction), clamp(base - correction), true, error};
  }
};
