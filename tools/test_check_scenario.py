"""Tests for check_scenario.py (run: python3 -m unittest tools/test_check_scenario.py)."""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(__file__))
import check_scenario as cs  # noqa: E402

LOG = """\
seri-tool 0.2.0 (SIM x60) booting
[modem] SMS to 9876543210: Shed-1: OK
LOG E,1795221000,TempHigh,Start,310
[printer] PRINTER| 21/11 06:10 TEMP HIGH 31.0C
[modem] SMS to 9876543210: Shed-1: Temp HIGH 31.0C (safe 26-29C) Chawki 06:10
[modem] MQTT PUBLISH seri/Shed-1/data 120B #1 {}
[modem] MQTT PUBLISH seri/Shed-1/data 120B #2 {}
[printer] PRINTER| 21/11 06:40 TEMP OK 28.0C
"""


class CheckTests(unittest.TestCase):
    def run_rules(self, text):
        return cs.check(cs.parse_rules(text), LOG.splitlines())

    def test_in_order_passes_when_lines_appear_in_sequence(self):
        self.assertEqual([], self.run_rules("in-order: SMS to .*: OK\nin-order: TEMP HIGH 31.0C\nin-order: TEMP OK"))

    def test_in_order_fails_when_out_of_sequence(self):
        errs = self.run_rules("in-order: TEMP OK\nin-order: TEMP HIGH")
        self.assertEqual(1, len(errs))
        self.assertIn("TEMP HIGH", errs[0])

    def test_count_rules(self):
        self.assertEqual([], self.run_rules("count >= 2: MQTT PUBLISH\ncount == 1: Temp HIGH 31.0C \\(safe"))
        self.assertEqual(1, len(self.run_rules("count >= 3: MQTT PUBLISH")))
        self.assertEqual(1, len(self.run_rules("count == 0: MQTT PUBLISH")))
        self.assertEqual([], self.run_rules("count <= 2: MQTT PUBLISH"))

    def test_absent_and_present(self):
        self.assertEqual([], self.run_rules("absent: Power cut\npresent: booting"))
        self.assertEqual(1, len(self.run_rules("absent: TEMP HIGH")))
        self.assertEqual(1, len(self.run_rules("present: Battery low")))

    def test_comments_and_blank_lines_ignored_and_bad_rule_rejected(self):
        self.assertEqual([], self.run_rules("# comment\n\nin-order: booting"))
        with self.assertRaises(ValueError):
            cs.parse_rules("sometimes: whatever")

    def test_ansi_colour_codes_are_stripped(self):
        rules = cs.parse_rules("in-order: \\[modem\\] SMS to")
        self.assertEqual([], cs.check(rules, ["\x1b[35mmodem\x1b[39m".join(["[", "] SMS to 1"])]))


if __name__ == "__main__":
    unittest.main()
