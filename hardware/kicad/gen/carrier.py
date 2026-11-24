"""Seri-tool carrier board, version 1: the netlist, written as Python.

Every part is listed with its value, footprint and the net on every pin. Running this file writes
hardware/kicad/carrier/carrier.kicad_sch, which is then checked by KiCad ERC (tools/phase4_gate.sh).
The PCB is built from the same netlist (pcbgen.py), so schematic and board cannot drift apart.

Pin assignments must match firmware/include/board.h (checked by test_carrier.py).
Design notes and datasheet pages: docs/hardware/power-architecture.md, docs/datasheets/facts.md.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from schgen import Library, Schematic  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "carrier", "carrier.kicad_sch")

# ---- footprints (hand-solder friendly: 0805 with long pads, SOT-23, SOIC, SMA, THT) ----
R0805 = "Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder"
C0805 = "Capacitor_SMD:C_0805_2012Metric_Pad1.18x1.45mm_HandSolder"
C1206 = "Capacitor_SMD:C_1206_3216Metric_Pad1.33x1.80mm_HandSolder"
SOT23 = "Package_TO_SOT_SMD:SOT-23"
SOT23_5 = "Package_TO_SOT_SMD:SOT-23-5"
SOT23_6 = "Package_TO_SOT_SMD:SOT-23-6"
SMA = "Diode_SMD:D_SMA_Handsoldering"
SOD123 = "Diode_SMD:D_SOD-123"
LED3 = "LED_THT:LED_D3.0mm"
HDR = "Connector_PinHeader_2.54mm:PinHeader_1x{n:02d}_P2.54mm_Vertical"
SKT = "Connector_PinSocket_2.54mm:PinSocket_1x{n:02d}_P2.54mm_Vertical"
XH = "Connector_JST:JST_XH_B{n}B-XH-A_1x{n:02d}_P2.50mm_Vertical"
CELL = "seri:BatteryHolder_Keystone_1043_1x18650"   # product-owner decision 2026-11-22 (THT, genuine Keystone)

PARTS = []  # (lib, symbol, ref, value, footprint, nets, fields)


def part(lib, sym, ref, value, fp, nets, **fields):
    PARTS.append((lib, sym, ref, value, fp, {str(k): v for k, v in nets.items()}, fields))


def R(ref, value, a, b, **f):
    part("Device", "R", ref, value, R0805, {1: a, 2: b}, **f)


def C(ref, value, a, b, fp=C0805, **f):
    part("Device", "C", ref, value, fp, {1: a, 2: b}, **f)


def CP(ref, value, plus, minus, fp, **f):
    part("Device", "C_Polarized", ref, value, fp, {1: plus, 2: minus}, **f)


def NPN(ref, b, c, e="GND"):
    part("Transistor_BJT", "MMBT3904", ref, "MMBT3904", SOT23, {1: b, 2: e, 3: c})


def PFET(ref, g, s, d):
    part("Transistor_FET", "AO3401A", ref, "AO3401A", SOT23, {1: g, 2: s, 3: d})


def conn(ref, value, nets, fp):
    n = len(nets)
    part("Connector_Generic", f"Conn_01x{n:02d}", ref, value, fp.format(n=n), {i + 1: v for i, v in enumerate(nets)})


# =============================== 1. 12 V input =================================================
part("Connector", "Barrel_Jack_Switch", "J1", "12V 3A DC jack", "Connector_BarrelJack:BarrelJack_Horizontal",
     {1: "VIN_RAW", 2: "GND", 3: "NC"})
part("Device", "Polyfuse", "F1", "1.5A hold", "Fuse:Fuse_1812_4532Metric_Pad1.30x3.40mm_HandSolder",
     {1: "VIN_RAW", 2: "VIN_F"})
part("Diode", "SS34", "D1", "SS34", SMA, {2: "VIN_F", 1: "VIN_12V"})            # reverse-polarity block
part("Diode", "SMAJ15A", "D2", "SMAJ15A", SMA, {1: "VIN_12V", 2: "GND"})         # surge clamp
CP("C1", "100uF 25V", "VIN_12V", "GND", "Capacitor_THT:CP_Radial_D8.0mm_P3.50mm")
C("C2", "100nF", "VIN_12V", "GND")
R("R1", "100k", "VIN_12V", "MAINS_SENSE")                                       # 12.6 V -> 2.27 V
R("R2", "22k", "MAINS_SENSE", "GND")
C("C3", "100nF", "MAINS_SENSE", "GND")
# 12 V -> 5 V buck module (>= 3 A; part chosen in phase 5, D-048): wired by its four pads
conn("J4", "BUCK IN", ["VIN_12V", "GND"], HDR)
conn("J5", "BUCK OUT 5V", ["+5V_BUS", "GND"], HDR)
C("C4", "22uF 10V", "+5V_BUS", "GND", C1206)

# =============================== 2. Charger, protection, cells ===============================
part("seri", "TP4056", "U2", "TP4056", "Package_SO:SOIC-8-1EP_3.9x4.9mm_P1.27mm_EP2.41x3.3mm",
     {4: "+5V_BUS", 8: "+5V_BUS", 1: "CHG_TEMP", 2: "CHG_PROG", 5: "BAT_P", 7: "CHG_LED_K", 6: "NC", 3: "GND", 9: "GND"})
R("R3", "2.2k", "CHG_PROG", "GND", note="500 mA charge (TP4056 p5: 425-575 mA), D-048")
C("C5", "10uF", "+5V_BUS", "GND", C1206)
C("C6", "10uF", "BAT_P", "GND", C1206)
part("Jumper", "SolderJumper_3_Bridged12", "JP1", "TEMP: GND / NTC", "Jumper:SolderJumper-3_P1.3mm_Bridged12_RoundedPad1.0x1.5mm",
     {1: "GND", 2: "CHG_TEMP", 3: "NTC_DIV"}, in_bom="no")                                    # default: check off (D-048)
R("R4", "10k", "+5V_BUS", "NTC_DIV", dnp="yes")
part("Device", "Thermistor_NTC", "TH1", "10k B3950", XH.format(n=2), {1: "NTC_DIV", 2: "GND"}, dnp="yes")
part("Device", "LED", "D3", "red: charging", LED3, {2: "CHG_LED_A", 1: "CHG_LED_K"})
R("R5", "1k", "+5V_BUS", "CHG_LED_A")
# One-cell protection on the negative side: cell- (CELL_N) -> 3x FS8205A -> board GND (D-049)
part("Battery_Management", "DW01A", "U3", "DW01A", SOT23_6,
     {5: "PROT_VCC", 6: "CELL_N", 1: "PROT_OD", 3: "PROT_OC", 2: "PROT_CS", 4: "NC"})
R("R6", "100", "BAT_P", "PROT_VCC")
C("C7", "100nF", "PROT_VCC", "CELL_N")
R("R7", "1k", "GND", "PROT_CS")
for q in ("Q1", "Q2", "Q11"):                                                    # three in parallel: 23 mOhm (SPICE)
    part("seri", "FS8205A", q, "FS8205A", "Package_SO:TSSOP-8_4.4x3mm_P0.65mm",
         {4: "PROT_OD", 2: "CELL_N", 3: "CELL_N", 5: "PROT_OC", 6: "GND", 7: "GND", 1: "PROT_D", 8: "PROT_D"})
for i, ref in enumerate(("BT1", "BT2", "BT3", "BT4")):                            # 2 fitted, footprints for 4
    extra = {} if i < 2 else {"dnp": "yes"}
    part("Device", "Battery_Cell", ref, "18650", CELL, {1: "BAT_P", 2: "CELL_N"}, **extra)

# =============================== 3. Power path, rails ========================================
# Mains: +5V_BUS -> D4 -> VSYS. Battery: BAT_P -> Q3 (P-FET, on when +5V_BUS is absent) -> VSYS.
part("Diode", "SS34", "D4", "SS34", SMA, {2: "+5V_BUS", 1: "VSYS"})
PFET("Q3", g="+5V_BUS", s="VSYS", d="BAT_P")
PFET("Q12", g="+5V_BUS", s="VSYS", d="BAT_P")                                    # parallel with Q3: 42 mOhm
R("R8", "100k", "+5V_BUS", "GND")                                                # Q3 gate pulled low on a cut
CP("C8", "100uF 10V", "VSYS", "GND", "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm")
# Modem on the battery rail (brief): bulk >= 300 uF + ceramics close to the module (A7670 HD p26-27)
CP("C9", "1000uF 6.3V low-ESR", "BAT_P", "GND", "Capacitor_THT:CP_Radial_D10.0mm_P5.00mm")   # 2 x 1000 uF:
CP("C26", "1000uF 6.3V low-ESR", "BAT_P", "GND", "Capacitor_THT:CP_Radial_D10.0mm_P5.00mm")  # SPICE margin
C("C10", "22uF", "BAT_P", "GND", C1206)
C("C11", "22uF", "BAT_P", "GND", C1206)
C("C12", "1uF", "BAT_P", "GND")
C("C13", "100nF", "BAT_P", "GND")
C("C14", "33pF", "BAT_P", "GND")
C("C15", "10pF", "BAT_P", "GND")
# 3.3 V for the ESP32 and peripherals (AP2112K: 600 mA, p8)
part("Regulator_Linear", "AP2112K-3.3", "U4", "AP2112K-3.3", SOT23_5,
     {1: "VSYS", 3: "VSYS", 2: "GND", 4: "NC", 5: "+3V3"})
C("C16", "4.7uF", "VSYS", "GND")
C("C17", "10uF", "+3V3", "GND", C1206)
part("Jumper", "Jumper_2_Open", "JP2", "3V3 to DevKitC: REMOVE before USB", HDR.format(n=2),
     {1: "+3V3", 2: "ESP_3V3"})                                                  # D-043
# Battery and mains sensing
R("R9", "100k", "BAT_P", "BATT_SENSE")
R("R10", "100k", "BATT_SENSE", "GND")
C("C18", "100nF", "BATT_SENSE", "GND")

# =============================== 4. Printer power (switched boost) ===========================
PFET("Q4", g="PRN_SW_G", s="VSYS", d="BOOST_IN")                                 # D-046
R("R11", "100k", "VSYS", "PRN_SW_G")                                             # off by default
NPN("Q5", b="PRN_PWR_B", c="PRN_SW_G")
R("R12", "4.7k", "PRN_PWR", "PRN_PWR_B")
R("R13", "100k", "PRN_PWR_B", "GND")                                             # off while the ESP32 boots
CP("C19", "100uF 10V", "BOOST_IN", "GND", "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm")
conn("J6", "BOOST IN", ["BOOST_IN", "GND"], HDR)                                 # 5 V >= 2 A boost module
conn("J7", "BOOST OUT 5V", ["+5V_PRN", "GND"], HDR)
CP("C20", "470uF 10V", "+5V_PRN", "GND", "Capacitor_THT:CP_Radial_D8.0mm_P3.50mm")
conn("J8", "PRINTER POWER", ["+5V_PRN", "GND"], XH)
R("R14", "1k", "PRN_TXD", "PRN_TXD_EXT")                                         # printer TX -> GPIO39
conn("J9", "PRINTER TTL", ["PRN_RXD", "PRN_TXD_EXT", "GND"], XH)                 # RX in / TX out / GND

# =============================== 5. Modem (A7670C breakout) ==================================
# Harness header (any breakout pin order: wired to suit). VREF = module's VDD_1V8 (HD p18).
conn("J10", "A7670C HARNESS", ["BAT_P", "BAT_P", "GND", "GND", "MDM_TXD", "MDM_RXD", "MDM_PWRKEY", "MDM_VDD_EXT"], HDR)
# breakout mounted on the enclosure base, 8-wire harness <= 6 cm, 24 AWG (D-057)
part("Jumper", "SolderJumper_3_Bridged12", "JP3", "VREF: 1V8 / 3V3", "Jumper:SolderJumper-3_P1.3mm_Bridged12_RoundedPad1.0x1.5mm",
     {1: "MDM_VDD_EXT", 2: "MDM_VREF", 3: "+3V3"}, in_bom="no")                               # 3V3 for breakouts that shift already
# ESP32 TX (3.3 V) -> module RXD (1.8 V): common-base NPN, as the HD's MMBT3904 circuit (p33-34)
NPN("Q6", b="MDM_VREF_B1", c="MDM_RXD", e="MDM_TX_3V3")
R("R15", "4.7k", "MDM_VREF", "MDM_VREF_B1")
R("R16", "10k", "MDM_VREF", "MDM_RXD")
# module TXD (1.8 V) -> ESP32 RX (3.3 V)
NPN("Q7", b="MDM_VREF_B2", c="MDM_RX_3V3", e="MDM_TXD")
R("R17", "4.7k", "MDM_VREF", "MDM_VREF_B2")
R("R18", "10k", "+3V3", "MDM_RX_3V3")
R("R19", "10k", "+3V3", "MDM_TX_3V3")
# PWRKEY: open-collector NPN, GPIO27 high = press (board::kPwrKeyActiveHigh)
NPN("Q8", b="PWRKEY_B", c="MDM_PWRKEY")
R("R20", "4.7k", "MDM_PWRKEY_CTL", "PWRKEY_B")
R("R21", "100k", "PWRKEY_B", "GND")

# =============================== 6. ESP32-DevKitC V4 sockets =================================
# Espressif DevKitC V4 header pinout, J2 (left) and J3 (right), pin 1 at the antenna end.
J2 = ["ESP_3V3", "NC", "BATT_SENSE", "PRN_TXD", "SD_CD", "MAINS_SENSE", "BTN_UP", "BTN_OK", "PRN_RXD", "PRN_PWR",
      "MDM_PWRKEY_CTL", "BTN_DOWN", "RELAY_CTL", "GND", "BUZZER_CTL", "NC", "NC", "NC", "NC"]
J3 = ["GND", "SD_MOSI", "I2C_SCL", "NC", "NC", "I2C_SDA", "GND", "SD_MISO", "SD_SCK", "SD_CS", "MDM_TX_3V3",
      "MDM_RX_3V3", "LED_R", "NC", "LED_G", "ONEWIRE", "NC", "NC", "NC"]
GPIO_J2 = ["3V3", "EN", 36, 39, 34, 35, 32, 33, 25, 26, 27, 14, 12, "GND", 13, 9, 10, 11, "5V"]
GPIO_J3 = ["GND", 23, 22, 1, 3, 21, "GND", 19, 18, 5, 17, 16, 4, 0, 2, 15, 8, 7, 6]
conn("J2", "DevKitC J2 (left)", J2, SKT)
conn("J3", "DevKitC J3 (right)", J3, SKT)

# =============================== 7. Sensors, clock, storage ==================================
R("R22", "2.2k", "+3V3", "I2C_SDA")                                             # 2 m cable: rise time < 1 us (D-052)
R("R23", "2.2k", "+3V3", "I2C_SCL")
R("R24", "100", "I2C_SDA", "PROBE_SDA")                                          # 2 m cable: series R for ESD
R("R25", "100", "I2C_SCL", "PROBE_SCL")
conn("J11", "SHT31 PROBE (2 m)", ["+3V3", "PROBE_SDA", "PROBE_SCL", "GND"], XH)
C("C21", "100nF", "+3V3", "GND")
conn("J12", "DS18B20 (option)", ["+3V3", "ONEWIRE", "GND"], XH)
R("R26", "4.7k", "+3V3", "ONEWIRE")                                              # GPIO15 high at boot: default
part("seri", "DS3231SN", "U5", "DS3231SN", "Package_SO:SOIC-16W_7.5x10.3mm_P1.27mm",
     {2: "+3V3", 14: "RTC_VBAT", 15: "I2C_SDA", 16: "I2C_SCL", 4: "NC", 1: "NC", 3: "NC", 13: "GND",
      **{n: "GND" for n in range(5, 13)}})
C("C22", "100nF", "+3V3", "GND")
C("C23", "100nF", "RTC_VBAT", "GND")
part("Device", "Battery_Cell", "BT5", "CR2032", "Battery:BatteryHolder_Keystone_3002_1x2032", {1: "RTC_VBAT", 2: "GND"})
part("Connector", "Micro_SD_Card_Det_Hirose_DM3AT", "J13", "microSD", "Connector_Card:microSD_HC_Hirose_DM3AT-SF-PEJM5",
     {1: "SD_DAT2", 2: "SD_CS", 3: "SD_MOSI", 4: "+3V3", 5: "SD_SCK", 6: "GND", 7: "SD_MISO", 8: "SD_DAT1",
      9: "SD_CD", 10: "GND", 11: "GND"})                                        # DET switch closes to GND
for ref, net in (("R27", "SD_CS"), ("R28", "SD_MISO"), ("R29", "SD_DAT1"), ("R30", "SD_DAT2"), ("R31", "SD_CD")):
    R(ref, "10k", "+3V3", net)                                                   # GPIO5 high at boot: default
C("C24", "10uF", "+3V3", "GND", C1206)
C("C25", "100nF", "+3V3", "GND")

# =============================== 8. User interface ===========================================
conn("J14", "OLED cable (GND VCC SCL SDA)", ["GND", "+3V3", "I2C_SCL", "I2C_SDA"], HDR)    # OLED on the lid, 4-wire cable (D-056)
for ref, net in (("SW1", "BTN_UP"), ("SW2", "BTN_OK"), ("SW3", "BTN_DOWN")):
    part("Switch", "SW_Push", ref, net.split("_")[1], "Button_Switch_THT:SW_PUSH_6mm_H13mm", {1: net, 2: "GND"})
part("Device", "LED", "D5", "green", LED3, {2: "LED_G_A", 1: "GND"})
R("R32", "680", "LED_G", "LED_G_A")                                              # GPIO2: also keeps it low at boot
part("Device", "LED", "D6", "red", LED3, {2: "LED_R_A", 1: "GND"})
R("R33", "680", "LED_R", "LED_R_A")
part("Device", "Buzzer", "BZ1", "active 5V", "Buzzer_Beeper:Buzzer_12x9.5RM7.6", {1: "VSYS", 2: "BUZZER_K"})
part("Diode", "1N4148W", "D7", "1N4148W", SOD123, {2: "BUZZER_K", 1: "VSYS"})
NPN("Q9", b="BUZZER_B", c="BUZZER_K")
R("R34", "4.7k", "BUZZER_CTL", "BUZZER_B")
R("R35", "100k", "BUZZER_B", "GND")

# =============================== 9. Relay header (future control) ===========================
NPN("Q10", b="RELAY_B", c="RELAY_DRV")
R("R36", "4.7k", "RELAY_CTL", "RELAY_B")
R("R37", "10k", "RELAY_B", "GND")                                                # GPIO12 must be low at boot
R("R38", "10k", "RELAY_CTL", "GND")
conn("J15", "RELAY (VSYS, IN, GND)", ["VSYS", "RELAY_DRV", "GND"], XH)

# =============================== 10. Mechanical, power flags =================================
for i in range(1, 5):
    part("Mechanical", "MountingHole", f"H{i}", "M3", "MountingHole:MountingHole_3.2mm_M3", {}, in_bom="no")
for i, net in enumerate(("GND", "+5V_BUS", "VSYS", "CELL_N", "PROT_VCC", "RTC_VBAT", "ESP_3V3")):
    part("power", "PWR_FLAG", f"#FLG{i + 1:02d}", "PWR_FLAG", "", {1: net})


def build(path=OUT):
    lib = Library([os.path.join(HERE, "..", "lib")])
    s = Schematic("carrier", "Seri-tool carrier v1", lib)
    x, y, row_h = 30.0, 40.0, 0.0
    for libname, sym, ref, value, fp, nets, fields in PARTS:
        pins = Library.pins(lib.symbol(libname, sym)) if (libname, sym) != ("Mechanical", "MountingHole") else {}
        xs = [p[0] for p in pins.values()] or [0]
        ys = [p[1] for p in pins.values()] or [0]
        w, h = max(xs) - min(xs) + 45, max(ys) - min(ys) + 20
        if x + w > 570:
            x, y, row_h = 30.0, y + row_h, 0.0
        gx = round((x - min(xs) + 12) / 2.54) * 2.54
        gy = round((y + max(ys) + 8) / 2.54) * 2.54
        s.add(libname, sym, ref, value, (gx, gy), nets, fp, fields)
        x += w
        row_h = max(row_h, h)
    s.save(path)
    return s


if __name__ == "__main__":
    sch = build()
    print(f"{len([p for p in PARTS if not p[2].startswith('#')])} parts, {len(sch.nets)} nets -> {os.path.relpath(OUT)}")
