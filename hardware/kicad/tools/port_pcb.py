# Port EasyEDA-imported footprints on one board to library footprints.
#   - custom modules -> saved into ../lib/HAM.pretty, FPID set to HAM:<name>
#   - parts with a stock KiCad equivalent -> swapped only if every pad lands within TOL of the old pad
#   - EasyEDA Hole_gge*/Pad_gge* -> board-only MountingHole / HAM pad footprints
# Writes <board>.fpmap.json (ref -> FPID) for the schematic step. Run with KiCad's python3.
import sys, os, json, math, pcbnew

PCB = sys.argv[1]
HERE = os.path.dirname(os.path.abspath(__file__))
HAM = os.path.join(HERE, '..', 'lib', 'HAM.pretty')
STD = '/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints'
TOL = pcbnew.FromMM(0.15)

CUSTOM = {  # imported footprint name -> HAM footprint name
    'MEGA2560PRO-EMBED': 'Mega2560_Pro_Embed',
    'WIRELESS-BLUETOOTH-HC-05': 'HC-05_Module',
    'PCB_PCA9685_V2': 'PCA9685_Breakout_Adafruit',
    'LM2596{colon}YAAJ_DCDC_STEPDOWN_LM2596': 'LM2596_Module',
    'LM2596:YAAJ_DCDC_STEPDOWN_LM2596': 'LM2596_Module',
    'DC BARREL JACK 2.1': 'DC_Barrel_Jack_2.1mm',
    'POWERPOLE 1327 PAIR HORIZONTAL': 'Anderson_PowerPole_1327_Pair_Horizontal',
    'TOGGLESWITCH SPDT / SPST WIRE': 'Toggle_Switch_SPDT_Wire_Pads',
    'ARDUINO-UNO-R3-PINS': 'Arduino_Uno_R3_Shield',
    'CONN-TH_C722693': 'Conn_THT_LCSC_C722693_1x03_P2.50mm',  # part not identified: kept exact, verify
}
STOCK = {  # imported footprint name -> stock KiCad footprint (swapped only if pads fit)
    'R1206': 'Resistor_SMD:R_1206_3216Metric',
    'R0402': 'Resistor_SMD:R_0402_1005Metric',
    '4PIN_2.54_MM_JST_XHP_JST_CONNECTOR_FOR_DC_SOCKET': 'Connector_JST:JST_XH_B4B-XH-A_1x04_P2.50mm_Vertical',
}

b = pcbnew.LoadBoard(PCB)
log, fpmap = [], {}

def load_std(fpid):
    lib, name = fpid.split(':')
    f = pcbnew.FootprintLoad(os.path.join(STD, lib + '.pretty'), name)
    f.SetFPID(pcbnew.LIB_ID(lib, name))
    return f

def pads(f):
    return {p.GetNumber(): p.GetPosition() for p in f.Pads() if p.GetNumber()}

def fit(old, new):
    """Rotate/translate `new` so its pads sit on `old`'s pads; return max pad error or None."""
    op = pads(old)
    best = None
    for rot in (0, 90, 180, 270):
        new.SetOrientationDegrees(rot); new.SetPosition(pcbnew.VECTOR2I(0, 0))
        np_ = pads(new)
        if set(np_) != set(op):
            return None
        # align centroids so a pitch mismatch splits evenly across the pins
        cx = sum(v.x for v in op.values()) / len(op) - sum(v.x for v in np_.values()) / len(np_)
        cy = sum(v.y for v in op.values()) / len(op) - sum(v.y for v in np_.values()) / len(np_)
        err = max(math.hypot(np_[k].x + cx - op[k].x, np_[k].y + cy - op[k].y) for k in op)
        if best is None or err < best[0]:
            best = (err, rot, int(cx), int(cy))
    err, rot, cx, cy = best
    new.SetOrientationDegrees(rot); new.SetPosition(pcbnew.VECTOR2I(cx, cy))
    return err

def replace(old, new, board_only=False):
    new.SetReference(old.GetReference()); new.SetValue(old.GetValue())
    new.SetPath(old.GetPath()); new.SetSheetname(old.GetSheetname()); new.SetSheetfile(old.GetSheetfile())
    if old.IsFlipped():
        new.Flip(new.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
    nets = {p.GetNumber(): p.GetNet() for p in old.Pads()}
    b.Add(new)
    for p in new.Pads():
        if p.GetNumber() in nets:
            p.SetNet(nets[p.GetNumber()])
    if board_only:
        new.SetBoardOnly(True); new.SetExcludedFromBOM(True); new.SetExcludedFromPosFiles(True)
    b.Remove(old)

def save_custom(f, name):
    """Save a normalized copy of board footprint f into HAM.pretty (once per name)."""
    path = os.path.join(HAM, name + '.kicad_mod')
    pos, rot = f.GetPosition(), f.GetOrientationDegrees()
    ref, val = f.GetReference(), f.GetValue()
    f.SetOrientationDegrees(0); f.SetPosition(pcbnew.VECTOR2I(0, 0))
    f.SetReference('REF**'); f.SetValue(name)
    if os.path.exists(path):
        lib = pcbnew.FootprintLoad(HAM, name)
        same = sorted((k, v.x, v.y) for k, v in pads(lib).items()) == sorted((k, v.x, v.y) for k, v in pads(f).items())
        if not same:
            name += '_' + os.path.splitext(os.path.basename(PCB))[0].replace(' ', '_')
            log.append(f'  {ref}: pad layout differs from existing HAM footprint, saved as {name}')
    f.SetFPID(pcbnew.LIB_ID('HAM', name))
    if not os.path.exists(os.path.join(HAM, name + '.kicad_mod')):
        pcbnew.PCB_IO_KICAD_SEXPR().FootprintSave(HAM, f)
    f.SetReference(ref); f.SetValue(val); f.SetPosition(pos); f.SetOrientationDegrees(rot)
    return name

holes = 0
for f in list(b.GetFootprints()):
    ref, item = f.GetReference(), f.GetFPID().GetLibItemName().wx_str()
    nick = f.GetFPID().GetLibNickname().wx_str()
    full = (nick + ':' + item) if nick else item
    if ref.startswith('Hole_gge'):
        holes += 1
        d = pcbnew.ToMM(list(f.Pads())[0].GetDrillSize().x)
        new = load_std('MountingHole:MountingHole_3.2mm_M3' if d > 3.1 else 'MountingHole:MountingHole_3mm')
        new.SetPosition(list(f.Pads())[0].GetPosition())
        new.Reference().SetVisible(False)
        old_ref = ref
        replace(f, new, board_only=True); new.SetReference(f'H{holes}'); new.SetValue(f'Hole {d:g}mm')
        log.append(f'  {old_ref} -> H{holes} {new.GetFPIDAsString()} (board-only)')
        continue
    if ref.startswith('Pad_gge'):
        name = save_custom(f, 'Pad_THT_Wire_D%.2fmm' % pcbnew.ToMM(list(f.Pads())[0].GetDrillSize().x))
        n = sum(1 for x in b.GetFootprints() if x.GetReference().startswith('WP')) + 1
        f.SetReference(f'WP{n}'); f.SetValue('Wire pad'); f.SetBoardOnly(True); f.SetExcludedFromBOM(True); f.SetExcludedFromPosFiles(True)
        log.append(f'  {ref} -> WP{n} HAM:{name} (board-only GND wire pad)')
        continue
    if nick and os.path.isdir(os.path.join(STD, nick + '.pretty')):
        fpmap[ref] = full
        continue
    if item in STOCK or full in STOCK:
        target = STOCK.get(item) or STOCK[full]
        new = load_std(target)
        err = fit(f, new)
        if err is not None and err <= TOL:
            replace(f, new)
            fpmap[ref] = target
            log.append(f'  {ref}: {full} -> {target} (max pad shift {pcbnew.ToMM(err):.3f} mm)')
            continue
        log.append(f'  {ref}: {target} does not fit {full} (err {None if err is None else round(pcbnew.ToMM(err), 3)}), keeping exact copy')
    name = CUSTOM.get(item) or CUSTOM.get(full) or ('Imported_' + ''.join(c if c.isalnum() or c in '._-' else '_' for c in item))
    name = save_custom(f, name)
    fpmap[ref] = 'HAM:' + name
    log.append(f'  {ref}: {full} -> HAM:{name}')

b.Save(PCB)
json.dump(fpmap, open(os.path.splitext(PCB)[0] + '.fpmap.json', 'w'), indent=1, ensure_ascii=False)
print(os.path.basename(PCB)); print('\n'.join(log))
