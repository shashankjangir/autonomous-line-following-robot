# Telemetry example

`sample_telemetry.csv` is a small legacy parser example. Its measurement provenance and fixed/adaptive mode are unrecorded, so it must not be cited as experimental evidence or used for a mode comparison.

From the repository root:

```sh
python scripts/analyze_telemetry.py examples/telemetry/sample_telemetry.csv
```

The output is a normalized telemetry diagnostic with no physical accuracy pass/fail. See [measurement guidance](../../logs/README.md) before recording new evidence. The current firmware emits additional mode and timing columns described in [usage](../../docs/USAGE.md).
