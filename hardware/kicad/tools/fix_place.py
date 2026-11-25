# Placement fixes before re-routing (KiCad python3):
#  - LM2596 module standoff holes become part of HAM:LM2596_Module (separate H footprints removed)
#  - parts sitting inside another part's courtyard are nudged to the nearest free spot (groups move together)
#  - optional footprint swaps (e.g. Main SW1 -> 10 A JST VH header)
# Usage: fix_place.py <pcb> --move R10,R6,R4 [--group R1,R2,...] [--swap SW1=Lib:Name]
import sys, os, math, pcbnew
args = sys.argv[1:]; PCB = args[0]; opt = dict(zip(args[1::2], args[2::2]))
mm, MM = pcbnew.FromMM, pcbnew.ToMM
HAM = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'lib', 'HAM.pretty')
STD = '/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints'
b = pcbnew.LoadBoard(PCB)

def _unused(f, lx, ly):
    a = math.radians(f.GetOrientationDegrees())  # KiCad: +angle is CCW on screen (y down)
    return (f.GetPosition().x + mm(lx * math.cos(a) + ly * math.sin(a)), f.GetPosition().y + mm(-lx * math.sin(a) + ly * math.cos(a)))

# --- 1. module standoff holes into the LM2596 footprint ---
HOLES = [(5.30, 0.76), (35.02, 16.84)]  # mean of the six imported holes, footprint-local mm
lib = pcbnew.FootprintLoad(HAM, 'LM2596_Module')
if not any(p.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH for p in lib.Pads()):
    for lx, ly in HOLES:
        p = pcbnew.PAD(lib); p.SetAttribute(pcbnew.PAD_ATTRIB_NPTH); p.SetShape(pcbnew.PAD_SHAPE_CIRCLE)
        p.SetSize(pcbnew.VECTOR2I(mm(3.2), mm(3.2))); p.SetDrillSize(pcbnew.VECTOR2I(mm(3.2), mm(3.2)))
        p.SetLayerSet(pcbnew.PAD.UnplatedHoleMask())
        p.SetFPRelativePosition(pcbnew.VECTOR2I(mm(lx), mm(ly))); p.SetNumber('')
        lib.Add(p)
    lib.SetFPID(pcbnew.LIB_ID('HAM', 'LM2596_Module'))
    pcbnew.PCB_IO_KICAD_SEXPR().FootprintSave(HAM, lib)
    lib = pcbnew.FootprintLoad(HAM, 'LM2596_Module')
for f in [f for f in b.GetFootprints() if f.GetFPIDAsString() == 'HAM:LM2596_Module']:
    if any(p.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH for p in f.Pads()):
        continue
    new = pcbnew.FootprintLoad(HAM, 'LM2596_Module'); new.SetFPID(pcbnew.LIB_ID('HAM', 'LM2596_Module'))
    new.SetOrientation(f.GetOrientation()); new.SetPosition(f.GetPosition())
    targets = [p.GetPosition() for p in new.Pads() if p.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH]
    for h in [h for h in b.GetFootprints() if h.GetReference().startswith('H') and h.IsBoardOnly()]:
        dist = min(math.hypot(h.GetPosition().x - t.x, h.GetPosition().y - t.y) for t in targets)
        if dist < mm(0.6):
            print(f'  {h.GetReference()} -> part of {f.GetReference()} ({MM(dist):.2f} mm shift)'); b.Remove(h)
    nets = {p.GetNumber(): p.GetNet() for p in f.Pads()}
    new.SetReference(f.GetReference()); new.SetValue(f.GetValue()); new.SetPath(f.GetPath())
    new.SetSheetname(f.GetSheetname()); new.SetSheetfile(f.GetSheetfile())
    for fl in f.GetFields():
        if not new.HasField(fl.GetName()): new.SetField(fl.GetName(), fl.GetText())
    b.Add(new)
    for p in new.Pads():
        if p.GetNumber() in nets: p.SetNet(nets[p.GetNumber()])
    b.Remove(f)

# --- 2. footprint swaps ---
for spec in filter(None, opt.get('--swap', '').split(',')):
    ref, fpid = spec.split('=')
    old = b.FindFootprintByReference(ref); libn, name = fpid.split(':')
    new = pcbnew.FootprintLoad(os.path.join(STD, libn + '.pretty'), name); new.SetFPID(pcbnew.LIB_ID(libn, name))
    new.SetOrientation(old.GetOrientation()); new.SetPosition(old.GetPosition())
    # keep pad 1 where it was so the connector stays put
    d = old.FindPadByNumber('1').GetPosition() - new.FindPadByNumber('1').GetPosition(); new.Move(d)
    nets = {p.GetNumber(): p.GetNet() for p in old.Pads()}
    new.SetReference(ref); new.SetValue(old.GetValue()); new.SetPath(old.GetPath())
    new.SetSheetname(old.GetSheetname()); new.SetSheetfile(old.GetSheetfile())
    for fl in old.GetFields():
        if not new.HasField(fl.GetName()): new.SetField(fl.GetName(), fl.GetText())
    b.Add(new)
    for p in new.Pads(): p.SetNet(nets[p.GetNumber()])
    b.Remove(old); print(f'  {ref} -> {fpid}')

# --- 3. nudge parts out of other parts' courtyards ---
def crt(f):
    c = f.GetCourtyard(pcbnew.B_CrtYd if f.IsFlipped() else pcbnew.F_CrtYd)
    return c.BBox() if c.OutlineCount() else f.GetBoundingBox(False)
outline = pcbnew.SHAPE_POLY_SET(); b.GetBoardPolygonOutlines(outline, False)
MARGIN, GAP = mm(1.0), mm(0.25)

def free(box, movers):
    for x, y in ((box.GetLeft() - MARGIN, box.GetTop() - MARGIN), (box.GetRight() + MARGIN, box.GetTop() - MARGIN),
                 (box.GetLeft() - MARGIN, box.GetBottom() + MARGIN), (box.GetRight() + MARGIN, box.GetBottom() + MARGIN)):
        if not outline.Contains(pcbnew.VECTOR2I(int(x), int(y))):
            return False
    g = pcbnew.BOX2I(box.GetPosition(), box.GetSize()); g.Inflate(GAP)
    return not any(f not in movers and g.Intersects(crt(f)) for f in b.GetFootprints())

groups = [[r] for r in filter(None, opt.get('--move', '').split(','))]
if opt.get('--group'):
    groups.append(opt['--group'].split(','))
for grp in groups:
    fs = [b.FindFootprintByReference(r) for r in grp]
    box = crt(fs[0])
    for f in fs[1:]: box.Merge(crt(f))
    best = (0, 0) if free(box, fs) else None
    for r in range(1, 80 if best is None else 1):                       # 0.5 mm rings out to 40 mm
        for k in range(8 * r):
            a = 2 * math.pi * k / (8 * r); dx, dy = int(math.cos(a) * mm(0.5) * r), int(math.sin(a) * mm(0.5) * r)
            t = pcbnew.BOX2I(box.GetPosition() + pcbnew.VECTOR2I(dx, dy), box.GetSize())
            if free(t, fs):
                best = (dx, dy); break
        if best: break
    assert best, f'no free spot for {grp}'
    for f in fs: f.Move(pcbnew.VECTOR2I(*best))
    print(f'  moved {",".join(grp)} by ({MM(best[0]):.1f}, {MM(best[1]):.1f}) mm')
b.Save(PCB)
