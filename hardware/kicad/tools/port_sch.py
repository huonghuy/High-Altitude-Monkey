# Port EasyEDA-imported schematics to the shared HAM symbol library.
# Usage: python3 port_sch.py <sch> [<sch> ...]   (run on all boards together so symbol variants are named consistently)
#  - embedded, library-less symbols -> lib/HAM.kicad_sym (ASCII names, real pin types); pin geometry is kept so no wire moves
#  - bare GND/VCC/+5V -> KiCad power: library
#  - Footprint fields filled from <board>.fpmap.json written by port_pcb.py
#  - EasyEDA import artifact symbol (Unknown_0_*) removed
import sys, os, re, json, copy

class Q(str):
    """Quoted string token (raw text, KiCad escapes preserved)."""

TOK = re.compile(r'\s*(\(|\)|"(?:\\.|[^"\\])*"|[^\s()"]+)', re.S)

def parse(text):
    stack = [[]]
    for m in TOK.finditer(text):
        t = m.group(1)
        if t == '(':
            stack.append([])
        elif t == ')':
            x = stack.pop(); stack[-1].append(x)
        elif t[0] == '"':
            stack[-1].append(Q(t[1:-1]))
        else:
            stack[-1].append(t)
    return stack[0][0]

def atom(x):
    return '"%s"' % x if isinstance(x, Q) else str(x)

def dump(x, ind=0):
    if not isinstance(x, list):
        return atom(x)
    head = [atom(c) for c in x if not isinstance(c, list)]
    kids = [c for c in x if isinstance(c, list)]
    if not kids:
        return '(' + ' '.join(head) + ')'
    if all(not isinstance(k, list) or not any(isinstance(g, list) for g in k) for k in kids) and len(kids) <= 3 and sum(len(dump(k)) for k in kids) < 80:
        return '(' + ' '.join(head + [dump(k) for k in kids]) + ')'
    pad = '\t' * (ind + 1)
    return '(' + ' '.join(head) + ''.join('\n' + pad + dump(k, ind + 1) for k in kids) + '\n' + '\t' * ind + ')'

def kids(node, key):
    return [c for c in node if isinstance(c, list) and c and c[0] == key]

def first(node, key):
    k = kids(node, key)
    return k[0] if k else None

def prop(node, name):
    for p in kids(node, 'property'):
        if p[1] == name:
            return p
    return None

HERE = os.path.dirname(os.path.abspath(__file__))
LIBFILE = os.path.join(HERE, '..', 'lib', 'HAM.kicad_sym')
POWER = '/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols/power.kicad_sym'

BASE = {  # imported symbol name (without _N suffix) -> HAM name
    'DS1809-010+': 'DS1809',
    'JST-XH2.54- 4 PIN Y COPY': 'Conn_01x04', 'JST-XH 2.54 4 PIN': 'Conn_01x04',
    'XH-2P-2.54': 'Conn_01x02', 'XH插座1*3P间距2.5MM直针,': 'Conn_01x03',
    'JST-XH-07-KH': 'Conn_01x07', 'Header-Female-2.54_1x6': 'Conn_01x06',
    'MFR-25FTE52-12K': 'R', 'RC1206FR-7W1KL': 'R', 'RC1206JR-0710KL': 'R', 'RMC12061.5K1%N': 'R',
    'RMC12062K1%N': 'R', 'RMC12063.3K1%N': 'R', 'RMC12063.9K5%N': 'R', 'AECR1206F8K20K9': 'R',
    'RC0402JR-07820RL': 'R',
    '3266Y-1-103LF': 'R_Pot_Trim',
    '2N3904RLRAH': 'Q_NPN_2N3904', 'IRF9540N_FLAT': 'Q_PMOS_IRF9540N',
    'Anderson Powerpole PP15/45 connector pair side-by-side': 'Conn_PowerPole_Pair',
    'DC barrel jack 2.1mm': 'Barrel_Jack', 'TOGGLESWITCH SPST - SPDT WIRE': 'SW_Toggle_SPDT',
    'WIRELESS-BLUETOOTH-HC-05': 'HC-05_Module', 'BuckConverters_LM2596_Adj': 'LM2596_Module',
    'MEGA2560PRO-EMBED': 'Mega2560_Pro_Embed', 'PCA9685_Model_V1': 'PCA9685_Breakout',
    'ArduinoUnoR3_Custom': 'Arduino_Uno_R3',
}
POWER_SYMS = {'GND': 'power:GND', 'VCC': 'power:VCC', '+5V': 'power:+5V'}
DEFAULT_FP = {'Mega2560_Pro_Embed': 'HAM:Mega2560_Pro_Embed', 'HC-05_Module': 'HAM:HC-05_Module',
              'LM2596_Module': 'HAM:LM2596_Module', 'PCA9685_Breakout': 'HAM:PCA9685_Breakout_Adafruit',
              'Arduino_Uno_R3': 'HAM:Arduino_Uno_R3_Shield', 'Barrel_Jack': 'HAM:DC_Barrel_Jack_2.1mm',
              'Conn_PowerPole_Pair': 'HAM:Anderson_PowerPole_1327_Pair_Horizontal'}

def pin_type(sym, name, num):
    n = name.upper()
    if sym == 'DS1809':
        return {'VCC': 'power_in', 'GND': 'power_in', 'UC': 'input', 'DC': 'input', 'STR': 'input'}.get(n, 'passive')
    if sym == 'HC-05_Module':
        return {'STATE': 'output', 'RX': 'input', 'TX': 'output', 'GND': 'power_in', '+5V': 'power_in', 'EN': 'input'}[n]
    if sym == 'LM2596_Module':
        return {'IN+': 'power_in', 'IN-': 'power_in', 'OUT+': 'power_out', 'OUT-': 'passive'}[n]
    if sym == 'Mega2560_Pro_Embed':
        if n in ('5V', 'VIN', 'GND'): return 'power_in'
        if n == '3V3': return 'power_out'
        return 'bidirectional'
    if sym == 'PCA9685_Breakout':
        if n in ('VIN', 'VCC', 'V+', 'GND'): return 'power_in'
        if n == 'OE': return 'input'
        if n.startswith('PWM'): return 'output'
        return 'bidirectional'
    if sym == 'Arduino_Uno_R3':
        if n in ('5V', '3.3V'): return 'power_out'
        if n == 'GND': return 'power_in'
        if n == 'VIN': return 'power_in'
        if n == 'NC': return 'no_connect'
        if n in ('RESET', 'AREF'): return 'input'
        if n == 'IOREF': return 'output'
        return 'bidirectional'
    return 'passive'  # resistors, pots, connectors, switches, jacks, discrete transistors

def base_of(name):
    b = re.sub(r'_\d+$', '', name)
    return b if b in BASE else name

def pins_of(sym):
    out = []
    def walk(n):
        for c in n:
            if isinstance(c, list):
                if c and c[0] == 'pin':
                    at = first(c, 'at'); out.append((first(c, 'number')[1], first(c, 'name')[1], at[1], at[2], at[3] if len(at) > 3 else '0', c))
                else:
                    walk(c)
    walk(sym)
    return out

def signature(sym):
    return tuple(sorted((p[0], p[2], p[3], p[4]) for p in pins_of(sym)))

def tag(sym):
    ps = {p[0]: p for p in pins_of(sym)}
    p1 = ps.get('1') or sorted(ps.values())[0]
    side = {'0': 'L', '180': 'R', '90': 'B', '270': 'T'}.get(str(p1[4]), 'X')
    p2 = ps.get('2')
    if not p2:
        return side
    dx, dy = float(p2[2]) - float(p1[2]), float(p2[3]) - float(p1[3])
    return side + ('Up' if dy > 0 else 'Dn' if dy < 0 else 'Rt' if dx > 0 else 'Lf')

def rename(sym, new):
    """Rename a symbol; unit sub-symbols are named without the library nickname (KiCad convention)."""
    old = sym[1].split(':', 1)[-1]
    sym[1] = Q(new)
    for sub in kids(sym, 'symbol'):
        assert sub[1].startswith(old), (sub[1], old)
        sub[1] = Q(new.split(':', 1)[-1] + sub[1][len(old):])
    return sym

def main():
    files = sys.argv[1:]
    trees = {f: parse(open(f, encoding='utf-8').read()) for f in files}

    # Pass 1: collect geometries per HAM base name across all boards, then name variants.
    variants = {}  # base -> {sig: sym}
    for f, t in trees.items():
        for s in kids(first(t, 'lib_symbols'), 'symbol'):
            b = base_of(s[1])
            if b in BASE:
                variants.setdefault(BASE[b], {}).setdefault(signature(s), s)
    names = {}  # (base, sig) -> final name
    for base, sigs in variants.items():
        used = set()
        for sig, s in sigs.items():
            n = base if len(sigs) == 1 else base + '_' + tag(s)
            while n in used:
                n += 'x'
            used.add(n); names[(base, sig)] = n

    # Build library symbols.
    lib = {}
    for (base, sig), n in names.items():
        s = rename(copy.deepcopy(variants[base][sig]), n)
        for num, pname, x, y, r, node in pins_of(s):
            node[1] = pin_type(base, pname, num)
        v = prop(s, 'Value')
        if v: v[2] = Q(n)
        fp = prop(s, 'Footprint')
        if fp is not None and base in DEFAULT_FP:
            fp[2] = Q(DEFAULT_FP[base])
        lib[n] = s

    power_lib = {s[1]: s for s in kids(parse(open(POWER, encoding='utf-8').read()), 'symbol')}

    report = {}
    for f, t in trees.items():
        fpmap = json.load(open(os.path.splitext(f)[0] + '.fpmap.json', encoding='utf-8'))
        ls = first(t, 'lib_symbols')
        old_to_new = {}
        for s in kids(ls, 'symbol'):
            b = base_of(s[1])
            if b in BASE:
                old_to_new[s[1]] = 'HAM:' + names[(BASE[b], signature(s))]
            elif s[1] in POWER_SYMS:
                old_to_new[s[1]] = POWER_SYMS[s[1]]
        removed, changed, fpset = [], 0, []
        for inst in [c for c in t if isinstance(c, list) and c and c[0] == 'symbol']:
            lid = first(inst, 'lib_id'); ref = prop(inst, 'Reference')[2]
            if lid[1].startswith('Unknown_0_'):
                t.remove(inst); removed.append(ref); continue
            if lid[1] in old_to_new:
                lid[1] = Q(old_to_new[lid[1]]); changed += 1
            if not ref.startswith('#') and ref in fpmap:
                fp = prop(inst, 'Footprint')
                if fp[2] != fpmap[ref]:
                    fpset.append(f'{ref}={fpmap[ref]}'); fp[2] = Q(fpmap[ref])
        used = {first(inst, 'lib_id')[1] for inst in t if isinstance(inst, list) and inst and inst[0] == 'symbol'}
        newls = ['lib_symbols']
        for lid in sorted(used):
            if lid.startswith('HAM:'):
                newls.append(rename(copy.deepcopy(lib[lid[4:]]), lid))
            elif lid.startswith('power:'):
                newls.append(rename(copy.deepcopy(power_lib[lid[6:]]), lid))
            else:
                src = [s for s in kids(ls, 'symbol') if s[1] == lid]
                assert src, f'{f}: no definition for {lid}'
                newls.append(src[0])
        t[t.index(ls)] = newls
        open(f, 'w', encoding='utf-8').write(dump(t) + '\n')
        report[f] = dict(removed=removed, relinked=changed, footprints=fpset, symbols=sorted(used))

    hdr = ['kicad_symbol_lib', ['version', '20241209'], ['generator', Q('ham_port')], ['generator_version', Q('1.0')]]
    open(LIBFILE, 'w', encoding='utf-8').write(dump(hdr + [lib[k] for k in sorted(lib)]) + '\n')
    for f, r in report.items():
        print(f'== {f}: relinked {r["relinked"]} symbols, removed {r["removed"]}')
        print('   footprints set:', ', '.join(r['footprints']) or '-')
        print('   uses:', ', '.join(r['symbols']))
    print('HAM.kicad_sym:', ', '.join(sorted(lib)))


if __name__ == '__main__':
    main()
