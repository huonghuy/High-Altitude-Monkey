# Compact re-placement of LM_v2. Run with KiCad's bundled python3 from hardware/kicad/LM_v2.
# Clears tracks/vias/board zones, re-places footprints, redraws Edge.Cuts, adds GND pours on both layers.
import sys, pcbnew
args = [a for a in sys.argv[1:] if not a.startswith('--')]
PCB = args[0] if args else 'LM_v2.kicad_pcb'
EARS = '--no-ears' not in sys.argv  # --no-ears: plain 36x44 board, no mounting holes
X0, Y0 = 100.0, 100.0
W, H, R = 36.0, 44.0, 1.0

# ref: (x, y, rot_deg, side)  coordinates relative to board top-left
PLACE = {
    # top: ESP32 with antenna overhanging the top edge (edge = U1.y - 6.56, same as original)
    'U1': (25.4, 6.56, 0, 'F'),
    'SW1': (3.6, 4.7, 90, 'F'), 'SW2': (3.6, 14.2, 90, 'F'),
    'C6': (9.0, 2.4, 0, 'F'), 'C7': (13.5, 2.4, 0, 'F'),
    'C5': (9.0, 4.6, 0, 'F'), 'R5': (13.5, 4.6, 0, 'F'),
    'R6': (13.5, 6.8, 0, 'F'),
    'D1': (9.0, 11.6, 0, 'F'), 'R8': (13.5, 11.6, 0, 'F'),
    'D2': (9.0, 13.8, 0, 'F'), 'R9': (13.5, 13.8, 0, 'F'),
    'R7': (13.5, 17.4, 0, 'F'),
    # middle: USB-UART + auto-reset, RS-232 transceiver
    'U3': (6.5, 24.8, 90, 'F'), 'C3': (2.5, 29.7, 0, 'F'), 'C4': (6.5, 29.7, 0, 'F'),
    'Q1': (15.0, 22.6, 0, 'F'), 'Q2': (15.0, 26.8, 0, 'F'),
    'R3': (19.4, 21.9, 0, 'F'), 'R4': (19.4, 24.1, 0, 'F'),
    'U2': (30.0, 24.8, 90, 'F'), 'C12': (22.6, 21.9, 0, 'F'),
    'C8': (25.4, 30.2, 90, 'F'), 'C10': (27.4, 30.2, 90, 'F'),
    'C9': (29.4, 30.2, 90, 'F'), 'C11': (31.4, 30.2, 90, 'F'),
    # bottom: USB-C on left edge, buck, DE-9 on bottom edge (edge = pin1.y + 9.4)
    'J1': (3.67, 38.0, 270, 'F'),
    'F1': (12.7, 31.6, 180, 'F'), 'C1': (10.5, 34.6, 270, 'F'), 'U4': (13.3, 35.0, 0, 'F'),
    'L1': (17.4, 34.75, 0, 'F'), 'C2': (17.0, 39.2, 180, 'F'),
    'U5': (11.6, 39.2, 0, 'F'), 'R1': (10.6, 42.4, 0, 'F'), 'R2': (14.0, 42.4, 0, 'F'),
    'J2': (21.2, H - 9.4, 0, 'F'),
    # test points on the bottom side: no assembly needed
    'TP1': (22.5, 27.5, 0, 'B'), 'TP2': (22.5, 31.0, 0, 'B'), 'TP3': (8.0, 32.0, 0, 'B'),
}

mm = pcbnew.FromMM
b = pcbnew.LoadBoard(PCB)
edges = [d for d in b.GetDrawings() if d.GetLayer() == pcbnew.Edge_Cuts]
doomed = list(b.GetTracks()) + list(b.Zones()) + edges
for it in doomed:
    b.Remove(it)

for f in [f for f in b.GetFootprints() if f.GetReference().startswith('H')]:
    b.Remove(f)  # mounting holes are re-added below
refs = {f.GetReference(): f for f in b.GetFootprints()}
missing = set(refs) ^ set(PLACE)
assert not missing, f'placement table mismatch: {missing}'
for r, (x, y, rot, side) in PLACE.items():
    f = refs[r]
    if f.IsFlipped() != (side == 'B'):
        f.Flip(f.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
    f.SetPosition(pcbnew.VECTOR2I(mm(X0 + x), mm(Y0 + y)))
    f.SetOrientationDegrees(rot)
    for txt in (f.Reference(),):
        txt.SetTextSize(pcbnew.VECTOR2I(mm(0.8), mm(0.8)))
        txt.SetTextThickness(mm(0.12))

def seg(x1, y1, x2, y2):
    s = pcbnew.PCB_SHAPE(b, pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(pcbnew.VECTOR2I(mm(x1), mm(y1))); s.SetEnd(pcbnew.VECTOR2I(mm(x2), mm(y2)))
    s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(mm(0.05)); b.Add(s)

def arc(cx, cy, sx, sy, deg=90):
    a = pcbnew.PCB_SHAPE(b, pcbnew.SHAPE_T_ARC)
    a.SetCenter(pcbnew.VECTOR2I(mm(cx), mm(cy))); a.SetStart(pcbnew.VECTOR2I(mm(sx), mm(sy)))
    a.SetArcAngleAndEnd(pcbnew.EDA_ANGLE(deg, pcbnew.DEGREES_T), True)
    a.SetLayer(pcbnew.Edge_Cuts); a.SetWidth(mm(0.05)); b.Add(a)

x1, y1, x2, y2 = X0, Y0, X0 + W, Y0 + H
# M3 ears: 8 mm tabs with a rounded end, left beside U3 and right beside the DE-9 (diagonal, takes cable pull)
EL, ER, E = Y0 + 24.8, Y0 + 36.0, (4.0 if EARS else 0.0)
seg(x1 + R, y1, x2 - R, y1)
if not EARS:
    seg(x2, y1 + R, x2, y2 - R); seg(x2 - R, y2, x1 + R, y2); seg(x1, y2 - R, x1, y1 + R)
if EARS:
    seg(x2, y1 + R, x2, ER - E); seg(x2, ER - E, x2 + E, ER - E); arc(x2 + E, ER, x2 + E, ER - E, 180)
    seg(x2 + E, ER + E, x2, ER + E); seg(x2, ER + E, x2, y2 - R)
    seg(x2 - R, y2, x1 + R, y2)
    seg(x1, y2 - R, x1, EL + E); seg(x1, EL + E, x1 - E, EL + E); arc(x1 - E, EL, x1 - E, EL + E, 180)
    seg(x1 - E, EL - E, x1, EL - E); seg(x1, EL - E, x1, y1 + R)
lib = '/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints/MountingHole.pretty'
for ref, hx, hy in ((('H1', x1 - E, EL), ('H2', x2 + E, ER)) if EARS else ()):
    h = pcbnew.FootprintLoad(lib, 'MountingHole_3.2mm_M3')
    h.SetReference(ref); h.SetValue('M3'); h.SetPosition(pcbnew.VECTOR2I(mm(hx), mm(hy)))
    h.SetBoardOnly(True); h.SetExcludedFromBOM(True); h.SetExcludedFromPosFiles(True)
    h.Reference().SetVisible(False)
    b.Add(h)
arc(x2 - R, y1 + R, x2 - R, y1); arc(x2 - R, y2 - R, x2, y2 - R); arc(x1 + R, y2 - R, x1 + R, y2); arc(x1 + R, y1 + R, x1, y1 + R)

gnd = b.FindNet('/GND')
for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
    z = pcbnew.ZONE(b)
    z.SetLayer(layer); z.SetNet(gnd); z.SetZoneName('GND')
    o = z.Outline(); o.NewOutline()
    for px, py in ((x1 - 2 * E, y1), (x2 + 2 * E, y1), (x2 + 2 * E, y2), (x1 - 2 * E, y2)):
        o.Append(mm(px), mm(py))  # clipped to Edge.Cuts by the filler
    z.SetLocalClearance(mm(0.25)); z.SetMinThickness(mm(0.2))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetThermalReliefGap(mm(0.3)); z.SetThermalReliefSpokeWidth(mm(0.4))
    z.SetAssignedPriority(0)
    b.Add(z)

b.Save(PCB)
print('placed', len(PLACE), 'footprints;', 'M3 ears' if EARS else 'no ears', '; board', W, 'x', H, 'mm')
