#pragma once
#include <Arduino.h>
#include <Wire.h>

// Lightweight standalone driver for TCS34725 RGB Color Sensor
// Uses standard Wire.h with zero external library dependencies.
class ColorSensor {
  static const uint8_t ADDR = 0x29;
  static const uint8_t CMD_BIT = 0x80;
  static const uint8_t CMD_AUTO_INC = 0xA0;

  bool initialized = false;

  void writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ADDR);
    Wire.write(CMD_BIT | reg);
    Wire.write(val);
    Wire.endTransmission();
  }

  uint8_t readReg(uint8_t reg) {
    Wire.beginTransmission(ADDR);
    Wire.write(CMD_BIT | reg);
    if (Wire.endTransmission(false) != 0) return 0;
    Wire.requestFrom(ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0;
  }

public:
  enum ColorType { COLOR_UNKNOWN, COLOR_BLACK, COLOR_WHITE, COLOR_RED, COLOR_GREEN };

  bool begin() {
    Wire.begin();
    Wire.setWireTimeout(1500, true);
    // Check device ID (TCS34725 returns 0x44 or 0x4D)
    uint8_t id = readReg(0x12);
    if (id != 0x44 && id != 0x4D) {
      initialized = false;
      return false;
    }
    // Set Integration time to 50ms (0xEB)
    writeReg(0x01, 0xEB);
    // Set Gain to 4x (0x01)
    writeReg(0x0F, 0x01);
    // Oscillator startup occurs only in setup with motors stopped.
    writeReg(0x00, 0x01);
    delay(3);
    writeReg(0x00, 0x03);
    initialized = true;
    return true;
  }

  bool isAvailable() const { return initialized; }

  bool readRaw(uint16_t &r, uint16_t &g, uint16_t &b, uint16_t &c) {
    if (!initialized) return false;
    Wire.beginTransmission(ADDR);
    Wire.write(CMD_AUTO_INC | 0x14); // 0x14 is CDATAL
    if (Wire.endTransmission(false) != 0) return false;

    if (Wire.requestFrom(ADDR, (uint8_t)8) != 8) return false;
    uint16_t values[4];
    for (uint8_t i = 0; i < 4; ++i) {
      uint8_t lo = Wire.read();
      uint8_t hi = Wire.read();
      values[i] = (uint16_t)lo | ((uint16_t)hi << 8);
    }
    c = values[0]; r = values[1]; g = values[2]; b = values[3];
    return true;
  }

  ColorType detectColor() {
    uint16_t r, g, b, c;
    if (!readRaw(r, g, b, c)) return COLOR_UNKNOWN;

    if (c < 400) return COLOR_BLACK;
    // Provisional thresholds: calibrate against actual colored markers.
    if (r > 1.4f * g && r > 1.4f * b) return COLOR_RED;
    if (g > 1.3f * r && g > 1.3f * b) return COLOR_GREEN;
    if (c > 1200) return COLOR_WHITE;
    return COLOR_UNKNOWN;
  }
};
