# LM_v2: Land Module (ESP32)

Bluetooth Classic ↔ RS-232 bridge between the comms payload and Mission Control, built on an ESP32-WROOM-32E. It replaces the old ATmega2560 + HC-05 LM. Firmware is in `firmware/LM_v2`.

## Projects

| Project | Board |
|---|---|
| `LM_v2` | 36 × 55.3 mm, plus two M3 mounting ears (52 mm across) |
| `LM_v2_NoMount` | Same board, 36 × 55.3 mm, no ears |

`LM_v2.kicad_sch` is the master. `LM_v2_NoMount.kicad_sch` is a copy, and the NoMount PCB is LM_v2's board with the ears and M3 holes removed. After a change, copy both over and re-derive NoMount.

## Design notes

- Power: USB-C VBUS → PTC fuse F1 → TPS62162 3.3 V buck (U4, 2.2 µH L1, 10 µF in, 22 µF out). The buck is routed to TI's layout guide (SLVSAM2E §11). VOS senses +3V3 at C2.
- USB-UART: CH340C with two-transistor auto-reset into EN and IO0. USBLC6-2 ESD on the data lines.
- RS-232: MAX3232 to J2, an Assmann A-DS 09 A/KG-T2S (`HAM:DSUB-9_Male_Horizontal_Assmann_A-DS-KG`, drawing ASS 4888 CO in `hardware/datasheets/`). It is centred on the bottom edge with its two boardlock holes (Ø3.05 mm, 24.99 mm apart); the board edge sits at the flange, 7.2 mm from the pin 6–9 row.
- The ESP32 antenna overhangs the top edge. Keep metal away from it in the enclosure.
- Trace widths: 0.4 mm power class (VBUS, +3V3, GND), 0.2 mm signals, GND pours on both layers with stitching vias.

## Checks

Run from `hardware/kicad`:

```
python3 tools/check_netlist.py LM_v2/LM_v2.kicad_sch LM_v2/pin_nets.csv   # every pin's net vs the design table
kicad-cli sch erc LM_v2/LM_v2.kicad_sch
kicad-cli pcb drc --schematic-parity LM_v2/LM_v2.kicad_pcb
```

All three were clean on 2026-11-25. If you change a net on purpose, update `pin_nets.csv` as well.

## Connection to Mission Control

J2 plugs into MC's Telemetry Processor port (`Serial2`, a female DE-9 described in the firmware as "PC, for telemetry Processor"), either directly or through the existing 6 ft male-to-female cable. J2 is a male DE-9 wired like a PC: pin 2 = LM receive, pin 3 = LM transmit, pin 5 = GND. This matches the ground-station setup in `docs/` (the grey box next to the Command Processor).

## Parts

Every part has a manufacturer and part number in the schematic (`Manufacturer`, `MPN` fields). The boards are hand-assembled. U4 (TPS62162, WSON) and the ESP32's ground pad both need hot air or the hotplate. TP1–TP3 are bare pads and are not on the BOM.

## Open before ordering

Fab packages (Gerbers, drill, BOM, assembly PDFs) are in `hardware/gerbers/LM_v2/` and `hardware/gerbers/LM_v2_NoMount/`.

- **J2 footprint:** drawn from the Assmann drawing, reading its "7.2 mm footprint" as board edge to the pin 6–9 row. Check it against a real connector before a large order.
- **Bluetooth pairing:** the firmware waits as a slave, so the payload HC-05 has to be set as master and bound to the LM's address. See `firmware/LM_v2/README.md`.
- **Board-level details:** R2's reference label is on the Fab layer only, because the silkscreen had no room for it.
