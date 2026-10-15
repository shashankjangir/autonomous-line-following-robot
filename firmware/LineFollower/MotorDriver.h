#pragma once
#include <Arduino.h>

// TB6612FNG: one PWM pin and two direction pins per channel; shared STBY.
class MotorDriver {
  uint8_t pwm, in1, in2;
public:
  MotorDriver(uint8_t p, uint8_t a, uint8_t b) : pwm(p), in1(a), in2(b) {}
  void begin() {
    pinMode(pwm, OUTPUT); pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
    set(0);
  }
  void set(int speed) {
    speed = constrain(speed, -255, 255);
    analogWrite(pwm, 0);
    digitalWrite(in1, speed > 0 ? HIGH : LOW);
    digitalWrite(in2, speed < 0 ? HIGH : LOW);
    // Both direction pins LOW and PWM HIGH select high-impedance stop.
    analogWrite(pwm, speed == 0 ? 255 : abs(speed));
  }
};
