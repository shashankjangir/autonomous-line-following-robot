"""Capture one bench run; the D2 button controls calibration/start/stop.
Install pyserial first: python -m pip install pyserial
"""
import argparse
from pathlib import Path
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True, help='Actual Uno port, e.g. COM5')
    parser.add_argument('--seconds', type=float, default=120)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    if args.seconds <= 0:
        parser.error('--seconds must be positive')
    try:
        import serial
    except ImportError:
        parser.exit(2, 'Install pyserial: python -m pip install pyserial\n')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    try:
        # Exclusive creation prevents overwriting trial evidence.
        with args.output.open('x', encoding='utf-8', newline='') as output:
            with serial.Serial(args.port, 115200, timeout=0.2) as port:
                print('Port open (Uno may reset). Calibrate and start with D2. Record only one run.')
                deadline = time.monotonic() + args.seconds
                while time.monotonic() < deadline:
                    raw = port.readline()
                    if raw:
                        output.write(raw.decode('utf-8', errors='strict'))
                        output.flush()
    except (OSError, UnicodeError, serial.SerialException) as exc:
        parser.exit(2, f'Capture failed: {exc}\n')
    print(f'Saved {args.output}; inspect completeness before analysis.')


if __name__ == '__main__':
    main()
