"""Analyze telemetry proxies or externally measured distance/cross-track data.
Never infer travelled distance from PWM or a constant assumed wheel speed.
"""
import argparse
import csv
import json
import math
from pathlib import Path


def number(row, key):
    try:
        value = float(row[key])
    except (KeyError, TypeError, ValueError) as exc:
        raise ValueError(f"Invalid or missing {key}: {row.get(key)!r}") from exc
    if not math.isfinite(value):
        raise ValueError(f"Non-finite {key}")
    return value


def load_rows(path):
    with Path(path).open(encoding='utf-8-sig', newline='') as stream:
        content = list(stream)
        if sum(line.strip() == '# Running...' for line in content) > 1:
            raise ValueError('Multiple runs in file; record each run separately')
        lines = (line for line in content if line.strip() and not line.lstrip().startswith('#'))
        reader = csv.DictReader(lines)
        if not reader.fieldnames or 'ms' not in reader.fieldnames:
            raise ValueError('Expected a CSV header with ms; retain one run per file')
        rows = list(reader)
    if len(rows) < 2:
        raise ValueError('At least two samples are required')
    for row in rows:
        if None in row or any(value is None for value in row.values()):
            raise ValueError('Truncated or extra CSV fields')
    return rows


def analyze_telemetry(csv_path, error_tolerance=0.45, ground_truth=False,
                      tolerance_mm=None, max_gap_ms=500):
    if not 0 <= error_tolerance <= 1:
        raise ValueError('Normalized tolerance must be in [0, 1]')
    if max_gap_ms <= 0:
        raise ValueError('max_gap_ms must be positive')
    if ground_truth and (tolerance_mm is None or not math.isfinite(tolerance_mm) or tolerance_mm <= 0):
        raise ValueError('Ground truth requires a positive --tolerance-mm chosen before testing')
    rows = load_rows(csv_path)
    times = [number(row, 'ms') for row in rows]
    intervals = [b - a for a, b in zip(times, times[1:])]
    if any(dt <= 0 or dt > max_gap_ms for dt in intervals):
        raise ValueError('Non-monotonic timestamps or recording gap; split runs and investigate missing data')
    modes = {row.get('adaptive') for row in rows}
    if modes - {None, '0', '1'} or len(modes) != 1:
        raise ValueError('Missing/invalid mode or mixed fixed/adaptive modes in one run')
    mode = next(iter(modes))
    result = {'file': str(csv_path), 'samples': len(rows), 'duration_s': sum(intervals)/1000,
              'mode': {'0': 'fixed', '1': 'adaptive', None: 'unrecorded'}[mode],
              'source': 'external_ground_truth' if ground_truth else 'telemetry_proxy'}
    if ground_truth:
        distances = [number(row, 'distance_m') for row in rows]
        if any(d < 0 for d in distances):
            raise ValueError('Cumulative distance cannot be negative')
        steps = [b-a for a,b in zip(distances, distances[1:])]
        if any(d < 0 for d in steps) or sum(steps) <= 0:
            raise ValueError('distance_m must be nondecreasing measured cumulative travel with positive total')
        errors = [abs(number(row, 'cross_track_mm')) for row in rows]
        # Left-endpoint integration; includes stopped time in MAE, not distance.
        right = sum(d for d,e in zip(steps, errors) if e <= tolerance_mm)
        result.update(total_distance_m=sum(steps), in_tolerance_distance_m=right,
                      distance_accuracy_pct=100*right/sum(steps), tolerance_mm=tolerance_mm,
                      mae=sum(e*dt for e,dt in zip(errors,intervals))/sum(intervals),
                      mae_unit='mm', accuracy_target_met=100*right/sum(steps) > 98)
    else:
        errors = [abs(number(row, 'error')) for row in rows]
        seen = [number(row, 'seen') for row in rows]
        if any(e > 1 for e in errors) or any(s not in (0,1) for s in seen):
            raise ValueError('error must be in [-1,1] and seen must be 0 or 1')
        states = [row.get('state', 'track') for row in rows]
        if any(s not in ('track','obstacle','red_stop','lost_stop') for s in states):
            raise ValueError('Invalid controller state')
        valid = [bool(s) and state == 'track' for s,state in zip(seen, states)]
        acquired = sum(dt for dt,v in zip(intervals,valid) if v)
        good = sum(dt for dt,v,e in zip(intervals,valid,errors) if v and e <= error_tolerance)
        result.update(time_in_tolerance_pct=100*good/sum(intervals),
                      acquired_time_pct=100*acquired/sum(intervals),
                      mae=(sum(e*dt for e,dt,v in zip(errors,intervals,valid) if v)/acquired if acquired else None),
                      mae_unit='normalized, acquired intervals only',
                      accuracy_target_met=None,
                      note='No measured distance or physical deviation: cannot verify resume targets.')
        for field in ('max_dt_us','missed_ticks','dropped_logs'):
            if field in rows[0]:
                result[field] = max(number(row,field) for row in rows)
    return result


def compare_modes(fixed_csv, adaptive_csv, **kwargs):
    fixed = analyze_telemetry(fixed_csv, **kwargs)
    adaptive = analyze_telemetry(adaptive_csv, **kwargs)
    if fixed['mode'] != 'fixed' or adaptive['mode'] != 'adaptive':
        raise ValueError('Comparison requires adaptive=0 in fixed file and adaptive=1 in adaptive file')
    a, b = fixed['mae'], adaptive['mae']
    reduction = 100*(a-b)/a if a is not None and a > 0 and b is not None else None
    physical = fixed['source'] == 'external_ground_truth'
    return {'fixed': fixed, 'adaptive': adaptive, 'mae_reduction_pct': reduction,
            'deviation_target_met': reduction >= 30 if physical and reduction is not None else None,
            'note': 'A matched-pair calculation; repeat across lighting conditions and retain all failed runs. '
                    'Telemetry proxy improvement is not physical trajectory improvement.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('files', nargs='+', help='One run, or fixed then adaptive run')
    parser.add_argument('--ground-truth', action='store_true')
    parser.add_argument('--tolerance-mm', type=float)
    parser.add_argument('--error-tolerance', type=float, default=0.45)
    parser.add_argument('--max-gap-ms', type=float, default=500)
    args = parser.parse_args()
    if len(args.files) not in (1,2):
        parser.error('Supply one run or a fixed/adaptive pair')
    options = dict(ground_truth=args.ground_truth, tolerance_mm=args.tolerance_mm,
                   error_tolerance=args.error_tolerance, max_gap_ms=args.max_gap_ms)
    try:
        result = (analyze_telemetry(args.files[0], **options) if len(args.files)==1
                  else compare_modes(*args.files, **options))
    except (ValueError,OSError) as exc:
        parser.exit(2, f'Analysis error: {exc}\n')
    print(json.dumps(result, indent=2, allow_nan=False))


if __name__ == '__main__':
    main()
