# Power budget -> trace widths (IPC-2221, external layer) -> KiCad net classes and board rules.
# Usage: python3 net_rules.py            (writes all four .kicad_pro files and prints the table)
# Change a current below and re-run; then re-route the board (route.py) so the new widths are used.
import json, os

COPPER_OZ = 1.0   # 35 um outer copper (JLCPCB standard)
DELTA_T = 10.0    # allowed temperature rise, degC (conservative for a sealed payload box)
MIN_POWER_W = 0.4 # practical floor for any power trace, mm
SIGNAL_W = 0.25   # default signal trace, mm

def ipc2221_width_mm(amps, oz=COPPER_OZ, dt=DELTA_T):
    """IPC-2221 outer layer: I = 0.048 * dT^0.44 * A^0.725 (A in mil^2)."""
    area_mil2 = (amps / (0.048 * dt ** 0.44)) ** (1 / 0.725)
    return area_mil2 / (1.378 * oz) * 0.0254

# Worst-case continuous currents. Assumptions (flag if wrong):
#  Main: battery ~12.8 V. Servos: 6x MG90S, 100-250 mA moving, ~700 mA stalled each (AZ-Delivery datasheet),
#        so 6 stalled = ~4.2 A, above the LM2596 module's 3 A; traces are sized to the module's 3 A limit.
#        Servo buck U4 therefore draws up to ~1.7 A from the battery,
#        Pololu 13 V video supply for the VM-70X (2.2 A out -> ~2.5 A in), logic buck U7 ~0.25 A in.
#        Battery input through P1/P2 -> SW1 -> VCC therefore carries ~5 A.
#  MC:   5 V logic: Mega + serial LCD + RS-232 modules + lamps, < 0.6 A; 1 A allowed.
#  VL:   78L05 (100 mA max) and 5 V for fan/camera < 0.5 A.
#  HC-05 Programmer: one HC-05 powered at a time, < 0.1 A.
BOARDS = {
    'Main/Main': [
        ('Batt_5A', 5.0, ['VCC', 'Net-(P2-+)', 'Net-(SW1-S)', 'GND']),
        ('Servo_3A', 3.0, ['VCC(6)']),
        ('Video_2.5A', 2.5, ['VCC(12)', 'Switch(12)']),
        ('Logic_1A', 1.0, ['5V(M)']),
    ],
    'MC/MC': [('Power_1A', 1.0, ['5V(M)', 'GND'])],
    'VL/VL': [('Power_0.5A', 0.5, ['/5V', '/VCC(12)', '/5V_DS', 'GND'])],
    'HC-05_Programmer/HC-05 Programmer': [('Power_0.5A', 0.5, ['+5V', 'GND', 'SlideIn', 'PlugIn', '/SlideIn', '/PlugIn'])],
}
RULES = {  # JLCPCB standard 2-layer capability with margin
    'min_clearance': 0.2, 'min_track_width': 0.2, 'min_via_diameter': 0.6, 'min_through_hole_diameter': 0.3,
    'min_via_annular_width': 0.15, 'min_hole_clearance': 0.25, 'min_hole_to_hole': 0.25, 'min_copper_edge_clearance': 0.5,
}

def via_for(amps):
    # ~1 A per 0.3 mm plated via barrel (25 um plating); bigger via for high-current classes
    return (0.6, 0.3) if amps <= 1 else (1.0, 0.6) if amps <= 3 else (1.2, 0.7)

HERE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
if __name__ == '__main__':
    print(f'IPC-2221 outer, {COPPER_OZ:g} oz, dT {DELTA_T:g} C')
    for board, classes in BOARDS.items():
        p = os.path.join(HERE, board + '.kicad_pro'); d = json.load(open(p))
        ns = d['net_settings']; dflt = next(c for c in ns['classes'] if c['name'] == 'Default')
        dflt.update(track_width=SIGNAL_W, clearance=0.2, via_diameter=0.6, via_drill=0.3)
        out, pats = [dflt], []
        for name, amps, nets in classes:
            w = max(MIN_POWER_W, round(ipc2221_width_mm(amps) + 0.049, 1))  # round up to 0.1 mm
            vd, vh = via_for(amps)
            out.append(dict(dflt, name=name, track_width=w, clearance=0.2, via_diameter=vd, via_drill=vh, priority=len(out) - 1))
            pats += [{'netclass': name, 'pattern': n} for n in nets]
            print(f'  {board.split("/")[0]:17s} {name:11s} {amps:4.1f} A  calc {ipc2221_width_mm(amps):.2f} mm -> {w:.1f} mm, via {vd}/{vh}  {", ".join(nets)}')
        ns['classes'] = out; ns['netclass_patterns'] = pats
        d['board']['design_settings']['rules'].update(RULES)
        json.dump(d, open(p, 'w'), indent=2)
