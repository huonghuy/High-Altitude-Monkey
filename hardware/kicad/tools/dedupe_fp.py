# Swap boards onto one HAM footprint per part when two saved copies differ only by origin offset.
import sys, os, json, pcbnew
sys.argv = sys.argv[:1] + ['-']  # port_pcb's helpers expect a board arg; reuse fit()/replace() logic inline instead
HAM = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'lib', 'HAM.pretty')
TOL = pcbnew.FromMM(0.01)
MERGE = {'LM2596_Module_Main': 'LM2596_Module', 'LM2596_Module_MC': 'LM2596_Module',
         'Mega2560_Pro_Embed_MC': 'Mega2560_Pro_Embed',
         'Conn_THT_LCSC_C722693_1x03_P2.50mm_MC': 'Conn_THT_LCSC_C722693_1x03_P2.50mm'}
for pcb in ('Main/Main.kicad_pcb', 'MC/MC.kicad_pcb'):
    b = pcbnew.LoadBoard(pcb)
    fpmap_path = pcb.replace('.kicad_pcb', '.fpmap.json'); fpmap = json.load(open(fpmap_path))
    for f in list(b.GetFootprints()):
        name = f.GetFPID().GetLibItemName().wx_str()
        if f.GetFPID().GetLibNickname().wx_str() != 'HAM' or name not in MERGE:
            continue
        new = pcbnew.FootprintLoad(HAM, MERGE[name]); new.SetFPID(pcbnew.LIB_ID('HAM', MERGE[name]))
        new.SetOrientation(f.GetOrientation())
        new.SetPosition(pcbnew.VECTOR2I(0, 0))
        op = {p.GetNumber(): p.GetPosition() for p in f.Pads()}; np_ = {p.GetNumber(): p.GetPosition() for p in new.Pads()}
        k = sorted(op)[0]
        new.SetPosition(pcbnew.VECTOR2I(op[k].x - np_[k].x, op[k].y - np_[k].y))
        np_ = {p.GetNumber(): p.GetPosition() for p in new.Pads()}
        err = max(abs(np_[n].x - op[n].x) + abs(np_[n].y - op[n].y) for n in op)
        assert err <= TOL, (pcb, f.GetReference(), err)
        nets = {p.GetNumber(): p.GetNet() for p in f.Pads()}
        new.SetReference(f.GetReference()); new.SetValue(f.GetValue()); new.SetPath(f.GetPath())
        new.SetSheetname(f.GetSheetname()); new.SetSheetfile(f.GetSheetfile())
        b.Add(new)
        for p in new.Pads(): p.SetNet(nets[p.GetNumber()])
        fpmap[f.GetReference()] = 'HAM:' + MERGE[name]
        print(pcb, f.GetReference(), name, '->', MERGE[name])
        b.Remove(f)
    b.Save(pcb); json.dump(fpmap, open(fpmap_path, 'w'), indent=1, ensure_ascii=False)
for n in MERGE:
    os.remove(os.path.join(HAM, n + '.kicad_mod'))
