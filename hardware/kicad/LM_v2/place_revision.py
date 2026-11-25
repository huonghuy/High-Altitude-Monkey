#!/usr/bin/env python3
"""After native F8 synchronization, place revision 0.2 components and add an unfilled GND zone.
No library copies; no copper routing is created. Refuses boards with existing tracks/zones.
"""
from pathlib import Path
import re, csv, uuid, shutil
from datetime import datetime

root = Path(__file__).resolve().parent
p = root/'LM_v2.kicad_pcb'

def forms(s):
    depth=0; start=None; quoted=False; escaped=False
    for i,c in enumerate(s):
        if quoted:
            if escaped: escaped=False
            elif c=='\\': escaped=True
            elif c=='"': quoted=False
        elif c=='"': quoted=True
        elif c=='(':
            if not depth: start=i
            depth+=1
        elif c==')':
            depth-=1
            if not depth: yield s[start:i+1]
    if depth or quoted: raise ValueError('Unbalanced input')

def children(s): return list(forms(s.strip()[1:-1]))
def field(s,key): return next((x for x in children(s) if x.startswith('('+key+' ')),None)
def ref(s): return re.search(r'\(property\s+"Reference"\s+"([^"]+)"',s)[1]
def nums(s): return list(map(float,re.findall(r'-?\d+(?:\.\d+)?',s)))
def change_at(s,x,y):
    a=field(s,'at'); old=nums(a); suffix=(' '+str(old[2])) if len(old)>2 else ''
    if len(old)>2 and old[2]%360: raise SystemExit(f'STOP: {ref(s)} is rotated; restore original orientation before running this script.')
    dx,dy=x-old[0],y-old[1]
    # Footprint rule-area polygon coordinates are absolute in PCB files; shift any
    # such polygons if their footprint moves. U1 stays fixed in this revision.
    if (abs(dx)>1e-6 or abs(dy)>1e-6) and any(t.startswith('(zone ') for t in children(s)):
        raise SystemExit(f'STOP: {ref(s)} contains a rule area; move it manually in KiCad.')
    return s.replace(a,f'(at {x:g} {y:g}{suffix})',1)

positions={
    'U1':(135,94.56),'J1':(120,127.325),'J2':(154.5,121.6),
    'U4':(132,120),'L1':(137.5,120),'C1':(129,118),'C2':(143,120),
    'F1':(130,125),'C3':(120,104.7),'C4':(126,105.5),
    'C6':(122.5,91),'C7':(123,94),'U5':(120,119),'U3':(120,111.5),
    'U2':(164,109),'C8':(158,106),'C9':(158,109.2),'C10':(158,102.8),
    'C11':(158,113),'C12':(170,104.5),
    'TP1':(145,126),'TP2':(149,126),'TP3':(135,126.5),
}
old=p.read_text(); elems=children(old)
if any(x.startswith(('(segment ','(via ','(arc ','(zone ')) for x in elems):
    raise SystemExit('STOP: board contains existing tracks/vias/zones; no changes made.')
with (root/'BOM.csv').open() as f: bom={r['Reference']:r for r in csv.DictReader(f)}
with (root/'pin_nets.csv').open() as f: expected={(r['Reference'],r['Pin']):r['Net'] for r in csv.DictReader(f)}
fps={ref(x):x for x in elems if x.startswith('(footprint ')}
if set(fps)!=set(bom): raise SystemExit(f'STOP: synchronize with F8 first; missing {sorted(set(bom)-set(fps))}, extra {sorted(set(fps)-set(bom))}')
# KiCad 10 board pad nets are strings. Verify actual pin-level synchronization.
for r,fp in fps.items():
    name=re.match(r'\(footprint\s+"([^"]+)"',fp)[1]
    if name!=bom[r]['Footprint']: raise SystemExit(f'STOP: wrong footprint for {r}: {name}')
    seen=set()
    for pad in children(fp):
        if not pad.startswith('(pad '):continue
        n=re.match(r'\(pad\s+"([^"]*)"',pad)[1]
        if not n:continue
        seen.add(n); want=expected.get((r,n)); net=field(pad,'net'); actual=re.search(r'"([^"]*)"',net)[1] if net and '"' in net else ''
        if want is None:raise SystemExit(f'STOP: unexpected pad {r}.{n}')
        if want!='NC' and actual.lstrip('/')!=want:raise SystemExit(f'STOP: net mismatch {r}.{n}: {actual} != {want}')
        if want=='NC' and actual and 'unconnected-' not in actual:raise SystemExit(f'STOP: expected NC {r}.{n}, found {actual}')
    if seen!={n for rr,n in expected if rr==r}: raise SystemExit(f'STOP: pad-set mismatch {r}')
# Preserve 12 corrected edges; verify dimensions before adding the zone.
edges=[x for x in elems if x.startswith('(gr_') and '(layer "Edge.Cuts")' in x]
if len(edges)!=12:raise SystemExit('STOP: expected 12 legacy Edge.Cuts elements')
for r,(x,y) in positions.items():
    fp=fps[r]; new=change_at(fp,x,y); old=old.replace(fp,new,1)
# Use actual GND spelling; retain ESP32's existing rule area on both layers.
gnd=next(re.search(r'\(net "([^"]+)"\)',pad)[1] for fp in fps.values() for pad in children(fp) if pad.startswith('(pad ') and re.search(r'\(net "/?GND"\)',pad))
zone=f'''(zone (net "{gnd}") (layer "B.Cu") (uuid "{uuid.uuid4()}")
    (hatch edge 0.5) (connect_pads (clearance 0.2)) (min_thickness 0.2)
    (fill yes (thermal_gap 0.3) (thermal_bridge_width 0.3) (island_removal_mode 0))
    (polygon (pts (xy 111 88.0001) (xy 157 88.0001) (xy 157 77.0001)
                  (xy 176.9998 77.0001) (xy 176.9998 131) (xy 111 131))))'''
# Board editor clips the unfilled zone to Edge.Cuts and the existing RF keepout on refill.
old=old.rstrip(); new=old[:-1]+'\n'+zone+'\n)\n'
assert len(list(forms(new)))==1
backup=Path.home()/'Documents/HAM-hardware-review/backups'/('LM_v2_rev02_'+datetime.now().strftime('%Y%m%d-%H%M%S'))
backup.mkdir(parents=True,exist_ok=True);shutil.copy2(p,backup/p.name)
p.write_text(new)
print('Placed',len(positions),'components; added unfilled B.Cu GND zone. No tracks created.')
print('Backup:',backup)
print('Open KiCad, verify placement, press B, then save and run DRC.')
