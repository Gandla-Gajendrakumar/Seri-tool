"""Smoke test: a resistor between two rails must load in KiCad 9 and pass ERC with zero violations."""
import json
import os
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(__file__))
from schgen import Library, Schematic  # noqa: E402


class SchGenTests(unittest.TestCase):
    def test_minimal_circuit_passes_erc(self):
        s = Schematic("t", "test", Library())
        s.add("Device", "R", "R1", "10k", (50.8, 50.8), {"1": "+3V3", "2": "GND"}, "Resistor_SMD:R_1206_3216Metric")
        s.add("power", "+3V3", "#PWR01", "+3V3", (76.2, 38.1), {"1": "+3V3"})
        s.add("power", "GND", "#PWR02", "GND", (76.2, 63.5), {"1": "GND"})
        s.add("power", "PWR_FLAG", "#FLG01", "PWR_FLAG", (101.6, 38.1), {"1": "+3V3"})
        s.add("power", "PWR_FLAG", "#FLG02", "PWR_FLAG", (101.6, 63.5), {"1": "GND"})
        d = tempfile.mkdtemp()
        p = os.path.join(d, "t.kicad_sch")
        s.save(p)
        out = os.path.join(d, "erc.json")
        r = subprocess.run(["kicad-cli", "sch", "erc", "--format", "json", "--severity-all", "-o", out, p],
                           capture_output=True, text=True)
        self.assertTrue(os.path.exists(out), r.stdout + r.stderr)
        rep = json.load(open(out))
        viol = [v for sh in rep["sheets"] for v in sh["violations"]]
        self.assertEqual([], [v["description"] for v in viol])

    def test_unconnected_pin_is_refused(self):
        s = Schematic("t", "test", Library())
        with self.assertRaises(ValueError):
            s.add("Device", "R", "R1", "10k", (0, 0), {"1": "A"})


if __name__ == "__main__":
    unittest.main()
