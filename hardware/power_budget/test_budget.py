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


if __name__ == "__main__":
    unittest.main()
