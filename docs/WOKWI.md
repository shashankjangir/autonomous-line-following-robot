# Wokwi firmware demo

**[Open the project in Wokwi](https://wokwi.com/projects/477942843163696129)**

![Arduino Uno and three simulated reflectance inputs in Wokwi](images/wokwi-demo.png)

This browser demo runs the Arduino Uno firmware with three potentiometers representing left, center and right reflectance readings. Motor commands appear in serial telemetry. The hosted project is maintained separately from the files in this repository; edits are not automatically synchronized.

## Run the demo

1. Start the simulation and open the Serial Monitor.
2. Set all three potentiometers to 0. Send `b` and wait for the two-second background calibration to finish.
3. Set all three to 1023. Send `l` and wait for line calibration to finish.
4. Send `f` for fixed mode. Set left/center/right to `0,1023,0`, then send `r`.
5. Expect error `0.000`, seen `1`, motor commands `140,140`, and state `track`.

Send `s` before changing inputs for each steering test, then `r` after setting them:

| Test | Left | Center | Right | Expected settled left/right commands |
|---|---:|---:|---:|---|
| Centered | 0 | 1023 | 0 | 140, 140 |
| Line on left | 1023 | 0 | 0 | Approximately -40, 190 |
| Line on right | 0 | 0 | 1023 | Approximately 190, -40 |

For recovery, start centered, then change the center input to 0 while running. With all three inputs zero, the controller should search briefly and latch `lost_stop` after about 800 ms. Both commands become zero; restoring the center input does not restart movement until `r` is sent.

## Observed checks

On 15 October 2026, the user reported successful calibration, centered tracking, left/right steering and line-loss stopping. The centered-input telemetry supplied in chat showed zero error, equal PWM commands of 140, approximately 10 ms control intervals, and zero missed ticks or dropped logs. Steering and loss-recovery checks were confirmed by the user; their raw traces were not provided.

These checks exercise firmware responses to manually controlled inputs. The demo does not yet model a robot moving around a track, so it does not establish path-tracking accuracy or physical performance.
