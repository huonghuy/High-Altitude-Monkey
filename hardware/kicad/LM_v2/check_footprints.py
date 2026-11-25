#!/usr/bin/env python3
"""Read installed KiCad footprints; never copy them or modify this project."""
from pathlib import Path
import argparse, csv, re, os, sys
p=argparse.ArgumentParser()
p.add_argument('--footprints', default=os.getenv('KICAD10_FOOTPRINT_DIR','/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints'))
a=p.parse_args(); root=Path(a.footprints); here=Path(__file__).resolve().parent
if not root.is_dir(): sys.exit(f'Footprint directory not found: {root}')
with (here/'BOM.csv').open() as f: parts=list(csv.DictReader(f))
with (here/'pin_nets.csv').open() as f: nets=list(csv.DictReader(f))
errors=0
for part in parts:
    ref=part['Reference']; fp=part['Footprint']
    if ':' not in fp:
        print(f'MISSING ASSIGNMENT {ref}'); errors+=1; continue
    lib,name=fp.split(':',1); path=root/(lib+'.pretty')/(name+'.kicad_mod')
    if not path.is_file(): print(f'NOT FOUND {ref}: {fp}');errors+=1;continue
    padnames={x for x in re.findall(r'\(pad\s+"([^"]*)"',path.read_text()) if x}
    expected={n['Pin'] for n in nets if n['Reference']==ref}
    missing=expected-padnames; extra=padnames-expected
    if missing or extra:
        print(f'CHECK {ref}: missing footprint pads {sorted(missing)}; additional pads {sorted(extra)}');errors+=1
    else: print(f'OK {ref}: {fp}')
print(f'{errors} footprint assignments/pad sets need attention. This is not a native ERC or DRC.')
sys.exit(bool(errors))
