# Phase 6 enclosure check

| Check | Result | Detail |
|---|---|---|
| No interference (printed parts vs contents, contents vs contents) | PASS | 5 printed parts, 125 reference bodies |
| Lid inside face to tallest part >= 2 mm | PASS | 5.0 mm (tallest: part:BT1) |
| Floor modules to PCB underside (minus 2 mm leads) >= 2 mm | PASS | 12.4 mm |
| Printer body to PCB top edge >= 2 mm | PASS | 12.5 mm |
| Printer panel thickness within the printer's 1-6 mm range | PASS | 3.0 mm (CSN-A2 manual s3) |
| Openings line up with LEDs, buzzer, OLED, buttons, DC jack | PASS | all open/coaxial |
| OLED window covers the assumed active area (29.4 x 14.7) with margin | PASS | window 31.0 x 17.0 mm |
| Every printed part fits a 256 mm bed | PASS | base 210 x 247 x 61 mm |
| Walls, floor, lid >= 2.0 mm | PASS | 2.5/3.0/3.0 mm |

**ALL PASS**

Assumed dimensions (verify on the parts):

* modem_breakout: 45.0 x 45.0 x 16.0 mm, ASSUMED A7670C breakout incl. SIM + u.FL
* buck_xl4015: 51.5 x 26.5 x 14.0 mm, ASSUMED common XL4015 module
* boost_module: 40.0 x 30.0 x 12.0 mm, ASSUMED 5 V boost module
* OLED module 35.4 x 33.5 mm, hole pattern 31.4 x 29.5 mm (typical, not from a datasheet)
* DC jack axis 6.5 mm above the PCB
* part heights: mechanical/cadquery/board_features.py (each with its basis)
