#!/usr/bin/env python3
"""Validate native KiCad ERC and exported pin-level connectivity on macOS."""
from pathlib import Path
import csv, subprocess, xml.etree.ElementTree as ET, sys
root=Path(__file__).resolve().parent
cli='/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli'
reports=Path.home()/'Documents/HAM-hardware-review/LM_v2_rev02'
reports.mkdir(parents=True,exist_ok=True)
subprocess.run([sys.executable,str(root/'check_footprints.py')],check=True)
subprocess.run([cli,'sch','erc','--exit-code-violations','--output',str(reports/'ERC.rpt'),str(root/'LM_v2.kicad_sch')],check=True)
subprocess.run([cli,'sch','export','netlist','--format','kicadxml','--output',str(reports/'netlist.xml'),str(root/'LM_v2.kicad_sch')],check=True)
expected={(r['Reference'],r['Pin']):r['Net'] for r in csv.DictReader((root/'pin_nets.csv').open()) if not r['Reference'].startswith('#')}
actual={}
for net in ET.parse(reports/'netlist.xml').getroot().findall('./nets/net'):
    for node in net.findall('node'):
        if node.get('ref','').startswith('#'):continue
        k=(node.get('ref'),node.get('pin')); actual[k]=net.get('name','').lstrip('/')
errors=[]
for key,want in expected.items():
    got=actual.get(key,'')
    if want=='NC':
        if got and not got.startswith('unconnected-'):errors.append((key,want,got))
    elif got!=want:errors.append((key,want,got))
for key,got in actual.items():
    if key not in expected:errors.append((key,'not expected',got))
for e in errors:print('MISMATCH',e)
print(f'Netlist differences: {len(errors)}; reports: {reports}')
sys.exit(bool(errors))
