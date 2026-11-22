# Supplier queries to send before paying

Short messages to copy into IndiaMART, WhatsApp or e-mail. Each one closes a risk noted in the BOM (`bom/sources.yaml`).

## 1. Sharvi Technologies, Bengaluru (A7670C-LANS breakout, IndiaMART, Rs 975)

> Hello, we want 2 pieces (later about 10) of your A7670C-LANS SIMCom 4G/LTE Cat 1 breakout board.
> Please confirm before we order:
> 1. Is the board powered directly at VBAT 3.4-4.2 V, with **no voltage regulator** (no LM39302 or similar) on the board?
> 2. Please send a clear photo of the pin labels. We need VBAT, GND, TXD, RXD, PWRKEY and, if available, VDD_EXT (1.8 V).
> 3. Is the UART at the module's 1.8 V level, or does the board shift it to 3.3 V?
> 4. Antenna connector type (u.FL/IPEX or SMA), and is an antenna included?
> 5. Module firmware version if known (AT+SIMCOMATI), price for 2 and for 10, GST invoice, and dispatch time.

Why: the design feeds the modem straight from the battery (D-054). A board with a regulator would leave the modem
below 3.4 V on battery. The answers also set jumper JP3 (1.8 V or 3.3 V logic) and the J10 harness wiring.

## 2. DNA Technology (CSN-A2 58 mm thermal printer)

> Hello, we want 2 pieces of the CSN-A2 2-inch micro panel thermal printer, **TTL interface** version.
> Two pages on your site show different prices (Rs 2,076.80 with stock 4, and Rs 3,186 with stock 0). Which is current, and is it in stock?
> Please confirm the TTL connector has a **TXD (status) pin**, the default baud rate (9600 or 19200), the supply range
> (5-9 V), and whether paper rolls are included.

Why: paper-out detection needs the printer's TXD line (D-047); the firmware tries 9600 then 19200 baud (D-041).

## 3. Robocraze (7Semi TPS61023 5 V boost module, Rs 79)

> Hello, is the 7Semi TPS61023 boost converter module in stock (2 pieces, later 10)? Which inductor value does it use,
> and is the output fixed at 5 V?

Why: the current margin in D-062 was worked out from TI's datasheet with the inductor ripple ignored; the
inductor value confirms the real margin.

## 4. DigiKey India (Keystone 1043 holders, SanDisk/Samsung cards, Jacob PG7 gland)

No query needed; check stock in the cart. Order **one extra Keystone 1043** for the footprint fit check (bench item B-20)
before the PCBs are ordered.
