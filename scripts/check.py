"""Run portable host checks from any directory. Requires Python 3.10+ and g++."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def run(*command):
    print('+ ' + ' '.join(str(arg) for arg in command), flush=True)
    subprocess.run([str(arg) for arg in command], cwd=ROOT, check=True)


def main():
    compiler = shutil.which(os.environ.get('CXX', 'g++'))
    if not compiler:
        sys.exit('C++ compiler not found. Install g++ or set CXX to the compiler executable.')
    build = ROOT / 'build' / 'host'
    build.mkdir(parents=True, exist_ok=True)
    executable = build / ('control_test.exe' if os.name == 'nt' else 'control_test')
    try:
        run(sys.executable, '-m', 'unittest', 'discover', '-s', 'tests', '-p', 'test_*.py', '-v')
        run(compiler, '-std=c++11', '-Wall', '-Wextra', '-pedantic',
            'tests/control_test.cpp', '-o', executable)
        run(executable)
        run(sys.executable, 'scripts/analyze_telemetry.py', 'examples/telemetry/sample_telemetry.csv')
    except subprocess.CalledProcessError as exc:
        sys.exit(exc.returncode)
    print('All host checks passed. Hardware and simulation performance are not validated.')


if __name__ == '__main__':
    main()
