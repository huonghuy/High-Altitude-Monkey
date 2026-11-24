# Known hardware issues

Found while reviewing the EasyEDA designs (Nov 2026) before moving them to KiCad.
None of these have been fixed yet. The KiCad projects in `hardware/kicad/` are
straight imports of the EasyEDA files and carry the same issues.

## Main board (V2.6 – V2.8)

| # | Issue | Where | Fix |
|---|-------|-------|-----|
| 1 | The 5 V buck output (`VCC(5)`) feeds the Mega 2560 Pro **VIN** pin. VIN goes through the module's LDO, which needs 6–9 V, so the Mega's 5 V rail sags to about 4 V. That rail also powers the HC-05 and the PCA9685 logic. Likely cause of resets and Bluetooth dropouts. | U1 VIN | Feed `VCC(5)` into a Mega **5V** pin (the REV 2.5 perfboard does this), or set that buck to about 7.5 V into VIN. |
| 2 | LM135 temperature sensors wired backwards: sensor + is tied straight to the supply, and R1/R2 feed A0/A1 from the supply. The sensor sits across the rail with no current limit, and A0/A1 read pinned at full scale. | CN2/CN3, R1/R2, R7/R8 | `5V → R (1.5k) → node(A0, LM135 +)`, LM135 − to GND, cal pot across +/− with the wiper on ADJ. |
| 3 | The A2 divider (10k / 3.9k, which scales about 17.8 V down to 5 V) is on `VCC(5)`, so it always reads about 1.4 V. The ratio suggests it was meant to monitor the battery. | R6/R4 | Move the top of the divider to `VCC` (battery), if battery monitoring is the intent. |
| 4 | The "12 V" rail is an LM2596 buck running off a ~12 V battery. A buck can't regulate 12 V from 12 V, and the VM-70X needs 12–13.5 V at up to 2.2 A. | U6 | Higher-voltage battery, a buck-boost, or power the VM-70X directly if the battery stays in its range. |
| 5 | V2.6 only: IRF9540 drain and source both on `VCC(12)`, and a PNP 2N3905 used as a low-side driver. Fixed in V2.7/V2.8 (2N3904, separate `Switch(12)` net). | U8/U9 | Already fixed in V2.7+. |
| 6 | V2.7: the LM2679 section is placed but not routed, so nothing drives `VCC(5)`; also, the part is the fixed **12 V** version. | U9 (V2.7) | Use LM2679-5.0 or -ADJ and route it, or drop it. |
| 7 | V2.8: bare ATmega2560 (U9) and ADP7105 LDOs (U10/U11) are unfinished: no supply, crystal, reset or ISP. U9's footprint sits under the Mega module. | U9–U11 (V2.8) | Finish the bare-chip design or remove those parts. Leave them unpopulated until then. |

## VideoLynx (VL) board (V1.4, V1.5)

| # | Issue | Fix |
|---|-------|-----|
| 1 | The DS1809 digital pot's VCC comes from `VCC(12)` on CN1 pin 4. The DS1809 runs on 2.7–5.5 V (abs. max 7 V), so it is probably dead, and RF power control via D24/D26 won't work. Its UC/DC pull-ups also push about 12 V back into the Mega pins. | Power the DS1809 from 5 V. |
| 2 | Schematic and PCB disagree: the PCB has U2/U3 connectors, the schematic has CN3/CN4. | Pick one and sync with Update PCB from Schematic. |
