"""Checks the carrier netlist against the firmware pin map and basic design rules
(python3 -m unittest hardware/kicad/gen/test_carrier.py)."""
import os
import re
import sys
import unittest

sys.path.insert(0, os.path.dirname(__file__))
import carrier  # noqa: E402

BOARD_H = os.path.join(os.path.dirname(__file__), "..", "..", "..", "firmware", "include", "board.h")

# board.h constant -> carrier net on that ESP32 pin
EXPECT = {"kI2cSda": "I2C_SDA", "kI2cScl": "I2C_SCL", "kModemTx": "MDM_TX_3V3", "kModemRx": "MDM_RX_3V3",
          "kModemPwrKey": "MDM_PWRKEY_CTL", "kPrinterTx": "PRN_RXD", "kPrinterRx": "PRN_TXD",
          "kPrinterPowerEn": "PRN_PWR", "kSdSck": "SD_SCK", "kSdMiso": "SD_MISO", "kSdMosi": "SD_MOSI",
          "kSdCs": "SD_CS", "kSdCardDetect": "SD_CD", "kBtnUp": "BTN_UP", "kBtnOk": "BTN_OK",
          "kBtnDown": "BTN_DOWN", "kBuzzer": "BUZZER_CTL", "kLedGreen": "LED_G", "kLedRed": "LED_R",
          "kMainsOk": "MAINS_SENSE", "kBattSense": "BATT_SENSE", "kDs18b20": "ONEWIRE", "kRelay": "RELAY_CTL"}


def board_pins():
    text = open(BOARD_H).read()
    pins = {}
    for m in re.finditer(r"constexpr int ([^;]+);", text):
        for a in m.group(1).split(","):
            name, val = (x.strip() for x in a.split("="))
            pins[name] = int(val)
    return pins


def gpio_nets():
    out = {}
    for gpios, nets in ((carrier.GPIO_J2, carrier.J2), (carrier.GPIO_J3, carrier.J3)):
        for g, n in zip(gpios, nets):
            if isinstance(g, int):
                out[g] = n
    return out


class CarrierTests(unittest.TestCase):
    def test_every_firmware_pin_lands_on_the_right_net(self):
        pins, nets = board_pins(), gpio_nets()
        for const, net in EXPECT.items():
            self.assertEqual(net, nets.get(pins[const]), f"{const} = GPIO{pins[const]}")

    def test_unused_gpios_are_not_connected(self):
        used = {board_pins()[c] for c in EXPECT}
        for g, n in gpio_nets().items():
            if g not in used:
                self.assertEqual("NC", n, f"GPIO{g} carries {n} but board.h does not use it")

    def test_flash_and_uart0_pins_left_free(self):
        nets = gpio_nets()
        for g in (6, 7, 8, 9, 10, 11, 1, 3, 0):
            self.assertEqual("NC", nets[g], f"GPIO{g}")

    def test_input_only_pins_only_carry_inputs(self):
        nets = gpio_nets()
        for g in (34, 35, 36, 39):
            self.assertIn(nets[g], {"SD_CD", "MAINS_SENSE", "BATT_SENSE", "PRN_TXD"})

    def test_every_net_joins_at_least_two_pins(self):
        sch = carrier.build(os.path.join(os.path.dirname(__file__), "..", "carrier", "carrier.kicad_sch"))
        lonely = {n: p for n, p in sch.nets.items() if len(p) < 2}
        self.assertEqual({}, lonely)

    def test_two_cells_fitted_four_footprints(self):
        cells = [p for p in carrier.PARTS if p[2].startswith("BT") and p[3] == "18650"]
        self.assertEqual(4, len(cells))
        self.assertEqual(2, sum(1 for p in cells if p[6].get("dnp") != "yes"))


if __name__ == "__main__":
    unittest.main()
