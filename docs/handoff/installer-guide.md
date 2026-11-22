# Installer guide

## Before the visit
* Device assembled and tested (bringup.md), firmware flashed, owner PIN on the label inside the lid.
* SIM active (Airtel or Vi), SMS tested. Charged cells fitted.
* Print one farmer card in the farmer's language (`docs/handoff/farmer-card.pdf`, A5, one page per language).

## Server settings (once per device, before mounting)

The device uploads readings to an MQTT server over TLS (D-060). Each device gets its own login.
Copy two files to the root of the microSD card, insert it, and power the device once:

`seri.cfg` (plain text, one setting per line; lines starting with `#` are ignored):

```
mqtt_host=your-broker.example.com
mqtt_port=8883
mqtt_user=shed-017
mqtt_pass=the-device-password
client_id=seri-017
```

`ca.pem`: the server's CA certificate, as given by the MQTT provider (PEM text, up to 4 KB).

At start-up the device checks both files, saves them, and **deletes them from the card** so the password
does not stay on a removable card. The event log on the card shows `CFG mqtt imported host=...` (the password
is never written). If a setting is wrong, the files stay on the card and the log says why, for example
`CFG mqtt rejected: unknown key: mqtt_prot`. Until a server is set, readings are kept on the card (30 days)
and sent later. SMS and printing never depend on the server.

## Mounting
1. Mount on an inside wall of the rearing house, 1.5-1.8 m high, away from the heater and the door,
   out of direct sun and spray. Two screws for the keyholes, one at the bottom.
2. Run the probe cable through the gland and hang the probe at the height of the rearing trays,
   in free air (not touching walls, trays or wet gunny bags), at least 0.5 m from the heater.
3. Plug in the 12 V adapter. The OLED shows the readings within a minute; the green light comes on.

## Set-up by SMS (installer codes)
| Code | Use |
|---|---|
| `10 <PIN>` | From the farmer's phone: makes it the owner (PIN on the label). Do this first. |
| `12 <1-4>` | Farmer's SMS language: 1 English, 2 Hindi, 3 Telugu, 4 Kannada |
| `11 <1-6>` | Current stage (see the card) |
| `21 <number>` | Add a second / third number (from the owner phone) |
| `13 <t> <r>` | Tolerance: t in tenths of °C (5-30), r in % RH (2-15). Default ±1.0 °C, ±5 %. Example `13 10 5` |
| `30 <ssid> <password>` | Wi-Fi (optional; 4G works without it) |
| `20` | Status reply: check that SMS works both ways |

The same settings are available on the device with the three buttons (middle = OK) in the farmer's language.

## Hand-over to the farmer
Show the card: `20` for readings, `11 N` to change the stage, green/red light, press the middle button to stop
the beeping. Tell them the printer prints every alert and a daily summary at 06:00, and how to change the paper roll
(57 mm, max 39 mm roll).
