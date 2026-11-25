# Clean up silkscreen so DRC passes (KiCad python3):  fix_silk.py <pcb>
#  - Value fields and footprint texts smaller than MIN_H go to the Fab layer (still visible in assembly drawings)
#  - all silk text >= MIN_H tall, stroke >= MIN_T
#  - reference designators re-placed at the nearest spot clear of pads, other silk, holes and the board edge;
#    if nothing fits they move to Fab and are reported
#  - footprint silk lines that leave the board (overhanging connectors) are dropped
import sys, pcbnew
PCB = sys.argv[1]
mm, MM = pcbnew.FromMM, pcbnew.ToMM
MIN_H, MIN_T = 0.8, 0.12
PAD_CLR, SILK_CLR, EDGE_CLR = mm(0.15), mm(0.1), mm(0.5)
b = pcbnew.LoadBoard(PCB)
FAB = {pcbnew.F_SilkS: pcbnew.F_Fab, pcbnew.B_SilkS: pcbnew.B_Fab}
SILK = set(FAB)
edges = [d for d in b.GetDrawings() if d.GetLayer() == pcbnew.Edge_Cuts]
outline = pcbnew.SHAPE_POLY_SET(); b.GetBoardPolygonOutlines(outline, False)

def size(t, h):
    t.SetFont(None)  # KiCad stroke font everywhere on silk (EasyEDA imports carry an unavailable CJK TrueType font)
    t.SetTextSize(pcbnew.VECTOR2I(mm(h), mm(h))); t.SetTextThickness(mm(max(MIN_T, h * 0.15)))

moved_fab, dropped = 0, 0
for f in b.GetFootprints():
    for fl in f.GetFields():  # extra symbol fields (MPN, LCSC, ...) are data, not silkscreen
        if not (fl.IsReference() or fl.IsValue()) and fl.GetLayer() in SILK:
            fl.SetVisible(False); fl.SetLayer(FAB[fl.GetLayer()])
    v = f.Value()
    if v.GetLayer() in SILK:
        v.SetLayer(FAB[v.GetLayer()]); moved_fab += 1
    for g in list(f.GraphicalItems()):
        if g.GetLayer() not in SILK:
            continue
        if g.GetClass() in ('PCB_TEXT', 'MTEXT') or 'TEXT' in g.GetClass():
            g.SetFont(None)  # EasyEDA TrueType text -> KiCad stroke font (DRC cannot check TrueType silk reliably)
            if MM(g.GetTextHeight()) < MIN_H - 1e-6:
                g.SetLayer(FAB[g.GetLayer()]); moved_fab += 1
            elif g.GetTextThickness() < mm(MIN_T):
                g.SetTextThickness(mm(MIN_T))
            continue
        # silk graphics that leave the board never print cleanly; only edit our own (HAM) footprints,
        # editing a stock footprint would make it differ from the KiCad library
        if f.GetFPID().GetLibNickname().wx_str() != 'HAM':
            continue
        shp = g.GetEffectiveShape(g.GetLayer())
        bb = g.GetBoundingBox()
        inside = all(outline.Contains(pcbnew.VECTOR2I(x, y)) for x, y in
                     ((bb.GetLeft(), bb.GetTop()), (bb.GetRight(), bb.GetBottom()), (bb.GetLeft(), bb.GetBottom()), (bb.GetRight(), bb.GetTop())))
        if not inside or any(shp.Collide(e.GetEffectiveShape(), EDGE_CLR) for e in edges):
            f.Remove(g); dropped += 1
board_texts = []
for d in b.GetDrawings():
    if d.GetLayer() in SILK and 'TEXT' in d.GetClass():
        d.SetFont(None)  # EasyEDA TrueType labels fail KiCad's stroke-weight check; use the KiCad stroke font
        size(d, max(MIN_H, MM(d.GetTextHeight())))
        board_texts.append(d)

# Obstacles for reference placement
def side_layers(f):
    if f is None: return (pcbnew.F_SilkS, pcbnew.F_Mask, pcbnew.F_Cu)
    if isinstance(f, int): return (pcbnew.B_SilkS, pcbnew.B_Mask, pcbnew.B_Cu)
    return (pcbnew.B_SilkS, pcbnew.B_Mask, pcbnew.B_Cu) if f.IsFlipped() else (pcbnew.F_SilkS, pcbnew.F_Mask, pcbnew.F_Cu)
pads = [p for f in b.GetFootprints() for p in f.Pads()]
silk_items = {pcbnew.F_SilkS: [], pcbnew.B_SilkS: []}
for f in b.GetFootprints():
    for g in f.GraphicalItems():
        if g.GetLayer() in SILK: silk_items[g.GetLayer()].append(g)
for d in b.GetDrawings():
    if d.GetLayer() in SILK: silk_items[d.GetLayer()].append(d)

def near(a, o, m):
    ba, bo = a.GetBoundingBox(), o.GetBoundingBox()
    return not (ba.GetRight() + m < bo.GetLeft() or bo.GetRight() + m < ba.GetLeft() or ba.GetBottom() + m < bo.GetTop() or bo.GetBottom() + m < ba.GetTop())

def clear(t, owner):
    silk, mask, cu = side_layers(owner)
    shp = t.GetEffectiveShape(silk)
    bb = t.GetBoundingBox()
    for x, y in ((bb.GetLeft(), bb.GetTop()), (bb.GetRight(), bb.GetBottom()), (bb.GetLeft(), bb.GetBottom()), (bb.GetRight(), bb.GetTop())):
        if not outline.Contains(pcbnew.VECTOR2I(x, y)): return False
    for e in edges:
        if near(t, e, EDGE_CLR) and shp.Collide(e.GetEffectiveShape(), EDGE_CLR): return False
    for p in pads:
        if (p.IsOnLayer(mask) or p.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH) and near(t, p, PAD_CLR):
            if shp.Collide(p.GetEffectiveShape(cu if p.IsOnLayer(cu) else mask), PAD_CLR) or \
               (p.GetDrillSize().x and shp.Collide(p.GetEffectiveHoleShape(), PAD_CLR)):
                return False
    for v in [v for v in b.GetTracks() if v.GetClass() == 'PCB_VIA']:
        if near(t, v, PAD_CLR) and shp.Collide(v.GetEffectiveHoleShape(), PAD_CLR): return False
    for g in silk_items[silk]:
        if g.m_Uuid.AsString() != t.m_Uuid.AsString() and near(t, g, SILK_CLR) and shp.Collide(g.GetEffectiveShape(silk), SILK_CLR): return False
    return True

def candidates(f, t):
    c = f.GetCourtyard(pcbnew.B_CrtYd if f.IsFlipped() else pcbnew.F_CrtYd)
    box = c.BBox() if c.OutlineCount() else f.GetBoundingBox(False)
    cx, cy = box.GetCenter().x, box.GetCenter().y
    yield (cx, cy, 0); yield (cx, cy, 90)  # inside the body (big modules have free space there)
    for gap in (0.3, 1.0, 2.0, 3.5):
        g = mm(gap)
        for ang in (0, 90):
            t.SetTextAngleDegrees(ang); t.SetPosition(pcbnew.VECTOR2I(cx, cy))
            tb = t.GetBoundingBox(); hw, hh = tb.GetWidth() // 2, tb.GetHeight() // 2
            for frac in (0, -0.3, 0.3, -0.45, 0.45):
                yield (cx + int(box.GetWidth() * frac), box.GetTop() - g - hh, ang)
                yield (cx + int(box.GetWidth() * frac), box.GetBottom() + g + hh, ang)
                yield (box.GetRight() + g + hw, cy + int(box.GetHeight() * frac), ang)
                yield (box.GetLeft() - g - hw, cy + int(box.GetHeight() * frac), ang)

placed, fabbed = 0, []
fps = sorted(b.GetFootprints(), key=lambda f: -f.GetBoundingBox(False).GetArea())  # big parts first
for f in fps:
    t = f.Reference()
    if not t.IsVisible() or t.GetLayer() not in SILK:
        continue
    size(t, max(MIN_H, min(MM(t.GetTextHeight()), 1.2)))
    t.SetKeepUpright(True)
    start = (t.GetPosition(), t.GetTextAngleDegrees())
    if clear(t, f):
        placed += 1; silk_items[t.GetLayer()].append(t); continue
    ok = False
    for x, y, a in candidates(f, t):
        t.SetTextAngleDegrees(a); t.SetPosition(pcbnew.VECTOR2I(int(x), int(y)))
        if clear(t, f):
            ok = True; break
    if ok:
        placed += 1; silk_items[t.GetLayer()].append(t)
    else:
        t.SetPosition(start[0]); t.SetTextAngleDegrees(start[1]); t.SetLayer(FAB[t.GetLayer()]); fabbed.append(f.GetReference())
# Board-level labels: keep them where the designer put them if clear, else nudge up to 4 mm
nudged, label_fab = 0, []
for d in board_texts:
    owner = None if d.GetLayer() == pcbnew.F_SilkS else 1
    if clear(d, owner):
        continue
    p0, ok = d.GetPosition(), False
    for r in range(1, 17):
        for k in range(8 * r):
            import math
            a = 2 * math.pi * k / (8 * r)
            d.SetPosition(pcbnew.VECTOR2I(p0.x + int(math.cos(a) * mm(0.25) * r), p0.y + int(math.sin(a) * mm(0.25) * r)))
            if clear(d, owner):
                ok = True; break
        if ok: break
    if ok:
        nudged += 1
    else:
        d.SetPosition(p0); d.SetLayer(FAB[d.GetLayer()]); label_fab.append(d.GetText())
b.Save(PCB)
print(f'  labels: {len(board_texts)} total, {nudged} nudged' + (f', no room -> Fab: {label_fab}' if label_fab else ''))
print(f'{PCB}: {moved_fab} texts to Fab, {dropped} off-board silk lines removed, {placed} references placed on silk',
      f'| no room, ref on Fab: {",".join(fabbed)}' if fabbed else '')
