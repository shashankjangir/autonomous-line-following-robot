# Development guide

Use Python 3.10+ and a C++11-capable g++ compiler for host development. Arduino CLI with `arduino:avr@1.8.8` is required only for Uno compilation. See [README.md](README.md) for setup.

Before submitting a change:

1. Run `python scripts/check.py` from the repository root.
2. Compile both firmware profiles when changing the sketch or headers.
3. Update documentation when commands, wiring, telemetry fields or behavior change.
4. Label example, simulated and physically measured data accurately. Never report controller-test results as track performance.

Keep compiled programs, downloaded tools, local configuration, credentials and raw personal recordings out of commits. Curated reproducible examples belong in `examples/`; local captures belong in ignored `logs/` files. Preserve the archived source in `reference/` as historical input, not the active implementation.

For bug reports, include the firmware profile, reproduction steps, expected/actual behavior and relevant logs without credentials or personal information. Tests should cover behavior and failure cases affected by the change.
