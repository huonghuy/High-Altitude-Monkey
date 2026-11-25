# Hand-route the TPS62162 buck (U4) per TI SLVSAM2E sec. 11 layout guidance; tracks/vias are locked so Freerouting keeps them.
import sys, pcbnew
PCB = sys.argv[1]
mm = pcbnew.FromMM
b = pcbnew.LoadBoard(PCB)
pad = lambda r, n: b.FindFootprintByReference(r).FindPadByNumber(n).GetPosition()
P = lambda x, y: pcbnew.VECTOR2I(mm(x), mm(y))

def trk(net, pts, w):
    n = b.FindNet(net)
    for a, c in zip(pts, pts[1:]):
        t = pcbnew.PCB_TRACK(b); t.SetStart(a); t.SetEnd(c); t.SetWidth(mm(w))
        t.SetLayer(pcbnew.F_Cu); t.SetNet(n); t.SetLocked(True); b.Add(t)

def via(net, p):
    v = pcbnew.PCB_VIA(b); v.SetPosition(p); v.SetWidth(mm(0.6)); v.SetDrill(mm(0.3))
    v.SetNet(b.FindNet(net)); v.SetLocked(True); b.Add(v)

# Input: F1 -> C1 (input cap right at VIN/PGND) -> VIN pins 2/3, EN (pin 3) tied to VIN.
trk('/VBUS_5V', [pad('F1', '2'), P(110.5, 132.4), pad('C1', '1')], 0.5)
trk('/VBUS_5V', [pad('C1', '1'), P(111.7, 133.65)], 0.4)
trk('/VBUS_5V', [P(111.7, 133.65), P(111.7, 134.75), pad('U4', '2'), pad('U4', '3')], 0.25)
# Switch node: pin 7 -> L1, kept short (narrow at the 0.5 mm-pitch pins).
trk('/BUCK_SW', [pad('U4', '7'), pad('L1', '1')], 0.3)
# Output: L1 -> C2, and VOS sensed at the output cap (datasheet: "VOS ... shortest way to VOUT at the output capacitor").
trk('/+3V3', [pad('L1', '2'), P(118.58, 137.5), pad('C2', '1')], 0.6)
trk('/+3V3', [pad('U4', '6'), P(115.3, 135.25), P(115.3, 137.5), P(118.58, 137.5)], 0.2)
# Ground vias next to the input and output cap ground pads, into the bottom pour.
c1g, c2g = pad('C1', '2'), pad('C2', '2')
via('/GND', P(mm and pcbnew.ToMM(c1g.x), pcbnew.ToMM(c1g.y) + 1.0))
via('/GND', P(pcbnew.ToMM(c2g.x), pcbnew.ToMM(c2g.y) + 1.2))
b.Save(PCB)
print('buck prerouted')
