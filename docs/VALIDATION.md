# Roadmap execution record — 15 October 2026

Repository preparation: documentation moved under docs/, the original sketch under reference/, and the parser sample under examples/telemetry/. Portable host checks and GitHub Actions configuration have been added. Hosted CI has not run yet. No hardware is available; simulation is the next planned milestone and has not been implemented.

Packaging verification: all 12 Python tests and the C++ checks passed both in the workspace and in a clean copy containing only publishable files. Local Markdown links, publishable file sizes, basic credential/machine-path patterns, and ignore rules were checked. This was not a comprehensive secret audit. GitHub publication and hosted CI execution have not been performed.

## Software checks completed

| Check | Result |
|---|---|
| Uno baseline firmware, Arduino AVR core 1.8.8 | Compiles: 12,308 bytes flash; 680 bytes global RAM |
| Uno with ultrasonic and color enabled | Compiles: 15,086 bytes flash; 808 bytes global RAM |
| C++ controller, sensor normalization/adaptation, scheduler and echo tests | Passed |
| Python analyzer regression suite | 12 tests passed |
| Legacy sample analysis | Parsed as diagnostic proxy; no physical target pass issued |
| Serial capture command-line help | Passed; actual serial capture not exercised |
| Board discovery | Serial ports found, but none identified as an Arduino Uno |

Initial clean compilations emitted unused-parameter warnings from the installed Arduino core's new.cpp, not project sources. Final incremental builds completed successfully. Global RAM figures exclude dynamic stack usage.

## Changes made to satisfy software prerequisites

- Replaced the 15 ms pulseIn wait with interrupt-based echo capture on Uno D12; unknown/stale range pauses the optional obstacle profile.
- Added phase-based 10 ms scheduling, measured cycle diagnostics, skipped periods, and queued UART output.
- Added fixed/adaptive boot selection on D3; optional color/obstacle effects default off for controlled comparisons.
- Bounded Wire transactions and sequenced color channel reads; color detection remains provisional and needs physical calibration.
- Replaced PWM/constant-speed distance claims with explicit diagnostic-only telemetry metrics and a separate external-measurement path.
- Enforced >98% and >=30% thresholds, rejected malformed/mixed/gapped runs, handled zero baseline, and added a serial capture utility.
- Corrected unsupported trial claims and adaptation guarantees in the roadmap. Preserved the original reference/original_six_sensor_code.txt and existing sample file.

Implementation references: [Arduino Wire timeout API](https://github.com/arduino/ArduinoCore-avr/blob/master/libraries/Wire/src/Wire.h) and [Adafruit TCS34725 startup sequence](https://github.com/adafruit/Adafruit_TCS34725/blob/master/Adafruit_TCS34725.cpp).

## Remaining phase gates

1. Assemble/verify hardware and measure the track (Phase 1).
2. Identify the actual Uno port, upload, calibrate and check motor direction/stopping (Phase 2). Unknown serial ports were not flashed.
3. Measure actual loop timing and tune full-course tracking (Phase 3).
4. Collect paired fixed/adaptive videos and logs across lighting conditions, retaining failures (Phase 4).
5. Produce the final numerical report and resume claims only from that evidence (Phase 5).

No motor operation, physical upload, actual 100 Hz timing guarantee, >98% path accuracy, or 30% deviation reduction has been verified. Completing software prerequisites does not complete the physical roadmap.
