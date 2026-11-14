# BattleAura v2 hardware

The board is described as code in `battleaura_v2.py` (SKiDL). Every connection traces back to
`docs/architecture.md` and the requirement IDs in `docs/requirements.md`.

| File | What |
|---|---|
| `battleaura_v2.py` | Board netlist as code — the schematic source of truth |
| `battleaura_v2.net` | Generated KiCad netlist (commit it; layout imports this) |
| `lib/battleaura.kicad_sym` | Custom symbols not in KiCad's stock library (FS8205A) |
| `make_pcb_starter.py` | Builds `pcb/battleaura_v2.kicad_pcb` from the netlist (outline, rules, parked footprints); won't overwrite |
| `pcb/` | The KiCad board and project |
| `sim/` | ngspice checks (reversed cell) |
| `LAYOUT-HANDOFF.md` | Brief for the layout session |

## Regenerate

```sh
python3 -m venv .venv-hw && .venv-hw/bin/pip install skidl   # once
KICAD9_SYMBOL_DIR=/usr/share/kicad/symbols KICAD9_FOOTPRINT_DIR=/usr/share/kicad/footprints \
    .venv-hw/bin/python hardware/battleaura_v2.py
```

Expected: `0 errors` from ERC and 2 warnings (BQ24074 open-drain CHG/PGOOD into ESP32 GPIOs — by design).

- Part tags are set to the reference designators, so the netlist is reproducible and KiCad's
  "Update PCB from netlist" keeps placements after a re-run. Add new parts at the end of their
  section, or references (and therefore placements) shift.
- Each part carries an `LCSC` field for the JLC assembly BOM; the part counts per LCSC number match
  `docs/bom-draft.csv` exactly (checked 2026-11-14). Test pads have no LCSC number (bare copper).
- Needs KiCad 9 libraries (symbols + footprints) installed.
