# Clean silkscreen artwork inside HAM footprints, in the library, then re-apply the footprint on every board.
#  - silk lines/arcs that hit a pad or hole, another part's silk, or the board edge are removed
#  - footprint texts that hit any of those move to the Fab layer
#  - --fork REF=Name (board-specific): first copy a stock footprint into HAM as Name so it can be edited
# Usage (KiCad python3): fix_fp_silk.py <pcb> [<pcb> ...] [--fork LM_v2/LM_v2.kicad_pcb:J2=DSUB-9_Male_Horizontal_BoardEdge]
import sys, os, math, pcbnew
mm, MM = pcbnew.FromMM, pcbnew.ToMM
HAM = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'lib', 'HAM.pretty')
IO = pcbnew.PCB_IO_KICAD_SEXPR()
SILK = (pcbnew.F_SilkS, pcbnew.B_SilkS); FAB = {pcbnew.F_SilkS: pcbnew.F_Fab, pcbnew.B_SilkS: pcbnew.B_Fab}
args = sys.argv[1:]; forks = []
if '--fork' in args:
    i = args.index('--fork'); forks = args[i + 1].split(','); args = args[:i] + args[i + 2:]
boards = {p: pcbnew.LoadBoard(p) for p in args}

def is_text(g): return 'TEXT' in g.GetClass()
def local(f, pt):
    t = pcbnew.PAD(f); t.SetPosition(pt); return t.GetFPRelativePosition()
def key(v): return (round(MM(v.x), 2), round(MM(v.y), 2))

def save_to_ham(f, name):
    pos, rot, ref = f.GetPosition(), f.GetOrientationDegrees(), f.GetReference()
    f.SetOrientationDegrees(0); f.SetPosition(pcbnew.VECTOR2I(0, 0)); f.SetReference('REF**')
    f.SetFPID(pcbnew.LIB_ID('HAM', name)); IO.FootprintSave(HAM, f)
    f.SetReference(ref); f.SetPosition(pos); f.SetOrientationDegrees(rot)

for spec in forks:  # e.g. LM_v2/LM_v2.kicad_pcb:J2=Name
    where, name = spec.split('=')
    pcb, ref = where.split(':')
    save_to_ham(boards[pcb].FindFootprintByReference(ref), name)
    print(f'  forked {pcb} {ref} -> HAM:{name}')

# 1. find offending silk items, as footprint-local centres per library footprint
drop, tofab = {}, {}
for pcb, b in boards.items():
    edges = [d for d in b.GetDrawings() if d.GetLayer() == pcbnew.Edge_Cuts]
    outline = pcbnew.SHAPE_POLY_SET(); b.GetBoardPolygonOutlines(outline, False)
    pads = [p for f in b.GetFootprints() for p in f.Pads()]
    for f in b.GetFootprints():
        if f.GetFPID().GetLibNickname().wx_str() != 'HAM':
            continue
        name = f.GetFPID().GetLibItemName().wx_str()
        others = [g for o in b.GetFootprints() if o.GetReference() != f.GetReference()
                  for g in list(o.GraphicalItems()) + [o.Reference()] if g.GetLayer() in SILK and (not hasattr(g, 'IsVisible') or g.IsVisible())]
        for g in f.GraphicalItems():
            if g.GetLayer() not in SILK:
                continue
            shp = g.GetEffectiveShape(g.GetLayer()); bb = g.GetBoundingBox()
            hit = False
            for p in pads:
                if not p.GetBoundingBox().Intersects(bb) and not bb.Intersects(p.GetBoundingBox()):
                    pb = p.GetBoundingBox(); pb.Inflate(mm(0.2))
                    if not pb.Intersects(bb): continue
                cu = pcbnew.F_Cu if p.IsOnLayer(pcbnew.F_Cu) else pcbnew.B_Cu
                if shp.Collide(p.GetEffectiveShape(cu), mm(0.1)) or (p.GetDrillSize().x and shp.Collide(p.GetEffectiveHoleShape(), mm(0.1))):
                    hit = True; break
            if not hit:
                hit = any(o.GetLayer() == g.GetLayer() and o.GetBoundingBox().Intersects(bb) and shp.Collide(o.GetEffectiveShape(o.GetLayer()), 0) for o in others)
            if not hit:
                corners = ((bb.GetLeft(), bb.GetTop()), (bb.GetRight(), bb.GetBottom()), (bb.GetLeft(), bb.GetBottom()), (bb.GetRight(), bb.GetTop()))
                hit = not all(outline.Contains(pcbnew.VECTOR2I(x, y)) for x, y in corners) or any(shp.Collide(e.GetEffectiveShape(), mm(0.5)) for e in edges)
            if hit:
                (tofab if is_text(g) else drop).setdefault(name, set()).add(key(local(f, g.GetBoundingBox().GetCenter())))

# 2. edit each library footprint once
for name in sorted(set(drop) | set(tofab)):
    lib = IO.FootprintLoad(HAM, name); nd = nf = 0
    for g in list(lib.GraphicalItems()):
        if g.GetLayer() not in SILK: continue
        k = key(g.GetBoundingBox().GetCenter())
        if k in drop.get(name, ()): lib.Remove(g); nd += 1
        elif k in tofab.get(name, ()): g.SetLayer(FAB[g.GetLayer()]); nf += 1
    lib.SetFPID(pcbnew.LIB_ID('HAM', name)); IO.FootprintSave(HAM, lib)
    print(f'  HAM:{name}: {nd} silk lines removed, {nf} texts to Fab')

# 3. re-apply the edited footprints on every board (pads must land exactly where they were)
edited = {n[:-10] for n in os.listdir(HAM) if n.endswith('.kicad_mod')}  # re-apply every HAM footprint so boards always match the library
for pcb, b in boards.items():
    todo = [f.GetReference() for f in b.GetFootprints()
            if f.GetFPID().GetLibNickname().wx_str() == 'HAM' and f.GetFPID().GetLibItemName().wx_str() in edited]
    for ref in todo:
        f = b.FindFootprintByReference(ref)
        name = f.GetFPID().GetLibItemName().wx_str()
        if f.GetFPID().GetLibNickname().wx_str() != 'HAM' or name not in edited:
            continue
        new = pcbnew.FootprintLoad(HAM, name); new.SetFPID(pcbnew.LIB_ID('HAM', name))  # fresh copy per placement
        if f.IsFlipped(): new.Flip(new.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
        new.SetOrientation(f.GetOrientation()); new.SetPosition(f.GetPosition())
        nums = [p.GetNumber() for p in f.Pads() if p.GetNumber()]
        uniq = [n for n in nums if nums.count(n) == 1]  # some modules repeat pad numbers; align on unique ones
        k = uniq[0]
        new.Move(f.FindPadByNumber(k).GetPosition() - new.FindPadByNumber(k).GetPosition())
        err = max(abs(new.FindPadByNumber(n).GetPosition().x - f.FindPadByNumber(n).GetPosition().x) + abs(new.FindPadByNumber(n).GetPosition().y - f.FindPadByNumber(n).GetPosition().y)
                  for n in uniq)
        assert err < 1000, (pcb, f.GetReference(), err)
        nets = {p.GetNumber(): p.GetNet() for p in f.Pads()}
        new.SetReference(f.GetReference()); new.SetValue(f.GetValue()); new.SetPath(f.GetPath())
        new.SetSheetname(f.GetSheetname()); new.SetSheetfile(f.GetSheetfile())
        for src, dst in ((f.Reference(), new.Reference()), (f.Value(), new.Value())):
            dst.SetLayer(src.GetLayer()); dst.SetVisible(src.IsVisible()); dst.SetTextSize(src.GetTextSize())
            dst.SetTextThickness(src.GetTextThickness()); dst.SetTextAngle(src.GetTextAngle()); dst.SetPosition(src.GetPosition()); dst.SetFont(None)
        for fl in f.GetFields():
            if not (fl.IsReference() or fl.IsValue()) and not new.HasField(fl.GetName()):
                new.SetField(fl.GetName(), fl.GetText())
        for fl in new.GetFields():
            if not (fl.IsReference() or fl.IsValue()):
                fl.SetVisible(False); fl.SetLayer(FAB.get(fl.GetLayer(), fl.GetLayer()))
        zc = {p.GetNumber(): p.GetLocalZoneConnection() for p in f.Pads()}
        new.SetField('Datasheet', f.GetFieldText('Datasheet')) if f.HasField('Datasheet') else None
        b.Add(new)
        for p in new.Pads():
            if p.GetNumber() in nets: p.SetNet(nets[p.GetNumber()])
            if p.GetNumber() in zc: p.SetLocalZoneConnection(zc[p.GetNumber()])
        b.Remove(f)
    pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(pcb)
print('done')
