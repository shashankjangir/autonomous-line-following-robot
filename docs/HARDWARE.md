# Hardware Configuration & Wiring Guide

This document describes the complete wiring and power distribution for the autonomous line follower robot with obstacle detection and RGB color recognition.

The roadmap benchmark uses only the line sensors. HC-SR04 and TCS34725 are optional and disabled by default (ENABLE_ULTRASONIC / ENABLE_COLOR). Physical wiring and behavior have not been verified. Follow the phase gates in ROADMAP.md.

## Bill of Materials

| Component | Part / Specification | Purpose |
|---|---|---|
| **Microcontroller** | Arduino Uno R3 (ATmega328P, 5V) | System controller |
| **Motor Driver** | Pololu TB6612FNG Dual H-Bridge Carrier | Motor speed and direction control |
| **Motors** | 2x Micro Metal Gearmotors (100:1, 6V LP) | Differential drive wheels |
| **Reflectance Array** | Pololu QTR-3A (3 Analog Channels) | Line tracking (Left, Center, Right) |
| **Obstacle Sensor** | HC-SR04 Ultrasonic Distance Sensor | Forward collision warning & avoidance |
| **Color Sensor** | TCS34725 RGB Color Sensor with IR filter (I2C) | Surface marker & color line recognition |
| **Pushbutton** | Momentary Push Button | Hands-free calibration & Run/Stop trigger |
| **Status Indicator** | Builtin LED (D13) | Blinking for calibration, solid when running |
| **Power Source** | 4x NiMH AA Rechargeable Battery Pack (~4.8V - 6V) | Motor & logic power source |
| **5V Step-Up/Step-Down Regulator** | Pololu S7V8F5 or equivalent 5V buck-boost | Clean 5V power supply to Uno |

---

## Signal Pinout & Wiring Table

| Arduino Uno Pin | Peripheral | Device Pin / Function | Notes |
|---|---|---|---|
| **A0** | QTR-3A Sensor | Left Sensor Analog Out | Read reflectance level (0-1023) |
| **A1** | QTR-3A Sensor | Center Sensor Analog Out | Read reflectance level (0-1023) |
| **A2** | QTR-3A Sensor | Right Sensor Analog Out | Read reflectance level (0-1023) |
| **A4** | TCS34725 | SDA (I2C Data) | Hardware I2C line |
| **A5** | TCS34725 | SCL (I2C Clock) | Hardware I2C line |
| **D2** | Pushbutton | Button Pin (Active LOW) | Connected to GND; internal pull-up enabled |
| **D3** | Mode jumper | GND=fixed, open=adaptive at reset | Internal pull-up; for untethered A/B trials |
| **D4** | TB6612FNG | STBY (Standby) | HIGH to enable driver, LOW for sleep |
| **D5** | TB6612FNG | PWMA | Left motor PWM speed control (Timer0) |
| **D6** | TB6612FNG | PWMB | Right motor PWM speed control (Timer0) |
| **D7** | TB6612FNG | AIN1 | Left motor direction input 1 |
| **D8** | TB6612FNG | AIN2 | Left motor direction input 2 |
| **D9** | TB6612FNG | BIN1 | Right motor direction input 1 |
| **D10** | TB6612FNG | BIN2 | Right motor direction input 2 |
| **D11** | HC-SR04 | TRIG | Ultrasonic pulse trigger output |
| **D12** | HC-SR04 | ECHO | Ultrasonic pulse echo input |
| **D13** | Onboard LED | Builtin LED Indicator | Blinks during sampling, solid while running |
| **5V** | Sensors & Logic | VCC (QTR-3A, HC-SR04, TB6612 VCC) | Color breakout VIN only if explicitly 5V-compatible |
| **GND** | System Ground | GND (All modules + Battery ground) | **Mandatory Common Ground** |

---

## Motor Connections (TB6612FNG)

* **Left Motor**: Connect motor leads to `AO1` and `AO2`.
* **Right Motor**: Connect motor leads to `BO1` and `BO2`.
* *Direction check*: If either motor spins backward, swap its motor leads on AO1/AO2 (or BO1/BO2), or invert `LEFT_SIGN` / `RIGHT_SIGN` in [firmware/LineFollower/LineFollower.ino](../firmware/LineFollower/LineFollower.ino).

---

## Power Distribution & Wiring Safety

1. **Motor Power (VM)**:
   * Connect Battery Positive (`+` 4.8V-6V) through a power switch and a 1A-2A fuse directly to `VM` on the TB6612FNG.
   * Add a 100µF - 470µF electrolytic capacitor between `VM` and `GND` right at the driver terminals to absorb motor inductive spikes.
2. **Logic Power (Uno & Sensors)**:
   * Route battery power through the 5V regulator (e.g. S7V8F5) into the Uno's USB-B power socket with a correctly wired lead. Verify polarity/5V first.
   * The Uno's 5V pin powers TB6612 `VCC`, QTR-3A `VCC`, and HC-SR04 `VCC`. Only use a TCS34725 breakout explicitly supporting 5V input and Uno-compatible I2C levels; the bare sensor is not a 5V module.
3. **NEVER**:
   * Never connect motor `VM` power to the Arduino 5V or 3.3V pin.
   * Unplug the regulator-powered USB lead before plugging in the computer USB cable. Do not use a Y cable joining both power sources. Switching off a supply alone does not guarantee isolation against back-feed.
4. **COMMON GROUND**:
   * All GND terminals (Battery negative, Uno GND, TB6612 GND, Sensor GNDs) must be tied to a single shared Ground bus.

---

## Step-by-Step Bring-Up Order

1. **Continuity & Power Check (No motors attached)**:
   * Check with a multimeter: verify 5V on logic pins, verify ~5V on battery VM line, verify 0V between all grounds.
2. **Firmware Upload**:
   * Connect Arduino Uno via USB to PC.
   * Open firmware/LineFollower/LineFollower.ino and upload using the Uno board profile and actual port. Serial Monitor uses 115200 baud.
   * The baseline profile reports the color sensor disabled/unavailable. Test color initialization only after enabling that optional feature.
3. **Sensor Verification**:
   * Wave your hand in front of HC-SR04: verify telemetry shows accurate distance in `dist_cm`.
   * Place colored paper under TCS34725: verify `color` reports RED, GREEN, or WHITE.
   * Check QTR-3A readings on white vs. black tape.
4. **Bench Motor Test (Wheels off ground)**:
   * Turn on motor power switch.
   * Press D2 button to calibrate background (2 sec), then line (2 sec), then start.
   * Verify wheels rotate forward when line is centered, and differential steering responds when the line is moved left or right.
5. **Track Trial**:
   * Place robot on course and test autonomous driving.
