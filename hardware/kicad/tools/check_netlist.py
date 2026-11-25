#!/usr/bin/env python3
"""Check a schematic's netlist pin by pin against a design table.

Usage: python3 check_netlist.py <board.kicad_sch> <pin_nets.csv>
The CSV has columns Reference,Pin,Net. Net "NC" means the pin must be unconnected.
Exits non-zero on any difference, so it works as a regression check after schematic edits.
"""
import csv, subprocess, sys, tempfile, xml.etree.ElementTree as ET
from pathlib import Path

CLI = '/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli'
sch, table = map(Path, sys.argv[1:3])
with tempfile.TemporaryDirectory() as tmp:
    xml = Path(tmp) / 'netlist.xml'
    subprocess.run([CLI, 'sch', 'export', 'netlist', '--format', 'kicadxml', '--output', str(xml), str(sch)],
                   check=True, capture_output=True)
    actual = {}
    for net in ET.parse(xml).getroot().findall('./nets/net'):
        for node in net.findall('node'):
            if not node.get('ref', '').startswith('#'):
                actual[(node.get('ref'), node.get('pin'))] = net.get('name', '').lstrip('/')

expected = {(r['Reference'], r['Pin']): r['Net'] for r in csv.DictReader(table.open()) if not r['Reference'].startswith('#')}
errors = []
for key, want in expected.items():
    got = actual.get(key, '')
    if want == 'NC':
        if got and not got.startswith('unconnected-'):
            errors.append((key, want, got))
    elif got != want:
        errors.append((key, want, got))
errors += [(key, 'not in table', got) for key, got in actual.items() if key not in expected]
for (ref, pin), want, got in errors:
    print(f'MISMATCH {ref}.{pin}: table says {want!r}, schematic has {got!r}')
print(f'{sch.name}: {len(expected)} pins checked, {len(errors)} differences')
sys.exit(bool(errors))
