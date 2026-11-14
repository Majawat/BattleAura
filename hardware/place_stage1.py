"""Stage-1 placement for BattleAura v2: the hull-fixed parts only.

Places the module, USB-C, slide switch, battery connector, the eight zone connectors and two
M2 mounting holes on the top side of hardware/pcb/battleaura_v2.kicad_pcb. Everything else stays
parked beside the board (see LAYOUT-HANDOFF.md, "Stage 1 result" for why).

Coordinates are mm from the board's top-left corner (x along the 45 mm side, y along the 36 mm
side). The antenna end is x = 0; the speaker end of the hull is x = 45.

Run with KiCad's Python:  python3 hardware/place_stage1.py     (safe to re-run)
"""

import os

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(HERE, "pcb", "battleaura_v2.kicad_pcb")
PRO = PCB.replace(".kicad_pcb", ".kicad_pro")
ORIGIN = (100.0, 100.0)
HOLE_LIB, HOLE_FP = "MountingHole", "MountingHole_2.2mm_M2"

# ref: (x, y, rotation in degrees)
PLACE = {
    "U1":  (12.75, 10.00,  90),   # antenna tip flush with the left edge, pins 1-14 face the board
    "J1":  (23.30, 32.30,   0),   # USB-C, shell front flush with the bottom edge (ENV-4)
    "SW1": (33.20, 34.25,   0),   # slide switch, body 0.3 mm inside the bottom edge, lever beyond it
    "J2":  (40.10,  4.64,  90),   # battery, opening faces the speaker end; pin 1 (+) is the lower pin
    "J3":  ( 9.75, 23.60,   0),   # Z1
    "J4":  (14.42, 33.20, 180),   # Z2
    "J5":  (16.65, 23.60,   0),   # Z3
    "J6":  (28.95, 23.60,   0),   # Z4
    "J7":  (35.85, 23.60,   0),   # Z5
    "J8":  (42.15, 23.90, 270),   # Z6
    "J9":  (41.20, 33.20, 180),   # Z7
    "J10": (30.60,  2.75,   0),   # Z8
}
HOLES = {"H1": (8.50, 33.60), "H2": (42.60, 11.80)}   # ENV-6, diagonal pair


def at(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(ORIGIN[0] + x), pcbnew.FromMM(ORIGIN[1] + y))


def main():
    board = pcbnew.LoadBoard(PCB)
    for ref, (x, y, rot) in PLACE.items():
        fp = board.FindFootprintByReference(ref)
        fp.SetOrientationDegrees(rot)
        fp.SetPosition(at(x, y))
    for ref, (x, y) in HOLES.items():
        fp = board.FindFootprintByReference(ref)
        if fp is None:
            fp = pcbnew.FootprintLoad(f"/usr/share/kicad/footprints/{HOLE_LIB}.pretty", HOLE_FP)
            fp.SetReference(ref)
            fp.SetFPID(pcbnew.LIB_ID(HOLE_LIB, HOLE_FP))
            fp.SetAttributes(pcbnew.FP_BOARD_ONLY | pcbnew.FP_EXCLUDE_FROM_BOM
                             | pcbnew.FP_EXCLUDE_FROM_POS_FILES)
            board.Add(fp)
        fp.SetPosition(at(x, y))
    for fp in board.GetFootprints():          # the starter left the LCSC field visible on silkscreen
        f = fp.GetFieldByName("LCSC")
        if f:
            f.SetVisible(False)
    pro = open(PRO, encoding="utf-8").read()
    pcbnew.SaveBoard(PCB, board)
    open(PRO, "w", encoding="utf-8").write(pro)   # SaveBoard resets the net classes; keep ours
    print(f"placed {len(PLACE)} parts and {len(HOLES)} mounting holes")


if __name__ == "__main__":
    main()
