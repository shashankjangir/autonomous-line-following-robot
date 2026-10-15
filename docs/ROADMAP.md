# Engineering Roadmap: Autonomous Line Following Robot

Updated 15 October 2026. This is the governing implementation and validation guide. The >98% accuracy and >=30% improvement are acceptance targets, not measured achievements. Earlier example numbers and interview answers claiming completed trials were unsupported and have been removed. No physical trial evidence has been supplied.

## 0. Current priority: simulation-first development

No hardware is currently available. Repository preparation and host tests are complete. The [Wokwi firmware demo](WOKWI.md) now covers the first simulation step, with user-confirmed calibration, steering and recovery checks. Closed-loop track modeling and comparative experiments remain the next milestones before physical Phases 1-5 below:

1. Completed initial exercise: [Uno firmware in Wokwi](https://wokwi.com/projects/477942843163696129), using potentiometers as controllable analog inputs and telemetry for motor commands.
2. Implement a closed-loop track model with robot motion, three reflectance channels and the same controller behavior. Record geometry, motor/sensor assumptions, timestep and random seeds.
3. Run fixed/adaptive comparisons across curves, lighting offsets and noise, including failed runs. Derive ground truth from the simulated pose, not the controller's own error estimate.
4. Publish reproducible scenarios, simulation-labeled results and limitations. Keep all numerical simulation claims separate from eventual hardware measurements.

Manual input sweeps alone are firmware exercises, not autonomous tracking experiments. Physical phases remain a future validation path rather than a prerequisite for current software work.

## 1. Scope and specification

Build a differential-drive Arduino Uno robot with calibrated QTR-3A analog inputs, TB6612FNG motor outputs, PD steering, dynamic PWM, bounded recovery and adaptive ambient compensation. The original six-channel code remains in `reference/original_six_sensor_code.txt` for reference. Optional ultrasonic and color features are retained, but disabled in the benchmark build so they do not change speed or stop criteria between fixed/adaptive trials.

| Requirement | Implementation / acceptance evidence | Current status |
|---|---|---|
| Three-channel calibration | Each channel measures background/line for 2 s; absolute contrast >=100 ADC counts; motors inhibited until valid | Implemented; physical calibration pending |
| 100 Hz control | Phase-based 10,000 us scheduler; no ultrasonic pulseIn; record dt_us, max_dt_us and missed_ticks | Build/tests passed; actual timing unverified |
| PD and dynamic PWM | Normalized centroid, filtered derivative, bounded signed motor commands | Implemented; gains require track tuning |
| >98% path accuracy | Measured distance within a predeclared physical tolerance / all measured travel | Pending physical trials |
| >=30% deviation reduction | Matched fixed/adaptive external cross-track MAE, same course/lighting/tolerance | Pending physical trials |
| Honest results | Telemetry proxies cannot establish distance or physical MAE; retain failed runs | Analyzer and regression tests implemented |

## 2. Control mathematics

For each channel, span = line - background and s = clamp((raw - background - ambient)/span, 0, 1). Both polarities work; |span| must be at least 100. In fixed mode, ambient is omitted.

A line is acquired when at least one strength exceeds 0.5. While acquired, error e = (s_right - s_left)/(s_left + s_center + s_right). Positive error turns right. This is a normalized sensor-space position proxy, not a calibrated displacement in millimeters. Symmetric patterns and intersections are ambiguous with only three sensors.

At nominal 10 ms intervals:

- rate = (e - previous_e) / elapsed_seconds
- filtered_rate += 0.25 * (rate - filtered_rate)
- correction = 115 * e + 2.0 * filtered_rate
- base_PWM = 140 - 65 * |e|
- left/right = clamp(base_PWM +/- correction, -255, 255)

These are starting gains, not experimentally tuned values. Reverse PWM is allowed during sharp corrections. After loss, search toward the last error side for at most 800 ms, then latch a stop. Starting without a line stops immediately. Reacquisition resets derivative history. Physical coasting continues after zero commands.

Adaptation is **per channel**, when its normalized value lies strictly between -0.25 and 0.15; it does not require all three to see background. Set target = clamp(raw - background, -0.15*|span|, +0.15*|span|), then ambient += 0.002*(target - ambient). The magnitude bound holds for both signal polarities. The slow update reduces sensitivity to gradual baseline drift but does not prove immunity to sunlight, glare, saturation or misclassification. At 100 Hz, the approximate filter time constant is 5 seconds while the gate is open.

## 3. Measurement definitions

### Physical path accuracy

Use calibrated overhead video or another independent position reference. For each timestamp, record cumulative robot travel d in meters and signed cross-track displacement in millimeters. Choose the physical tolerance before collecting data and keep it identical across modes.

D_total = sum(delta_d); D_right = sum(delta_d * I(|cross_track_mm| <= tolerance_mm)). Accuracy = 100*D_right/D_total; the target is strictly >98%, not >=98%. Analyzer integration uses the earlier sample's error for each interval. Sample densely, especially on corners. Stationary periods add no distance, so report lap completion, elapsed time and interventions separately: a robot that stops early cannot pass a full-course test merely because its partial distance was accurate.

PWM is not measured wheel speed. The motors have no encoders, so onboard logs alone do not supply distance. `seen` and normalized error are useful diagnostics, not independent accuracy evidence. Logs are decimated to 10 Hz even though the control target is 100 Hz; timing counters describe internal cycles.

### Physical deviation reduction

Use time-weighted external MAE = sum(|cross_track_mm|*delta_t)/sum(delta_t), including recovery and stopped periods. Reduction = 100*(MAE_fixed - MAE_adaptive)/MAE_fixed. A zero fixed baseline makes percentage reduction undefined. The target is >=30%; 28% does not pass it.

Telemetry-only MAE is reported in normalized units over acquired intervals with acquisition coverage beside it. Loss periods retain stale controller error, so they must not be treated as accurate zero-error measurements. This proxy cannot verify the 30% physical-deviation target.

Collect at least ten matched pairs per lighting condition, alternate mode order, and report mean, variation and all failures. A single passing pair is preliminary evidence. Do not change the tolerance or omit failing runs to obtain a target result. A lamp's distance does not establish lux; use a meter or record illumination as unmeasured.

## 4. Execution roadmap and completion gates

Durations are estimates after parts and equipment are available, not an automatic schedule.

### Phase 1 — Hardware and track (pending physical work)

- Follow [HARDWARE.md](HARDWARE.md), verify common ground and supply voltage with motors disconnected.
- Mount QTR-3A about 3 mm above the floor and 25-35 mm ahead of the axle.
- Build an 8-12 m closed track with 19 mm matte black tape on white matte material: two >=1.5 m straights, broad >=30 cm-radius bends, two tighter approximately 15 cm-radius quarter turns, and an S-curve. Measure actual centerline length.
- Keep photos of wiring, sensor height, layout, and measured supply voltages.
- Gate: completed hardware checks and measured layout. Not yet verified.

### Phase 2 — Firmware, calibration and bench checks

- Build the baseline profile with the command in README.md. Software compilation is complete; upload is pending an actual board/port.
- Calibrate all three sensors on a wide white sample, then a wide black sample at operating height. A narrow strip under only the center sensor is not sufficient.
- With wheels raised, verify forward direction, left/right correction, button stop and line-loss timeout.
- D3 open at reset selects adaptive mode; D3 grounded selects fixed mode. D2 cycles background/line calibration/run/stop. Serial a/f also select mode and stop the robot.
- Gate: physical calibration, direction and stop checks recorded. Not yet verified.

### Phase 3 — Baseline tracking and timing

- Start at low PWM and tune one gain at a time, recording each change. Suggested experiments: reduce Kp if sustained oscillation occurs; reduce speed before increasing Kd for overshoot. These are experiments, not guaranteed fixes.
- Verify at least three full laps before the comparison campaign. Record completion and interventions.
- Inspect max_dt_us and missed_ticks under load; use a logic analyzer if strict timing validation is needed. A nominal scheduler and a successful compile do not establish deterministic timing. Startup calibration and command handling are excluded from running-cycle timing; asynchronous interrupts still introduce jitter.
- Gate: stable full-course baseline, actual timing evidence and documented fixed gains. Pending.

### Phase 4 — Paired illumination trials and analysis

- Keep gains, course, battery range, tolerance, sensor height and calibration procedure fixed for both modes.
- Use room lighting, a repeatable shadow/spotlight condition, and a repeatable glare gradient. Record lux only when measured. Allow a documented, identical warm-up protocol; adaptation takes seconds.
- Use overhead video for untethered physical measurements. USB telemetry capture is a bench aid; a dragging cable can alter performance. Opening a serial port may reset the Uno, requiring recalibration.
- Record each run separately and preserve original video/logs. See [logs/README.md](../logs/README.md) for measurement fields and metadata.

```powershell
# Bench capture; replace COM5 with the actual port. Requires pyserial.
python scripts/capture_serial.py --port COM5 --seconds 120 logs/bench_fixed_01.csv
# Diagnostic proxy only (no physical pass/fail)
python scripts/analyze_telemetry.py logs/bench_fixed_01.csv
# External measurement schema: ms,distance_m,cross_track_mm,adaptive
# 5 mm below is an example; choose and document your tolerance before testing.
python scripts/analyze_telemetry.py logs/fixed_ground_truth.csv logs/adaptive_ground_truth.csv --ground-truth --tolerance-mm 5
```

- Gate: repeatable distance accuracy >98% with full-course completion and physical MAE reduction >=30% under the stated conditions. Pending, with no guaranteed outcome.

### Phase 5 — Report and resume claims

- Archive firmware/profile, gains, raw evidence, calibration records, timing data, completed/failed runs, and computed metrics.
- Describe what was measured, how tolerance was selected, and the lighting conditions. Report uncertainty/variation and limitations.
- Only add numerical resume claims after the campaign supports them. Until then: "Implemented a three-channel line-following controller with calibrated reflectance sensing, PD steering, dynamic PWM and adaptive background compensation."
- Gate: evidence-backed report; pending hardware trials.

## 5. Interview guide

Explain the algorithm and tradeoffs using the implemented equations. For performance questions, describe the measurement procedure and say trials are pending until real data exists. Dynamic PWM is intended to improve cornering; this project has not established that it eliminates overshoot or guarantees tire adhesion. Ambient adaptation is bounded and gated, not a universal optical-noise rejection guarantee.
