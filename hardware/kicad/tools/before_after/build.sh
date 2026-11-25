#!/bin/bash
# Rebuild the before/after comparison images and DRC numbers.
#   before = last EasyEDA version (hardware/easyeda-archive), after = current KiCad board
# Usage: tools/before_after/build.sh [OUT_DIR]   (run from hardware/kicad; default OUT_DIR = /tmp/ham_before_after)
# Then publish tools/before_after/ham-boards.html with OUT_DIR/img2/* to the same artifact URL (see memory / README note).
set -e
K=/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli
OUT=${1:-/tmp/ham_before_after}; IMG=$OUT/img2; mkdir -p "$IMG" "$OUT/easyeda"
A=../easyeda-archive
# key | EasyEDA PCB json | current KiCad board | project whose rules both sides are checked against
BOARDS=(
  "main|$A/HAM-Main_V2.8/PCB_HAM.json|Main/Main.kicad_pcb|Main/Main.kicad_pro"
  "lm|$A/HAM-LM_V2.8/PCB_HAM_Land_Module.json|LM_v2/LM_v2.kicad_pcb|LM_v2/LM_v2.kicad_pro"
  "mc|$A/HAM-MC_V2.5/PCB_HAM- Mission Control.json|MC/MC.kicad_pcb|MC/MC.kicad_pro"
  "vl|$A/HAM-VL_V1.5/PCB_HAM-VideoLynx.json|VL/VL.kicad_pcb|VL/VL.kicad_pro"
  "hc|$A/HC-05 Programmer_V0.2/PCB_HC-05 Programmer_V0.1.json|HC-05_Programmer/HC-05 Programmer.kicad_pcb|HC-05_Programmer/HC-05 Programmer.kicad_pro"
)
render() {  # name pcb
  $K pcb render --side top --width 1000 --height 800 --quality basic --background opaque -o "$IMG/$1.png" "$2" >/dev/null 2>&1
  $K pcb export svg --layers F.Cu,B.Cu,Edge.Cuts --mode-single --fit-page-to-board --exclude-drawing-sheet -o "$IMG/$1_cu.svg" "$2" >/dev/null 2>&1
  qlmanage -t -s 1000 -o "$IMG" "$IMG/$1_cu.svg" >/dev/null 2>&1 && mv "$IMG/$1_cu.svg.png" "$IMG/$1_cu.png"; rm -f "$IMG/$1_cu.svg"
}
for e in "${BOARDS[@]}"; do
  IFS='|' read -r k src now pro <<<"$e"
  b="$OUT/easyeda/${k}_easyeda.kicad_pcb"
  $K pcb import -o "$b" "$src" >/dev/null 2>&1
  cp "$pro" "${b%.kicad_pcb}.kicad_pro"                       # same rules on both sides
  $K pcb drc --refill-zones --format json -o "$OUT/${k}_before.drc.json" "$b" >/dev/null 2>&1 || true
  $K pcb drc --schematic-parity --format json -o "$OUT/${k}_after.drc.json" "$now" >/dev/null 2>&1 || true
  render "${k}_before" "$b"; render "${k}_after" "$now"
  python3 - "$OUT/${k}_before.drc.json" "$OUT/${k}_after.drc.json" "$k" <<'PY'
import json, sys, collections
def s(p):
    d = json.load(open(p)); c = collections.Counter(x['type'] for x in d['violations'])
    silk = sum(n for t, n in c.items() if t.startswith(('silk', 'text'))); crt = sum(n for t, n in c.items() if 'courtyard' in t)
    return dict(unconnected=len(d['unconnected_items']), copper=sum(c.values()) - silk - crt, courtyard=crt, silk=silk)
print(sys.argv[3], 'before', s(sys.argv[1]), 'after', s(sys.argv[2]))
PY
done
echo "LM V2.8's EasyEDA outline is invalid, so its renders come out small: re-render at 3000x2400 and crop to the board (see ham-boards.html notes)."
