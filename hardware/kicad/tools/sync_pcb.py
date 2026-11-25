# Update a PCB from its schematic netlist (what "Update PCB from Schematic" does for nets, values and fields),
# but refuse to merge or split copper: a PCB net is renamed only if all its pads agree on one schematic net.
# Usage (KiCad python3): sync_pcb.py <pcb> <netlist.xml>
import sys, collections
import xml.etree.ElementTree as ET
import pcbnew

PCB, NET = sys.argv[1:3]
nl = ET.parse(NET).getroot()
want = {}  # (ref, pad) -> schematic net
for n in nl.iter('net'):
    for nd in n.iter('node'):
        want[(nd.get('ref'), nd.get('pin'))] = n.get('name')
comps = {}
for c in nl.iter('comp'):
    fields = {f.get('name'): (f.text or '') for f in c.iter('field')}
    fields['Value'] = c.findtext('value') or ''
    fields['Footprint'] = c.findtext('footprint') or ''
    comps[c.get('ref')] = fields

b = pcbnew.LoadBoard(PCB)
fps = [f for f in b.GetFootprints() if not f.IsBoardOnly()]
# Which schematic nets does each PCB net map to?
votes = collections.defaultdict(set)
for f in fps:
    for p in f.Pads():
        key = (f.GetReference(), p.GetNumber())
        if key in want and p.GetNetname():
            votes[p.GetNetname()].add(want[key])
conflicts = {o: n for o, n in votes.items() if len(n) > 1}
for o, n in conflicts.items():
    print(f'  CONFLICT PCB net {o!r} -> schematic nets {sorted(n)} (left unchanged)')

def net(name):
    ni = b.FindNet(name)
    if ni is None:
        ni = pcbnew.NETINFO_ITEM(b, name); b.Add(ni)
    return ni

# One pass over all copper with a precomputed map, so chained renames (X->Y while Y->Z) cannot merge nets.
target = {old: next(iter(new)) for old, new in votes.items() if old not in conflicts and next(iter(new)) != old}
dst = {old: net(new) for old, new in target.items()}
for item in list(b.GetTracks()) + list(b.Zones()) + [p for f in b.GetFootprints() for p in f.Pads()]:
    if item.GetNetname() in dst:
        item.SetNet(dst[item.GetNetname()])
renamed = len(target)
# Pads with no copper yet (or newly connected in the schematic)
assigned = 0
for f in fps:
    for p in f.Pads():
        key = (f.GetReference(), p.GetNumber())
        if key in want and p.GetNetname() != want[key] and p.GetNetname() not in conflicts:
            p.SetNet(net(want[key])); assigned += 1
# Values and symbol fields
vals = []
for f in fps:
    c = comps.get(f.GetReference())
    if not c:
        print(f'  {f.GetReference()}: not in schematic'); continue
    if f.GetValue() != c['Value']:
        vals.append(f'{f.GetReference()}: {f.GetValue()!r} -> {c["Value"]!r}'); f.SetValue(c['Value'])
    for k, v in c.items():
        if k not in ('Value', 'Footprint', 'Reference') and v and (not f.HasField(k) or f.GetFieldText(k) != v):
            f.SetField(k, v)
b.BuildConnectivity()
b.Save(PCB)
print(f'{PCB}: renamed {renamed} nets, re-assigned {assigned} pads, {len(conflicts)} conflicts')
for v in vals:
    print('  value', v)
