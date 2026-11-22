"""Tests for gen_bom.py (python3 -m unittest bom/test_bom.py)."""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(__file__))
import gen_bom  # noqa: E402

SRC = {
    "Widget": {"item": "Widget", "mpn": "W1", "sources": [
        {"supplier": "LCSC", "country": "CN", "url": "u1", "pack_qty": 10, "pack_price_inr": 50.0},
        {"supplier": "Robu.in", "country": "IN", "url": "u2", "pack_qty": 100, "pack_price_inr": 200.0}]},
    "Gadget": {"item": "Gadget", "mpn": "G1", "sources": [
        {"supplier": "Robu.in", "country": "IN", "url": "u3", "pack_qty": 1, "pack_price_inr": None}]},
}


class BomTests(unittest.TestCase):
    def test_prefers_priced_indian_source(self):
        s = gen_bom.pick_source(SRC["Widget"])
        self.assertEqual("Robu.in", s["supplier"])

    def test_selected_source_wins_over_cheaper(self):
        e = {"item": "X", "sources": [
            {"supplier": "Cheap", "country": "IN", "url": "a", "pack_qty": 1, "pack_price_inr": 5.0},
            {"supplier": "Genuine", "country": "CN", "url": "b", "pack_qty": 1, "pack_price_inr": 9.0, "selected": True}]}
        self.assertEqual("Genuine", gen_bom.pick_source(e)["supplier"])

    def test_out_of_stock_skipped_unless_selected(self):
        e = {"item": "X", "sources": [
            {"supplier": "Gone", "country": "IN", "url": "a", "pack_qty": 1, "pack_price_inr": 5.0, "in_stock": False},
            {"supplier": "Here", "country": "IN", "url": "b", "pack_qty": 1, "pack_price_inr": 9.0}]}
        self.assertEqual("Here", gen_bom.pick_source(e)["supplier"])

    def test_chosen_out_of_stock_source_is_an_availability_problem(self):
        src = {"X": {"item": "X", "sources": [
            {"supplier": "S", "country": "IN", "url": "a", "pack_qty": 1, "pack_price_inr": 5.0,
             "in_stock": False, "selected": True}]}}
        rows, missing = gen_bom.price_lines([{"item": "X", "qty": 1, "refs": "", "dnp_refs": ""}], src)
        self.assertEqual([], missing)
        self.assertEqual(["X"], gen_bom.availability_problems(rows))

    def test_unit_and_batch_cost_use_whole_packs(self):
        s = gen_bom.pick_source(SRC["Widget"])
        self.assertAlmostEqual(2.0, gen_bom.unit_inr(s))                 # 200 / 100
        self.assertAlmostEqual(200.0, gen_bom.batch_inr(s, qty=3, units=10))   # 30 pcs -> 1 pack
        self.assertAlmostEqual(400.0, gen_bom.batch_inr(s, qty=11, units=10))  # 110 pcs -> 2 packs

    def test_missing_price_makes_bom_not_final(self):
        lines = [{"item": "Gadget", "qty": 1, "refs": "X1"}]
        rows, missing = gen_bom.price_lines(lines, SRC)
        self.assertEqual(["Gadget"], missing)
        self.assertIsNone(rows[0]["unit_inr"])

    def test_unknown_item_is_an_error(self):
        with self.assertRaises(KeyError):
            gen_bom.price_lines([{"item": "Nope", "qty": 1, "refs": ""}], SRC)

    def test_every_carrier_part_maps_to_a_research_item(self):
        lines = gen_bom.carrier_lines()
        self.assertTrue(lines)
        for ln in lines:
            self.assertTrue(ln["item"], ln)

    def test_dnp_parts_are_listed_but_not_counted(self):
        lines = gen_bom.carrier_lines()
        holders = [ln for ln in lines if ln["item"].startswith("18650 PCB holder")]
        self.assertEqual(2, sum(ln["qty"] for ln in holders))
        self.assertIn("BT3", " ".join(ln.get("dnp_refs", "") for ln in holders))


if __name__ == "__main__":
    unittest.main()
