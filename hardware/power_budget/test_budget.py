"""Tests for budget.py (run: python3 -m unittest hardware/power_budget/test_budget.py)."""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(__file__))
import budget  # noqa: E402

LOADS = {
    "battery": {"usable_fraction": {"value": 0.8, "source": "test"}},
    "loads": {
        "always":   {"ma": 10.0, "duty": 1.0, "source": "test"},
        "burst":    {"ma": 1000.0, "duty": 0.01, "source": "test"},
        "mains_only": {"ma": 50.0, "duty": 1.0, "source": "test", "on_battery": False},
    },
}


class BudgetTests(unittest.TestCase):
    def test_average_current_weights_duty_and_skips_mains_only_loads(self):
        self.assertAlmostEqual(20.0, budget.average_ma(LOADS["loads"], on_battery=True))
        self.assertAlmostEqual(70.0, budget.average_ma(LOADS["loads"], on_battery=False))

    def test_runtime_uses_usable_fraction(self):
        # 5000 mAh * 0.8 / 20 mA = 200 h
        self.assertAlmostEqual(200.0, budget.runtime_hours(5000, LOADS))

    def test_pending_sources_block_final_report(self):
        bad = {"battery": LOADS["battery"], "loads": {"x": {"ma": 1, "duty": 1, "source": "PENDING datasheet"}}}
        self.assertEqual(["loads.x"], budget.pending(bad))
        self.assertEqual([], budget.pending(LOADS))

    def test_duty_from_events(self):
        # 2 s every 900 s
        self.assertAlmostEqual(2 / 900, budget.duty(seconds=2, every_seconds=900))

    def test_assumed_values_are_listed_for_bench_measurement(self):
        cfg = {"battery": LOADS["battery"],
               "loads": {"a": {"ma": 1, "duty": 1, "source": "ASSUMED: no datasheet value"},
                         "b": {"ma": 1, "duty": 1, "source": "WROOM p29"}}}
        self.assertEqual(["loads.a"], budget.assumed(cfg))

    def test_sensitivity_overrides_one_load_current(self):
        # 'always' 10 mA -> 30 mA: average 20 -> 40 mA, runtime 5000*0.8/40 = 100 h
        self.assertAlmostEqual(100.0, budget.runtime_hours(5000, LOADS, override={"always": 30.0}))

    def test_runtime_meets_target_check(self):
        cfg = dict(LOADS, target={"hours": 24, "pack_mah": 5000})
        self.assertTrue(budget.meets_target(cfg))
        cfg = dict(LOADS, target={"hours": 300, "pack_mah": 5000})
        self.assertFalse(budget.meets_target(cfg))


if __name__ == "__main__":
    unittest.main()
