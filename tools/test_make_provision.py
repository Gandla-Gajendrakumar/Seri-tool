"""Tests for make_provision.py (python3 -m unittest tools/test_make_provision.py)."""
import csv
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(__file__))
import make_provision  # noqa: E402


class ProvisionTests(unittest.TestCase):
    def test_batch_is_valid_unique_and_complete(self):
        out = tempfile.mkdtemp()
        self.assertEqual(0, make_provision.main(["--host", "abc123.s1.eu.hivemq.cloud", "--count", "3", "--out", out]))
        rows = list(csv.DictReader(open(os.path.join(out, "credentials.csv"))))
        self.assertEqual(["seri-0001", "seri-0002", "seri-0003"], [r["client_id"] for r in rows])
        self.assertEqual(3, len({r["password"] for r in rows}))
        for r in rows:
            cfg = open(os.path.join(out, r["client_id"], "seri.cfg")).read()
            self.assertIn("mqtt_host=abc123.s1.eu.hivemq.cloud\n", cfg)
            self.assertIn(f"client_id={r['client_id']}\n", cfg)
            self.assertIn("BEGIN CERTIFICATE", open(os.path.join(out, r["client_id"], "ca.pem")).read())

    def test_rejects_a_url_instead_of_a_host(self):
        with self.assertRaises(SystemExit):
            make_provision.main(["--host", "mqtts://abc.hivemq.cloud", "--count", "1", "--out", tempfile.mkdtemp()])


if __name__ == "__main__":
    unittest.main()
