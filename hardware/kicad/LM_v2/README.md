# LM V2 revision 0.2 — electrical and placement update (DRAFT)

This is a patch for the existing `hardware/kicad/LM_v2` project, not a new repository or fabrication-ready PCB. The original LM, firmware, project configuration and existing PCB are not overwritten by the ZIP. The existing 35 footprint instance IDs are retained. Four new footprints (L1 and three test points) and revised U4/C1/C2 footprint assignments must be imported by native KiCad.

## Changes
- U4: TPS62162DSGR, fixed 3.3 V synchronous buck, 1 A nominal output capability. PGND/AGND/FB/EP grounded, EN connected to fused VBUS, VOS to output, SW to L1, PG unconnected.
- L1: 2.2 uH shielded Coilcraft XFL4020-222ME family. C1: 10 uF 10 V X7R input; C2: 22 uF 10 V X7R output. Verify exact purchased part DC bias, tolerance, inductor saturation and RMS rating against TI's requirements.
- TP1/TP2/TP3: 3.3 V, GND, fused VBUS. USB-C shield pad SH and DE-9 Pins footprint corrections retained.
- `place_revision.py` proposes revised locations for 23 parts after synchronization; adds one unfilled B.Cu GND zone. It checks all pad numbers and named nets before changing the PCB, refuses an already routed board and backs up the original. It does not create tracks/vias or adjust the RF keepout. It assumes the previously placed PCB has the corrected outline and ESP32 rule area.
- `validate_schematic.py` runs footprint checker, native ERC and pin-level comparison of the XML netlist against the design table; it stores reports outside the repo. Not run against native KiCad here.

## Apply
Close KiCad. Back up the current LM_v2 directory outside the repository. Extract this ZIP into the repository root. From `hardware/kicad/LM_v2` run `python3 validate_schematic.py`. Correct any validation failures before continuing. Open the schematic, press F8 and update the PCB (allow footprint replacements); preserve the existing outline and placements, save, and close KiCad. Run `python3 place_revision.py`. Inspect and refill the zone using B, save, and run native DRC. Do not fabricate or connect to live Mission Control equipment.

## Remaining
No complete copper routing is supplied. USB differential routing requires the actual stackup; buck hot-loop layout must follow TI's reference, including exposed-pad grounding and thermal design. Finish ground stitching, all other routing, silk cleanup, mechanical checks, and native DRC/parity. Verify DE-9 pinout, gender, shell bonding, cable and voltage levels. USB current allowance/inrush and protection require verification before operation with a computer; passive CC resistors are not a current negotiation controller. Confirm radio current/thermal behavior and test at mission temperatures. Firmware is unchanged and still lacks full legacy telemetry parsing.

References: https://www.ti.com/lit/ds/symlink/tps62160.pdf ; https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/index.html ; https://docs.kicad.org/10.0/en/cli/cli.html

## Checks actually performed
Schematic and symbol-library S-expression structure; unique pin records; old 35 instance UUIDs match existing PCB paths; source-level buck pin mapping against TI's datasheet. Python script syntax. No native KiCad ERC/DRC, footprint resolution of new parts, actual placement execution or hardware testing in this environment.
