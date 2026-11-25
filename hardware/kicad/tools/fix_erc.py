# Resolve the mechanical ERC leftovers of an EasyEDA import, without hiding real problems.
# Usage: python3 fix_erc.py <sch> <erc.json> <netlist.xml>
#  - power rails fed through connectors/switches (power_pin_not_driven on a connected net) -> one PWR_FLAG per net
#  - unused pins of modules/connectors (single-pin nets) -> no-connect marker
#  - anything else (e.g. an unconnected regulator input) is only reported
import sys, os, re, uuid, copy, json
import xml.etree.ElementTree as ET
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from port_sch import parse, dump, kids, first, prop, rename, Q, POWER  # noqa: E402  (port_sch has no side effects on import)

SCH, ERC, NET = sys.argv[1:4]
NC_OK = re.compile(r'^HAM:(Mega2560_Pro_Embed|Arduino_Uno_R3|PCA9685_Breakout|HC-05_Module|Conn_\w+|Barrel_Jack)$')

nl = ET.parse(NET).getroot()
pin_net, net_size = {}, {}
for n in nl.iter('net'):
    nodes = list(n.iter('node'))
    net_size[n.get('name')] = len(nodes)
    for nd in nodes:
        pin_net[(nd.get('ref'), nd.get('pin'))] = n.get('name')

t = parse(open(SCH, encoding='utf-8').read())
insts = [c for c in t if isinstance(c, list) and c and c[0] == 'symbol']
lib_of = {prop(i, 'Reference')[2]: first(i, 'lib_id')[1] for i in insts}
val_of = {prop(i, 'Reference')[2]: prop(i, 'Value')[2] for i in insts}

e = json.load(open(ERC))
flags, ncs, left = {}, set(), []
for v in (v for s in e['sheets'] for v in s['violations']):
    if v['type'] not in ('power_pin_not_driven', 'pin_not_driven', 'pin_not_connected'):
        continue
    it = v['items'][0]
    m = re.match(r'Symbol (\S+) Pin (\S+) \[', it['description'])
    if not m:
        continue
    ref, pin = m.groups()
    xy = (round(it['pos']['x'] * 100, 4), round(it['pos']['y'] * 100, 4))
    net = pin_net.get((ref, pin))
    alone = net is None or net_size.get(net, 0) <= 1
    if ref.startswith('#'):  # power symbols are not in the netlist; their net is their value
        net, alone = val_of[ref], False
    if alone and NC_OK.match(lib_of.get(ref, '')):
        ncs.add(xy)
    elif not alone and v['type'] == 'power_pin_not_driven':
        flags.setdefault(net, xy)
    else:
        left.append(f'{v["type"]}: {it["description"]} (net {net})')

for x, y in sorted(ncs):
    t.append(['no_connect', ['at', str(x), str(y)], ['uuid', Q(str(uuid.uuid4()))]])

if flags:
    ls = first(t, 'lib_symbols')
    if not any(s[1] == 'power:PWR_FLAG' for s in kids(ls, 'symbol')):
        pf = [s for s in kids(parse(open(POWER, encoding='utf-8').read()), 'symbol') if s[1] == 'PWR_FLAG'][0]
        ls.append(rename(copy.deepcopy(pf), 'power:PWR_FLAG'))
    tmpl = next(i for i in insts if prop(i, 'Reference')[2].startswith('#PWR'))
    n0 = 1 + max([int(r[4:]) for r in lib_of if r.startswith('#FLG')] or [0])
    for k, (net, (x, y)) in enumerate(sorted(flags.items())):
        ref = '#FLG%02d' % (n0 + k)
        s = copy.deepcopy(tmpl)
        first(s, 'lib_id')[1] = Q('power:PWR_FLAG')
        at = first(s, 'at'); at[1:] = [str(x), str(y), '0']
        if first(s, 'mirror'): s.remove(first(s, 'mirror'))
        first(s, 'uuid')[1] = Q(str(uuid.uuid4()))
        prop(s, 'Reference')[2] = Q(ref); prop(s, 'Value')[2] = Q('PWR_FLAG')
        for p in kids(s, 'property'):
            pa = first(p, 'at'); pa[1:3] = [str(x), str(round(y - 2.54, 4))]
        for p in kids(s, 'pin'):
            p[1] = Q('1'); first(p, 'uuid')[1] = Q(str(uuid.uuid4()))
        def set_ref(node):
            for c in node:
                if isinstance(c, list):
                    if c and c[0] == 'reference':
                        c[1] = Q(ref)
                    set_ref(c)
        set_ref(first(s, 'instances'))
        t.append(s)

open(SCH, 'w', encoding='utf-8').write(dump(t) + '\n')
print(f'{os.path.basename(SCH)}: {len(ncs)} no-connects, PWR_FLAG on {sorted(flags) or "-"}')
for l in left:
    print('  LEFT', l)
