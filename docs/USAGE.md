# Operation and telemetry

Run all shell commands from the repository root. Hardware instructions below are for later bench validation; no hardware is currently available. See the [project overview](../README.md) for checks that run without hardware.

Follow [ROADMAP.md](ROADMAP.md) for the governing specification, simulation-first next steps and measurement procedure. Physical assembly, upload, tuning and the >98% / >=30% performance targets remain unverified.

## Build and hardware

Arduino Uno R3, QTR-3A analog array and TB6612FNG differential drive; see [HARDWARE.md](HARDWARE.md) for wiring. `reference/original_six_sensor_code.txt` is preserved as the original six-channel reference.

```powershell
arduino-cli compile --fqbn arduino:avr:uno --warnings all firmware/LineFollower
```

Alternatively open `firmware/LineFollower/LineFollower.ino` in Arduino IDE and select Uno and the actual board port. No external firmware libraries are required. The local CLI/core are isolated under ignored `build/`. Never infer the upload port from an example.

The default benchmark profile disables HC-SR04 and TCS34725. To test them, set ENABLE_ULTRASONIC and/or ENABLE_COLOR to 1 in the sketch (or pass compiler definitions). The echo driver uses Uno D12 pin-change interrupts; this implementation is Uno-specific. The TCS34725 driver has a bounded Wire timeout and performs oscillator startup delay only in setup. Missing/invalid ultrasonic data pauses motors when that feature is enabled; no echo is not assumed to mean a clear path. Optional red markers latch a stop and green markers scale PWM by 80%; color thresholds need hardware calibration. These features are not collision-safety guarantees and are excluded from benchmark comparisons.

## Calibration and operation

Connect D2 through a momentary button to GND. At reset, D3 open selects adaptive mode; a D3-to-GND jumper selects fixed mode for untethered comparisons. Do not wire D3 to the battery supply. Calibration remains in RAM and is lost on reset.

1. Hold all three sensors over a wide white sample at operating height. Press D2; sampling lasts 2 seconds while LED blinks.
2. Hold all three over a wide black sample. Press D2 and wait for sampling to finish. Each channel must have >=100 ADC counts of contrast to run.
3. Place on track; press D2 to run. Press again to stop. Reset to repeat the calibration sequence.

At 115200 baud, serial commands are b=background, l=line, r=run, s=stop, a=adaptive, f=fixed. Mode changes stop motors and reset ambient offsets. Raising the wheels for direction/stop checks precedes floor trials.

For comparator-output sensors, set ANALOG_SENSORS=false; software optical threshold adaptation is unavailable. Calibration then checks stable opposite digital levels and polarity. The selected QTR-3A uses the default analog mode.

## Control and telemetry

`Control.h` implements normalized centroid position, filtered PD feedback, curve slowdown, gated bounded ambient compensation and an 800 ms latched loss timeout. `Timing.h` implements phase-based 10 ms scheduling, skipped-period counts and echo edge capture. Motor zero commands coast; they do not actively brake.

Telemetry snapshots are queued every 100 ms and drained only into available UART buffer space. Control is nominally 100 Hz, telemetry 10 Hz. Actual timing needs bench verification. Startup/commands are outside the running timing budget. Record these columns:

```text
ms,raw_L,raw_C,raw_R,error,seen,pwm_L,pwm_R,dist_cm,color,adaptive,dt_us,max_dt_us,missed_ticks,state,dropped_logs
```

Color enum: 0=unknown/disabled, 1=black, 2=white, 3=red, 4=green. Distance -1 means unavailable. State is track, obstacle, red_stop or lost_stop. `seen` is not a ground-truth accuracy measurement. `max_dt_us` and missed_ticks are cumulative since run start; dropped_logs records telemetry backlog/format failures. Inspect these and run completeness before comparing results.

```powershell
python scripts/analyze_telemetry.py examples/telemetry/sample_telemetry.csv
python -m unittest discover -s tests -p 'test_*.py' -v
g++ -std=c++11 -Wall -Wextra -pedantic tests/control_test.cpp -o tests/control_test.exe
.\tests\control_test.exe
```

The sample file has no measurement provenance and is a parser example only. The analyzer reports time-weighted telemetry proxies without fabricated distance or physical pass/fail. For externally measured distance and displacement, use --ground-truth --tolerance-mm with the schema and paired-trial method in the roadmap. Bench capture is available via scripts/capture_serial.py (requires pyserial); camera-based physical measurement remains a separate real experiment.

## Verification status

See [VALIDATION.md](VALIDATION.md) for software verification and the next physical gates. Compilation and host tests do not establish sensor reliability, deterministic physical timing, tracking accuracy or lighting robustness.
