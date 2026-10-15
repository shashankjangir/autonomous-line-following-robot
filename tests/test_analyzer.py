import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('analyzer', Path(__file__).parents[1] / 'scripts/analyze_telemetry.py')
analyzer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analyzer)


class AnalyzerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)

    def file(self, content, name='run.csv'):
        path = Path(self.temp.name) / name
        path.write_text(content, encoding='utf-8')
        return path

    def telemetry(self, body):
        return self.file('ms,error,seen,adaptive,state\n' + body)

    def test_time_weighting_and_no_distance_claim(self):
        p = self.telemetry('0,0.2,1,0,track\n100,0.8,1,0,track\n400,0,1,0,track\n')
        r = analyzer.analyze_telemetry(p)
        self.assertAlmostEqual(r['mae'], .65)
        self.assertEqual(r['time_in_tolerance_pct'], 25)
        self.assertNotIn('total_distance_m', r)
        self.assertIsNone(r['accuracy_target_met'])

    def test_lost_line_not_counted_as_zero_error_success(self):
        p = self.telemetry('0,0,0,0,track\n100,0,0,0,lost_stop\n')
        r = analyzer.analyze_telemetry(p)
        self.assertEqual(r['time_in_tolerance_pct'], 0)
        self.assertIsNone(r['mae'])

    def test_ground_truth_uses_distance_weights(self):
        p = self.file('ms,distance_m,cross_track_mm,adaptive\n0,0,0,0\n100,1,20,0\n200,10,0,0\n')
        r = analyzer.analyze_telemetry(p, ground_truth=True, tolerance_mm=5)
        self.assertEqual(r['distance_accuracy_pct'], 10)
        self.assertEqual(r['mae'], 10)

    def test_exact_98_does_not_meet_greater_than_target(self):
        p = self.file('ms,distance_m,cross_track_mm\n0,0,0\n100,98,20\n200,100,0\n')
        self.assertFalse(analyzer.analyze_telemetry(p, ground_truth=True, tolerance_mm=5)['accuracy_target_met'])

    def test_gap_and_reset_rejected(self):
        for end in ('600', '0', '-1'):
            p = self.telemetry(f'0,0,1,0,track\n{end},0,1,0,track\n')
            with self.assertRaises(ValueError): analyzer.analyze_telemetry(p)

    def test_invalid_data_not_silently_dropped(self):
        for error in ('nan', 'inf', 'garbage', '2'):
            p = self.telemetry(f'0,{error},1,0,track\n100,0,1,0,track\n')
            with self.assertRaises(ValueError): analyzer.analyze_telemetry(p)

    def test_mixed_modes_rejected(self):
        p = self.telemetry('0,0,1,0,track\n100,0,1,1,track\n')
        with self.assertRaises(ValueError): analyzer.analyze_telemetry(p)

    def test_comparison_requires_modes(self):
        p = self.file('ms,error,seen\n0,0,1\n100,0,1\n')
        with self.assertRaises(ValueError): analyzer.compare_modes(p,p)

    def test_zero_baseline_is_undefined(self):
        a = self.file('ms,error,seen,adaptive\n0,0,1,0\n100,0,1,0\n', 'fixed.csv')
        b = self.file('ms,error,seen,adaptive\n0,0,1,1\n100,0,1,1\n', 'adaptive.csv')
        self.assertIsNone(analyzer.compare_modes(a,b)['mae_reduction_pct'])

    def test_28_percent_does_not_pass_30(self):
        a = self.file('ms,distance_m,cross_track_mm,adaptive\n0,0,100,0\n100,1,100,0\n', 'fixed.csv')
        b = self.file('ms,distance_m,cross_track_mm,adaptive\n0,0,72,1\n100,1,72,1\n', 'adaptive.csv')
        r = analyzer.compare_modes(a,b,ground_truth=True,tolerance_mm=5)
        self.assertEqual(r['mae_reduction_pct'],28)
        self.assertFalse(r['deviation_target_met'])

    def test_comments_and_insufficient_data(self):
        p = self.file('# startup\nms,error,seen\n0,0,1\n')
        with self.assertRaises(ValueError): analyzer.analyze_telemetry(p)

    def test_multiple_runs_rejected(self):
        p = self.file('ms,error,seen\n# Running...\n0,0,1\n# Running...\n100,0,1\n')
        with self.assertRaises(ValueError): analyzer.analyze_telemetry(p)


if __name__ == '__main__':
    unittest.main()
