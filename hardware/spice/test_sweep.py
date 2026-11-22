"""Guards for the SPICE sweep (python3 -m unittest hardware/spice/test_sweep.py)."""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(__file__))
import sweep  # noqa: E402


class SweepTests(unittest.TestCase):
    def test_printer_only_run_really_prints(self):
        # Regression: an earlier version started the printer after the simulation ended, so
        # "printer not during a burst" cases never printed. Printing must raise the pack current.
        idle = sweep._run(3.8, 2, 0.0, 0.0)
        printing = sweep._run(3.8, 2, 1.0, 0.0)
        self.assertGreater(printing[3] - idle[3], 1.0)

    def test_modem_only_run_really_bursts(self):
        idle = sweep._run(3.8, 2, 0.0, 0.0)
        burst = sweep._run(3.8, 2, 0.0, 2.0)
        self.assertGreater(burst[3] - idle[3], 1.5)
        self.assertLess(burst[0], idle[0])

    def test_overlap_is_worse_than_separated(self):
        sep = sweep.run(3.6, 2, 1.0, overlap=False)
        ovl = sweep.run(3.6, 2, 1.0, overlap=True)
        self.assertLess(ovl[0], sep[0])


if __name__ == "__main__":
    unittest.main()
