# Usage: route.py export <pcb> <dsn>    -> strip zones+tracks, export DSN
#        route.py import <pcb> <ses>    -> import SES, add GND pours on both layers (solid SMD / thermal THT), stitch, fill, save
import sys, pcbnew
mode, PCB, F = sys.argv[1:4]
mm = pcbnew.FromMM
b = pcbnew.LoadBoard(PCB)

if mode == 'export':
    for it in [t for t in b.GetTracks() if not t.IsLocked()] + list(b.Zones()):
        b.Remove(it)
    b.Save(PCB)
    print('export', pcbnew.ExportSpecctraDSN(b, F))
    sys.exit()

print('import', pcbnew.ImportSpecctraSES(b, F))
bb = b.GetBoardEdgesBoundingBox()
x1, y1, x2, y2 = bb.GetLeft(), bb.GetTop(), bb.GetRight(), bb.GetBottom()
gnd = b.FindNet('/GND')
for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
    z = pcbnew.ZONE(b)
    z.SetLayer(layer); z.SetNet(gnd); z.SetZoneName('GND')
    o = z.Outline(); o.NewOutline()
    for px, py in ((x1, y1), (x2, y1), (x2, y2), (x1, y2)):
        o.Append(px, py)
    z.SetLocalClearance(mm(0.25)); z.SetMinThickness(mm(0.2))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THT_THERMAL)  # solid to SMD pads, relief on through-hole
    z.SetThermalReliefGap(mm(0.3)); z.SetThermalReliefSpokeWidth(mm(0.4))
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    b.Add(z)
filler = pcbnew.ZONE_FILLER(b)
filler.Fill(b.Zones())

# Stitching vias on a 3 mm grid wherever both pours have solid copper with margin, away from the antenna edge.
STEP, VD, VH, KEEP = 3.0, 0.6, 0.3, 0.9
fz = [z for z in b.Zones() if z.GetLayer() == pcbnew.F_Cu][0]
bz = [z for z in b.Zones() if z.GetLayer() == pcbnew.B_Cu][0]
def solid(z, x, y):
    fp = z.GetFilledPolysList(z.GetLayer())
    for dx, dy in ((0, 0), (KEEP, 0), (-KEEP, 0), (0, KEEP), (0, -KEEP)):
        if not fp.Contains(pcbnew.VECTOR2I(x + mm(dx), y + mm(dy))):
            return False
    return True
import math
others = [t for t in b.GetTracks() if t.GetNetname() != '/GND']
pads = [p for f in b.GetFootprints() for p in f.Pads() if p.GetNetname() != '/GND']
holes = [(p.GetPosition().x, p.GetPosition().y, p.GetDrillSize().x // 2 + mm(VH / 2 + 0.3))
         for f in b.GetFootprints() for p in f.Pads() if p.GetDrillSize().x > 0]
holes += [(v.GetPosition().x, v.GetPosition().y, v.GetDrill() // 2 + mm(VH / 2 + 0.3))
          for v in b.GetTracks() if v.GetClass() == 'PCB_VIA']
def clear_of_others(x, y, need=mm(0.3 + 0.25)):
    pt = pcbnew.VECTOR2I(x, y)
    for t in others:
        if t.HitTest(pt, need + t.GetWidth() // 2):
            return False
    for p in pads:
        if p.HitTest(pt, need):
            return False
    for h in holes:  # hole-to-hole rule applies to every net, including GND
        if (h[0] - x) ** 2 + (h[1] - y) ** 2 < h[2] ** 2:
            return False
    return True
n = 0
y = y1 + mm(3.0)  # skip the top strip next to the antenna
while y < y2 - mm(1.0):
    x = x1 + mm(1.5)
    while x < x2 - mm(1.0):
        if solid(fz, x, y) and solid(bz, x, y) and clear_of_others(x, y):
            v = pcbnew.PCB_VIA(b); v.SetPosition(pcbnew.VECTOR2I(x, y))
            v.SetWidth(mm(VD)); v.SetDrill(mm(VH)); v.SetNet(gnd); b.Add(v); n += 1
        x += mm(STEP)
    y += mm(STEP)
filler.Fill(b.Zones())
b.Save(PCB)
print('stitch vias', n, 'tracks+vias', len(b.GetTracks()))
