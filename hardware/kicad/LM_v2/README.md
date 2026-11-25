# LM_v2: Land Module (ESP32)

Bluetooth Classic ↔ RS-232 bridge between the comms payload and Mission Control, built on an ESP32-WROOM-32E. It replaces the old ATmega2560 + HC-05 LM. Firmware is in `firmware/LM_v2`.

## Projects

| Project | Board |
|---|---|
| `LM_v2` | 36 × 44 mm, plus two M3 mounting ears (52 mm across) |
| `LM_v2_NoMount` | Same board, 36 × 44 mm, no ears |

`LM_v2.kicad_sch` is the master. `LM_v2_NoMount.kicad_sch` is a copy, so after a schematic change, copy it over and update the NoMount PCB.

## Design notes

- Power: USB-C VBUS → PTC fuse F1 → TPS62162 3.3 V buck (U4, 2.2 µH L1, 10 µF in, 22 µF out). The buck is routed to TI's layout guide (SLVSAM2E §11). VOS senses +3V3 at C2.
- USB-UART: CH340C with two-transistor auto-reset into EN and IO0. USBLC6-2 ESD on the data lines.
- RS-232: MAX3232 to J2 (DE-9, `HAM:DSUB-9_Male_Horizontal_BoardEdge`, overhangs the bottom edge).
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

## Open before ordering

- J2: the DE-9 is still provisional. Confirm the connector, gender and pinout against Mission Control.
- F1: the PTC part is TBD (0.75 A hold).
- No part numbers on the BOM yet.
- The firmware has never been compiled or tested.
- R2's reference label is on the Fab layer only, because the silkscreen had no room for it.
