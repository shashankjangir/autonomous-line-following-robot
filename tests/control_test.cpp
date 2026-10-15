#include "../firmware/LineFollower/Control.h"
#include "../firmware/LineFollower/Timing.h"
#include <assert.h>
#include <stdio.h>
int main() {
  SensorModel sensor;
  sensor.background = 800; sensor.line = 200;
  assert(sensor.valid());
  assert(sensor.strength(800, false) == 0);
  assert(sensor.strength(200, false) == 1);
  for (int i=0; i<10000; ++i) sensor.strength(750, true);
  assert(sensor.ambient < -40 && sensor.ambient > -51);
  const float offset = sensor.ambient;
  for (int i=0; i<1000; ++i) sensor.strength(200, true);
  assert(sensor.ambient == offset); // Line must not be learned as background.
  Controller c;
  float center[3] = {0,1,0}, right[3] = {0,0,1}, left[3] = {1,0,0}, none[3] = {0,0,0};
  Drive d = c.step(center, 10); assert(d.left == d.right && d.left > 0);
  d = c.step(right, 10); assert(d.left > d.right);
  c.reset(); d = c.step(left, 10); assert(d.left < d.right);
  d = c.step(none, 10); assert(d.left < 0 && d.right > 0);
  for (int i=0; i<80; ++i) d = c.step(none, 10);
  assert(c.timedOut && d.left == 0 && d.right == 0);
  d = c.step(center, 10); assert(d.left == 0 && d.right == 0);
  c.reset(); d = c.step(none, 10); assert(c.timedOut && d.left == 0);
  c.reset(); c.step(center, 10); c.step(none, 10);
  d = c.step(center, 10); assert(!c.timedOut && d.left == d.right);
  puts("Control tests passed");
  SensorModel positive;
  positive.background = 200; positive.line = 800;
  assert(positive.strength(200, false) == 0 && positive.strength(800, false) == 1);
  for (int i=0; i<10000; ++i) positive.strength(260, true);
  assert(positive.ambient > 59 && positive.ambient <= 90);
  SensorModel invalid;
  invalid.background = 400; invalid.line = 450;
  assert(!invalid.valid() && invalid.strength(450, true) == 0);
  ControlClock clock;
  uint32_t dt = 0;
  clock.reset(100);
  assert(!clock.due(10099, dt));
  assert(clock.due(10100, dt) && dt == 10000 && clock.missed == 0);
  assert(clock.due(45100, dt) && clock.missed == 2 && clock.next == 50100);
  clock.reset(UINT32_MAX - 4999);
  assert(clock.due(5000, dt) && dt == 10000);
  EchoCapture echo;
  echo.edge(true, 1); assert(echo.state == 0);
  echo.start(); echo.edge(false, 10); assert(echo.state == 1);
  echo.edge(true, UINT32_MAX - 99); echo.edge(false, 480);
  assert(echo.state == 3 && echo.width == 580);
  puts("Scheduler, rollover and echo capture tests passed");
}
