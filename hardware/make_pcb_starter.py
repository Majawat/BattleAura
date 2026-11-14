"""Build the layout starting point for BattleAura v2 from hardware/battleaura_v2.net.

Creates hardware/pcb/battleaura_v2.kicad_pcb + .kicad_pro with:
  * the 45 x 36 mm board outline (ENV-1) on Edge.Cuts, 2 copper layers (ASM-5),
  * JLCPCB-safe design rules and net classes for the power nets,
  * every footprint from the netlist, with nets assigned to pads, parked in rows beside the board.
Placement and routing are left for the layout session.

Run with KiCad's Python (system python3 with the pcbnew module):  python3 hardware/make_pcb_starter.py
Refuses to overwrite an existing board, so a started layout is never clobbered.
"""

import json
import os
import re
import sys

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
NETLIST = os.path.join(HERE, "battleaura_v2.net")
OUT_DIR = os.path.join(HERE, "pcb")
PCB = os.path.join(OUT_DIR, "battleaura_v2.kicad_pcb")
PRO = os.path.join(OUT_DIR, "battleaura_v2.kicad_pro")
FP_ROOT = "/usr/share/kicad/footprints"

BOARD_W, BOARD_H = 45.0, 36.0          # ENV-1
ORIGIN = (100.0, 100.0)                # board top-left in mm

# Net classes: widths sized for the currents in architecture.md §11 (1.16 A peak on the battery path).
NETCLASSES = {
    "Power":  {"track": 0.6, "clearance": 0.2, "via_dia": 0.8, "via_drill": 0.4,
               "nets": ["GND", "CELL_P", "CELL_N", "VBAT_RAW", "VBUS", "VSYS", "VBAT_SW", "ZONE_V+"]},
    "Supply": {"track": 0.3, "clearance": 0.15, "via_dia": 0.6, "via_drill": 0.3,
               "nets": ["+3V3"]},
    "Speaker": {"track": 0.4, "clearance": 0.15, "via_dia": 0.6, "via_drill": 0.3,
                "nets": []},  # filled with the two amp output nets below
}


def mm(v):
    return pcbnew.FromMM(v)


def parse_sexpr(text):
    tokens = re.findall(r'\(|\)|"(?:[^"\\]|\\.)*"|[^\s()]+', text)
    stack, cur = [], []
    for t in tokens:
        if t == "(":
            stack.append(cur)
            cur = []
        elif t == ")":
            done, cur = cur, stack.pop()
            cur.append(done)
        else:
            cur.append(t[1:-1] if t.startswith('"') else t)
    return cur[0]


def find(node, key):
    return [c for c in node if isinstance(c, list) and c and c[0] == key]


def first(node, key):
    hit = find(node, key)
    return hit[0] if hit else None


def read_netlist(path):
    root = parse_sexpr(open(path, encoding="utf-8").read())
    comps = []
    for comp in find(first(root, "components"), "comp"):
        fields = {}
        f_block = first(comp, "fields")
        if f_block:
            for f in find(f_block, "field"):
                fields[first(f, "name")[1]] = f[2] if len(f) > 2 else ""
        comps.append({
            "ref": first(comp, "ref")[1],
            "value": first(comp, "value")[1],
            "footprint": first(comp, "footprint")[1],
            "lcsc": fields.get("LCSC", ""),
        })
    pad_net = {}
    for net in find(first(root, "nets"), "net"):
        name = first(net, "name")[1]
        for node in find(net, "node"):
            pad_net[(first(node, "ref")[1], first(node, "pin")[1])] = name
    return comps, pad_net


def main():
    if os.path.exists(PCB):
        sys.exit(f"{PCB} exists — refusing to overwrite a started layout. Delete it first to regenerate.")
    os.makedirs(OUT_DIR, exist_ok=True)
    comps, pad_net = read_netlist(NETLIST)
    NETCLASSES["Speaker"]["nets"] = sorted({n for (r, p), n in pad_net.items() if r == "J11"})

    board = pcbnew.BOARD()
    board.SetCopperLayerCount(2)

    # outline
    x0, y0 = ORIGIN
    corners = [(x0, y0), (x0 + BOARD_W, y0), (x0 + BOARD_W, y0 + BOARD_H), (x0, y0 + BOARD_H)]
    for (ax, ay), (bx, by) in zip(corners, corners[1:] + corners[:1]):
        seg = pcbnew.PCB_SHAPE(board)
        seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
        seg.SetStart(pcbnew.VECTOR2I(mm(ax), mm(ay)))
        seg.SetEnd(pcbnew.VECTOR2I(mm(bx), mm(by)))
        seg.SetLayer(pcbnew.Edge_Cuts)
        seg.SetWidth(mm(0.1))
        board.Add(seg)

    # nets
    nets = {}
    for name in sorted(set(pad_net.values())):
        ni = pcbnew.NETINFO_ITEM(board, name)
        board.Add(ni)
        nets[name] = ni

    # footprints, parked in rows to the right of the board
    px, py, row_h = x0 + BOARD_W + 10, y0, 0.0
    missing = []
    for c in comps:
        lib, name = c["footprint"].split(":", 1)
        fp = pcbnew.FootprintLoad(os.path.join(FP_ROOT, lib + ".pretty"), name)
        if fp is None:
            missing.append(c["footprint"])
            continue
        fp.SetReference(c["ref"])
        fp.SetValue(c["value"])
        fp.SetFPID(pcbnew.LIB_ID(lib, name))
        if c["lcsc"]:
            fp.SetField("LCSC", c["lcsc"])
        for pad in fp.Pads():
            net = pad_net.get((c["ref"], pad.GetNumber()))
            if net:
                pad.SetNet(nets[net])
        board.Add(fp)
        bb = fp.GetBoundingBox(False)
        w, h = pcbnew.ToMM(bb.GetWidth()), pcbnew.ToMM(bb.GetHeight())
        if px + w > x0 + BOARD_W + 10 + 120:
            px, py, row_h = x0 + BOARD_W + 10, py + row_h + 2, 0.0
        fp.SetPosition(pcbnew.VECTOR2I(mm(px + w / 2), mm(py + h / 2)))
        px += w + 2
        row_h = max(row_h, h)
    if missing:
        sys.exit(f"missing footprints: {missing}")

    # JLCPCB standard 2-layer capabilities, with margin (track/space 0.15 mm vs 0.127 mm minimum)
    ds = board.GetDesignSettings()
    ds.m_TrackMinWidth = mm(0.15)
    ds.m_MinClearance = mm(0.15)
    ds.m_ViasMinSize = mm(0.6)
    ds.m_MinThroughDrill = mm(0.3)
    ds.m_CopperEdgeClearance = mm(0.3)
    ds.m_HoleToHoleMin = mm(0.5)
    pcbnew.SaveBoard(PCB, board)

    # project file: net classes + patterns (KiCad 9 reads these on open)
    pro = json.load(open(PRO)) if os.path.exists(PRO) else {}
    ns = pro.setdefault("net_settings", {})
    classes = [{"name": "Default", "track_width": 0.2, "clearance": 0.15, "via_diameter": 0.6,
                "via_drill": 0.3, "diff_pair_width": 0.2, "diff_pair_gap": 0.15}]
    patterns = []
    for cname, c in NETCLASSES.items():
        classes.append({"name": cname, "track_width": c["track"], "clearance": c["clearance"],
                        "via_diameter": c["via_dia"], "via_drill": c["via_drill"]})
        patterns += [{"netclass": cname, "pattern": n} for n in c["nets"]]
    ns["classes"] = classes
    ns["netclass_patterns"] = patterns
    pro.setdefault("meta", {"filename": os.path.basename(PRO), "version": 1})
    json.dump(pro, open(PRO, "w"), indent=2)

    print(f"wrote {PCB}: {len(comps)} footprints, {len(nets)} nets, outline {BOARD_W} x {BOARD_H} mm")


if __name__ == "__main__":
    main()
