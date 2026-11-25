# Re-route a board with Freerouting, then widen power traces to their net-class width and pour/stitch GND.
# Usage (KiCad python3):
#   route.py export <pcb> <dsn> [--cap MM]   strip unlocked tracks + zones, export DSN with power widths capped at MM
#   route.py import <pcb> <ses> [--step MM]  import routes, widen power traces, GND pours on both layers, stitch vias
# Between the two: java -jar freerouting.jar -de <dsn> -do <ses> -mp 200 --gui.enabled=false
import sys, json, os, pcbnew

args = [a for a in sys.argv[1:]]
mode, PCB, F = args[:3]
opt = dict(zip(args[3::2], args[4::2]))
mm, MM = pcbnew.FromMM, pcbnew.ToMM
PRO = os.path.splitext(PCB)[0] + '.kicad_pro'

if mode == 'export':
    cap = float(opt.get('--cap', 1.0))
    d = json.load(open(PRO)); saved = json.dumps(d)
    for c in d['net_settings']['classes']:
        c['track_width'] = min(c['track_width'], cap)  # route narrow, widen afterwards
    json.dump(d, open(PRO, 'w'), indent=2)
    try:
        b = pcbnew.LoadBoard(PCB)
        for it in [t for t in b.GetTracks() if not t.IsLocked()] + list(b.Zones()):
            if not (hasattr(it, 'GetIsRuleArea') and it.GetIsRuleArea()):
                b.Remove(it)
        b.Save(PCB)
        print('export', pcbnew.ExportSpecctraDSN(b, F))
    finally:
        open(PRO, 'w').write(json.dumps(json.loads(saved), indent=2))
    sys.exit()

b = pcbnew.LoadBoard(PCB)
print('import', pcbnew.ImportSpecctraSES(b, F))
ds = b.GetDesignSettings()

# --- widen power traces to their class width where clearance allows (neck down only near tight pads) ---
classes = {c['name']: c for c in json.load(open(PRO))['net_settings']['classes']}
edge = [d for d in b.GetDrawings() if d.GetLayer() == pcbnew.Edge_Cuts]
EDGE_CLR = mm(0.5)
HOLE_CLR = mm(0.25)
npth = [p for f in b.GetFootprints() for p in f.Pads() if p.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH]
copper = []
for f in b.GetFootprints():
    copper += [(p, p.GetNetCode()) for p in f.Pads()]
copper += [(t, t.GetNetCode()) for t in b.GetTracks()]

def bbox_near(a, b_, m):
    ba, bb = a.GetBoundingBox(), b_.GetBoundingBox()
    return not (ba.GetRight() + m < bb.GetLeft() or bb.GetRight() + m < ba.GetLeft() or
                ba.GetBottom() + m < bb.GetTop() or bb.GetBottom() + m < ba.GetTop())

def fits(t, clr):
    shp = t.GetEffectiveShape(t.GetLayer())
    for o, nc in copper:
        if nc == t.GetNetCode() or o is t or not o.IsOnLayer(t.GetLayer()) or not bbox_near(t, o, clr):
            continue
        if shp.Collide(o.GetEffectiveShape(t.GetLayer()), clr):
            return False
    for h in npth:  # non-plated holes have no copper layer but still need hole clearance
        if bbox_near(t, h, HOLE_CLR) and shp.Collide(h.GetEffectiveHoleShape(), HOLE_CLR):
            return False
    for e in edge:
        if bbox_near(t, e, EDGE_CLR) and shp.Collide(e.GetEffectiveShape(), EDGE_CLR):
            return False
    return True

short = {}
for t in b.GetTracks():
    if t.GetClass() != 'PCB_TRACK':
        continue
    name = t.GetNetClassName().split(',')[0]
    c = classes.get(name)
    if not c or name == 'Default':
        continue
    want, have = mm(c['track_width']), t.GetWidth()
    if have >= want:
        continue
    clr = mm(c.get('clearance', 0.2))
    w = want
    while w > have:
        t.SetWidth(w)
        if fits(t, clr):
            break
        w -= mm(0.1)
    if w <= have:
        t.SetWidth(have)
    if t.GetWidth() < want:
        short.setdefault(t.GetNetname(), []).append((MM(t.GetWidth()), MM(t.GetLength())))
for n, segs in sorted(short.items()):
    tot = sum(l for _, l in segs)
    print(f'  neck-down {n}: {len(segs)} segs, {tot:.1f} mm total, narrowest {min(w for w, _ in segs):.2f} mm')

# --- GND pours on both layers + stitching vias ---
gname = next(n for n in ('GND', '/GND') if b.FindNet(n))
gnd = b.FindNet(gname)
bb = b.GetBoardEdgesBoundingBox()
x1, y1, x2, y2 = bb.GetLeft(), bb.GetTop(), bb.GetRight(), bb.GetBottom()
for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
    z = pcbnew.ZONE(b); z.SetLayer(layer); z.SetNet(gnd); z.SetZoneName('GND')
    o = z.Outline(); o.NewOutline()
    for px, py in ((x1, y1), (x2, y1), (x2, y2), (x1, y2)):
        o.Append(px, py)
    z.SetLocalClearance(mm(0.3)); z.SetMinThickness(mm(0.25))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetThermalReliefGap(mm(0.4)); z.SetThermalReliefSpokeWidth(mm(0.6))
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    b.Add(z)
filler = pcbnew.ZONE_FILLER(b); filler.Fill(b.Zones())

STEP, VD, VH, KEEP = float(opt.get('--step', 5.0)), 0.6, 0.3, 0.9
fz = [z for z in b.Zones() if z.GetLayer() == pcbnew.F_Cu and z.GetNetname() == gname][0]
bz = [z for z in b.Zones() if z.GetLayer() == pcbnew.B_Cu and z.GetNetname() == gname][0]
def solid(z, x, y):
    fp = z.GetFilledPolysList(z.GetLayer())
    return all(fp.Contains(pcbnew.VECTOR2I(x + mm(dx), y + mm(dy))) for dx, dy in ((0, 0), (KEEP, 0), (-KEEP, 0), (0, KEEP), (0, -KEEP)))
holes = [(p.GetPosition().x, p.GetPosition().y, max(p.GetDrillSize().x, p.GetDrillSize().y) // 2 + mm(VH / 2 + 0.3))
         for f in b.GetFootprints() for p in f.Pads() if p.GetDrillSize().x > 0]
holes += [(v.GetPosition().x, v.GetPosition().y, v.GetDrill() // 2 + mm(VH / 2 + 0.3)) for v in b.GetTracks() if v.GetClass() == 'PCB_VIA']
n = 0
y = y1 + mm(STEP / 2)
while y < y2:
    x = x1 + mm(STEP / 2)
    while x < x2:
        if solid(fz, x, y) and solid(bz, x, y) and all((hx - x) ** 2 + (hy - y) ** 2 >= r ** 2 for hx, hy, r in holes):
            v = pcbnew.PCB_VIA(b); v.SetPosition(pcbnew.VECTOR2I(x, y)); v.SetWidth(mm(VD)); v.SetDrill(mm(VH)); v.SetNet(gnd); b.Add(v); n += 1
        x += mm(STEP)
    y += mm(STEP)
filler.Fill(b.Zones())
b.Save(PCB)
print('stitch vias', n)
