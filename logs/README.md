# Trial evidence

[sample_telemetry.csv](../examples/telemetry/sample_telemetry.csv) is a legacy example with no recorded provenance or mode. It is suitable for parser demonstration only, not evidence of physical performance.

Record one run per file; retain unedited originals, including stop/failure events. Use filenames such as `2026-10-15_room_pair01_fixed.csv` and a matching adaptive file. No measured trial results currently exist in this folder.

For each pair, record track length/layout, sensor height, battery voltage, firmware/profile, gains, start/finish times, lighting setup and measured lux (or explicitly unknown), mode, completion/lap count, tolerance chosen before the run, video filename, and any intervention. Keep camera scale/track-centerline calibration with the video.

External measurement input has the header `ms,distance_m,cross_track_mm,adaptive`. Here `distance_m` is cumulative measured robot travel (nondecreasing), `cross_track_mm` is signed lateral displacement from the track centerline, and `adaptive` is 0 or 1. Measure these from calibrated overhead video or suitable instrumentation; do not derive them from PWM. Include stopped and lost-line periods. Use one common tolerance for all trials and uniformly spaced video samples. The analyzer uses left-endpoint intervals, so sample densely enough for the chosen tolerance and corner geometry.

Long recording gaps, resets, malformed data and mixed modes are errors, not samples to discard silently. Endpoint failure frames and full-lap completion must be reviewed independently of numeric accuracy.

Raw files in this directory are ignored by Git. Publish only deliberately selected, documented evidence under examples/ or a separate results directory.
