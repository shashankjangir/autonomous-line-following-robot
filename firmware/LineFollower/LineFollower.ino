#include "Control.h"
#include "MotorDriver.h"
#include "Ultrasonic.h"
#include "ColorSensor.h"
#include "Timing.h"
#include <avr/interrupt.h>

// Roadmap benchmark profile: optional peripherals cannot change test runs.
#ifndef ENABLE_ULTRASONIC
#define ENABLE_ULTRASONIC 0
#endif
#ifndef ENABLE_COLOR
#define ENABLE_COLOR 0
#endif

// ── Hardware Configuration ─────────────────────────────────────
// Motors: TB6612FNG (PWMA, AIN1, AIN2, PWMB, BIN1, BIN2, STBY)
const uint8_t STBY_PIN = 4;
MotorDriver leftMotor(5, 7, 8);
MotorDriver rightMotor(6, 9, 10);
const int LEFT_SIGN = 1, RIGHT_SIGN = 1;

// Reflectance Sensors: Pololu QTR-3A analog channels
const bool ANALOG_SENSORS = true;
const uint8_t SENSOR_PINS[3] = {A0, A1, A2};
SensorModel sensors[3];

// User Interface: Pushbutton on D2 and Built-in LED on D13
const uint8_t BUTTON_PIN = 2;
const uint8_t MODE_PIN = 3; // Ground at boot for fixed mode; open for adaptive.

// Ultrasonic Distance Sensor: HC-SR04 (Trig on D11, Echo on D12)
const uint8_t TRIG_PIN = 11;
const uint8_t ECHO_PIN = 12;
const long OBSTACLE_DIST_CM = 15;
Ultrasonic ultrasonic(TRIG_PIN, ECHO_PIN);
ISR(PCINT0_vect) { ultrasonic.edge((PINB & _BV(PB4)) != 0, micros()); }

// Color Sensor: TCS34725 on I2C (SDA=A4, SCL=A5)
ColorSensor colorSensor;

// ── State Variables ───────────────────────────────────────────
Controller controller;
bool running = false;
bool adaptive = ANALOG_SENSORS;
bool haveBackground = false, haveLine = false;
char calibration = 0;
unsigned long calibrationStart = 0;
ControlClock controlClock;
uint32_t cycleUs = 10000;
char logBuffer[160];
uint8_t logLength = 0, logPosition = 0;
unsigned long droppedLogs = 0;
bool obstaclePaused = false;
unsigned long lastLog = 0;
unsigned long lastColorCheck = 0;
unsigned long totals[3] = {0, 0, 0};
unsigned long samples = 0;

bool buttonRaw = HIGH, buttonStable = HIGH;
unsigned long buttonChanged = 0;
ColorSensor::ColorType currentColor = ColorSensor::COLOR_UNKNOWN;

// ── Motor Helper Functions ────────────────────────────────────
void driveMotor(MotorDriver &motor, int speed) {
  motor.set(speed);
}

void stopRobot() {
  running = false;
  driveMotor(leftMotor, 0);
  driveMotor(rightMotor, 0);
}

// ── Serial Command Processor ──────────────────────────────────
void command(char c) {
  if (c != 's' && c != 'b' && c != 'l' && c != 'a' && c != 'f' && c != 'r') return;
  // Finish a partial CSV line only after stopping, before any command messages.
  stopRobot();
  if (logPosition < logLength) Serial.write((const uint8_t*)logBuffer + logPosition, logLength - logPosition);
  logLength = logPosition = 0;
  if (c == 's') { 
    stopRobot(); 
    calibration = 0; 
    Serial.println(F("# Stopped"));
  }
  if (c == 'b' || c == 'l') {
    stopRobot();
    calibration = c;
    calibrationStart = millis();
    samples = 0;
    for (int i = 0; i < 3; ++i) totals[i] = 0;
    Serial.println(c == 'b' 
      ? F("# Sampling BACKGROUND for 2 sec: hold all 3 sensors over white surface")
      : F("# Sampling LINE for 2 sec: hold all 3 sensors over black line"));
  }
  if (c == 'a' || c == 'f') {
    stopRobot();
    if (c == 'a' && !ANALOG_SENSORS) {
      Serial.println(F("# Adaptive thresholds require analog sensors"));
      return;
    }
    adaptive = (c == 'a');
    for (int i = 0; i < 3; ++i) sensors[i].ambient = 0;
    Serial.println(adaptive ? F("# Adaptive background mode enabled") : F("# Fixed threshold mode enabled"));
  }
  if (c == 'r' && !calibration) {
    bool valid = haveBackground && haveLine;
    for (int i = 0; i < 3; ++i) valid = valid && sensors[i].valid();
    if (!valid) { 
      Serial.println(F("# ERROR: Calibrate 'b' (background) and 'l' (line) first; contrast must be >=100 counts")); 
      return; 
    }
    controller.reset();
    running = true;
    Serial.println(F("# Running..."));
    Serial.flush(); // Only at explicit start, before the first motor command.
    controlClock.reset(micros());
    droppedLogs = 0;
    obstaclePaused = false;
  }
}

// ── Arduino Setup ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(MODE_PIN, INPUT_PULLUP);
  adaptive = ANALOG_SENSORS && digitalRead(MODE_PIN) == HIGH;
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(STBY_PIN, OUTPUT); 
  digitalWrite(STBY_PIN, LOW); // TB6612 in standby initially

  leftMotor.begin(); 
  rightMotor.begin();
  for (int i = 0; i < 3; ++i) pinMode(SENSOR_PINS[i], INPUT);

  if (ENABLE_ULTRASONIC) ultrasonic.begin();

  if (ENABLE_COLOR && colorSensor.begin()) {
    Serial.println(F("# TCS34725 Color Sensor: INITIALIZED OK"));
  } else {
    Serial.println(F("# Color sensor disabled or unavailable"));
  }

  stopRobot();
  digitalWrite(STBY_PIN, HIGH); // Enable motor driver

  Serial.println(F("# Commands: b=background,l=line,r=run,s=stop,a=adaptive,f=fixed"));
  Serial.println(F("ms,raw_L,raw_C,raw_R,error,seen,pwm_L,pwm_R,dist_cm,color,adaptive,dt_us,max_dt_us,missed_ticks,state,dropped_logs"));
  Serial.flush();
  controlClock.reset(micros());
}

// ── Main Loop ─────────────────────────────────────────────────
void loop() {
  // Drain only the space available in the UART buffer; never wait for TX.
  if (logPosition < logLength) {
    uint8_t count = min((int)(logLength - logPosition), Serial.availableForWrite());
    Serial.write((const uint8_t*)logBuffer + logPosition, count);
    logPosition += count;
  }
  if (Serial.available()) command(Serial.read());
  const unsigned long now = millis();
  if (ENABLE_ULTRASONIC) ultrasonic.update(now, 60);

  // ── Pushbutton Debounce & State Cycling ────────────────────
  const bool pressed = digitalRead(BUTTON_PIN);
  if (pressed != buttonRaw) { buttonRaw = pressed; buttonChanged = now; }
  if (now - buttonChanged >= 30 && buttonStable != buttonRaw) {
    buttonStable = buttonRaw;
    if (buttonStable == LOW && !calibration) {
      if (running) command('s');
      else if (!haveBackground) command('b');
      else if (!haveLine) command('l');
      else command('r');
    }
  }

  // ── Status LED Indicator ───────────────────────────────────
  // Blinks during calibration, solid ON when running, OFF when stopped
  digitalWrite(LED_BUILTIN, calibration ? ((now / 150) % 2) : (running ? HIGH : LOW));

  // ── Sensor Sampling Rate Limiter (10ms control cycle) ──────
  if (!controlClock.due(micros(), cycleUs)) return;
  const unsigned long dt = max(1UL, (unsigned long)((cycleUs + 500) / 1000));

  // Read reflectance sensors
  int raw[3];
  for (int i = 0; i < 3; ++i) {
    raw[i] = ANALOG_SENSORS ? analogRead(SENSOR_PINS[i]) : (digitalRead(SENSOR_PINS[i]) == HIGH ? 1023 : 0);
  }

  // ── Calibration Handling ───────────────────────────────────
  if (calibration) {
    for (int i = 0; i < 3; ++i) totals[i] += raw[i];
    ++samples;
    if (now - calibrationStart >= 2000) {
      for (int i = 0; i < 3; ++i) {
        float mean = (float)totals[i] / samples;
        if (!ANALOG_SENSORS && mean > 102.3f && mean < 920.7f) {
          haveBackground = haveLine = false;
          calibration = 0;
          Serial.println(F("# Unstable digital readings: adjust sensor and recalibrate"));
          return;
        }
        if (!ANALOG_SENSORS) mean = mean > 511.5f ? 1023 : 0;
        if (calibration == 'b') sensors[i].background = mean;
        else sensors[i].line = mean;
        sensors[i].ambient = 0;
      }
      if (calibration == 'b') {
        haveBackground = true;
        Serial.println(F("# Background calibrated! Next: place on black line and press button or send 'l'"));
      } else {
        haveLine = true;
        Serial.println(F("# Line calibrated! Ready: place on track and press button or send 'r'"));
      }
      calibration = 0;
    }
    return;
  }

  // ── Ultrasonic Obstacle Check (every 60ms) ──────────────────
  long dist = ultrasonic.getDistanceCm();

  // ── Color Sensor Check (every 70ms) ────────────────────────
  if (ENABLE_COLOR && colorSensor.isAvailable() && (now - lastColorCheck >= 70)) {
    lastColorCheck = now;
    currentColor = colorSensor.detectColor();
  }

  if (!running) return;

  // ── Safety Stop if Obstacle Detected ───────────────────────
  const bool blocked = ENABLE_ULTRASONIC && (!ultrasonic.fresh(now) || dist <= OBSTACLE_DIST_CM);

  // ── Red Color Marker Detection (e.g., Stop / Finish Line) ──
  bool redStop = ENABLE_COLOR && currentColor == ColorSensor::COLOR_RED;
  if (redStop) {
    // Red detected: stop robot or signal end of run
    stopRobot();
  }

  // ── Normal Line Following Control Step ─────────────────────
  float strengths[3];
  for (int i = 0; i < 3; ++i) strengths[i] = sensors[i].strength(raw[i], adaptive);

  if (obstaclePaused && !blocked) controller.reset();
  obstaclePaused = blocked;
  Drive out = (blocked || redStop) ? Drive{0, 0, false, controller.previous} : controller.step(strengths, dt);

  // If green detected, reduce speed slightly for high stability
  int speedScale = (currentColor == ColorSensor::COLOR_GREEN) ? 80 : 100;
  int leftCmd = (LEFT_SIGN * out.left * speedScale) / 100;
  int rightCmd = (RIGHT_SIGN * out.right * speedScale) / 100;

  driveMotor(leftMotor, leftCmd);
  driveMotor(rightMotor, rightCmd);

  if (controller.timedOut) { 
    stopRobot(); 
  }

  // ── Serial Telemetry (decimated to 100ms) ───────────────────
  if (now - lastLog >= 100 || controller.timedOut || redStop) {
    lastLog = now;
    if (logPosition < logLength) { ++droppedLogs; return; }
    char errorText[12]; dtostrf(out.error, 1, 3, errorText);
    const char *state = redStop ? "red_stop" : (controller.timedOut ? "lost_stop" : (blocked ? "obstacle" : "track"));
    int length = snprintf(logBuffer, sizeof(logBuffer),
      "%lu,%d,%d,%d,%s,%d,%d,%d,%ld,%d,%d,%lu,%lu,%lu,%s,%lu\n",
      now, raw[0], raw[1], raw[2], errorText, (int)out.lineSeen, leftCmd, rightCmd,
      dist, (int)currentColor, (int)adaptive, (unsigned long)cycleUs,
      (unsigned long)controlClock.maxDt, (unsigned long)controlClock.missed, state, droppedLogs);
    if (length < 0 || length >= (int)sizeof(logBuffer)) { ++droppedLogs; return; }
    logLength = (uint8_t)length; logPosition = 0;
  }
}
