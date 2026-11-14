# BattleAura v2 — Hardware Architecture (DRAFT)

**Status:** draft for review, 2026-11-10; **revised 2026-11-14** with the owner's answers to OQ-A1…A15 (§15) — charger changed to BQ24074 (§3.4), R_Z 120 Ω, PTC on zone V+, 10 V bulk caps, USB-present via PGOOD. Remaining open items are marked **TODO (layout)** or **TODO (verify)**.
**Input:** `docs/requirements.md` v0.5. **Companion:** `docs/bom-draft.csv` (30 lines, 92 placements).

## 1. Summary

| Subsystem | Pick | MPN (LCSC#) | Satisfies |
|---|---|---|---|
| Cell | 1S LiPo, Adafruit #2011 2000 mAh, 60 × 36 × 7 mm, JST-PH, own PCM (off-board) | Adafruit 2011 | PWR-1, PWR-4 |
| Cell protection | DW01A + dual N-FET in the battery negative return | DW01A (C351410) + FS8205A (C908265) | PWR-2 |
| Reverse polarity | P-FET in the positive lead, gate to battery-connector negative | AO3401A (C15127) | PWR-3 |
| Charger | Power-path linear charger, 4.2 V, 500 mA charge, 4.4 V system output, CHG + PGOOD to GPIO | BQ24074RGTR (C54313; JLC Ext, 3,780 on 2026-11-14) | PWR-5, PWR-6, PWR-7, PWR-8, PWR-16 |
| Power switch | SMD slide switch driving a P-FET load switch | MSK12C02 (C431540) + AO3401A (C15127) | PWR-7 (see OQ-A1), PWR-10 |
| Regulation | 3.3 V 1 A LDO | TLV75733PDBVR (C485517) | PWR-9, PWR-11 |
| MCU | ESP32-S3 module, 16 MB flash, 8 MB PSRAM, PCB antenna | ESP32-S3-WROOM-1-N16R8 (C2913202) | MCU-1..7, ENV-5 |
| LED drive | 8 × single-bit level translator + 120 Ω 1206 series resistor per zone; one PTC fuse on the shared zone V+ | SN74LVC1T45DBVR (C7843); PTC nSMD075 (C70078) | LED-1..9 |
| Audio | I²S mono class-D amplifier | MAX98357AETE+T (C910544) | AUD-1, AUD-2, AUD-3, AUD-6 |
| Wireless / control | Module PCB antenna, Wi-Fi SoftAP + web UI; no extra RF hardware | (part of U1) | WEB-1..7, UI-1/2, SA-1..4 |
| Connectors | USB-C 16-pin; JST-PH 2-pin (battery); JST-SH 3-pin × 8 (zones); JST-SH 2-pin (speaker) | TYPE-C-31-M-12 (C165948); S2B-PH-SM4-TB (C295747); BM03B-SRSS-TB (C160389); BM02B-SRSS-TB (C160388) | PWR-1, PWR-5, LED-6, LED-7, AUD-3, ENV-4 |

### 1.1 Block diagram and rails

```
J1 USB-C ── VBUS ───────────────► U3 BQ24074 IN        OUT ─► VSYS ─► Q2 P-FET load switch ─► VBAT_SW
   │  CC1/CC2 ─ 5.1k ─ GND             │  (power path: OUT = 4.4 V from USB,   ▲ gate
   └─ D+/D- ─ U14 ESD ─► U1 GPIO20/19   │   or = BAT − ≤100 mV on battery)      └─ SW1 slide (signal only)
                                        │ BAT
J2 BAT+ ─► Q1 P-FET (reverse polarity) ─┴─► VBAT_RAW
J2 BAT- ─► Q3 FS8205A (driven by U4 DW01A) ─► GND      └─ U4 VCC

VBAT_SW ─┬─► U2 TLV75733P ─► 3V3 ─► U1 ESP32-S3-WROOM-1, U6–U13 VCCA
         ├─► U5 MAX98357A VDD ─► J11 speaker (8 Ω, 1 W)
         ├─► U6–U13 VCCB ─► R_Z 120 Ω ─► J3–J10 SIG        (zones Z1–Z8)
         ├─► F1 PTC ─► J3–J10 V+
         ├─► 200k/100k divider ─► GPIO1 (ADC)
         └─► 3 × 100 µF 10 V bulk
```

| Rail | Source | Range | Loads |
|---|---|---|---|
| VBUS | USB-C | 5 V nominal; U3 operates 4.35–10.5 V, survives 28 V [BQ pin table, AMR] | U3 charger only |
| VBAT_RAW | cell after Q1 | 3.0–4.2 V (PWR-1) | U4 protection IC, U3 BAT pin — always connected |
| VSYS | U3 OUT | 4.4 V on USB [BQ VOUT(REG)]; VBAT_RAW − 50/100 mV typ/max at 1 A on battery [BQ VDO(BAT-OUT)] | Q2 source — always live when USB or cell present |
| VBAT_SW | VSYS through Q2 | 3.0–4.4 V minus path drop | LDO, amplifier, zone V+ (via F1), translator B side, gauge divider |
| 3V3 | U2 | 3.3 V ±1.5 % [TLV §5.5] | ESP32-S3 module, translator A side, pull-ups |

## 2. Conventions

- **Citations.** Every electrical figure carries `[KEY §/Table]`; keys resolve in [§14 Sources](#14-sources).
  Datasheets without numbered sections are cited by table name: **AMR** = Absolute Maximum Ratings,
  **EC** = Electrical Characteristics, **ROC** = Recommended Operating Conditions.
- **(derived)** = arithmetic on cited figures. **(estimate)** = engineering estimate that must be
  replaced by simulation or measurement. **UNVERIFIED** = I could not confirm it from a datasheet
  I fetched; do not rely on it.
- Figures quoted *from* `requirements.md` are attributed to the requirement ID, not to a datasheet.
- A bare `§n` points inside this document; `req §n` or "requirements §n" points into `requirements.md`.
- **JLCPCB data** (library class, stock, qty-1 unit price) was read from the JLCPCB parts API on
  2026-11-10. "Basic" = base library, "Ext" = extended, "Ext-pref" = preferred extended.
  Extended-part loading fees and PCBA tier rules are **UNVERIFIED** (see §12).
- Typical/maximum values are written `typ/max`; three values are `min/typ/max`.

## 3. Power: battery, protection, charging, switch

### 3.1 Cell (PWR-1, PWR-4)

No cell has been selected, so there is no cell datasheet. Every cell figure below is from
`requirements.md` or UNVERIFIED.

| Option | Key specs | Cost | Size | JLCPCB/LCSC | Risks |
|---|---|---|---|---|---|
| **803450 LiPo pouch + JST-PH lead (pick)** | ≈1500 mAh (PWR-4; UNVERIFIED — no datasheet) | UNVERIFIED | 50 × 34 × 8 mm (req §2.1) | Off-board; not sourced via JLCPCB | Capacity, max discharge current, internal resistance, lead polarity and built-in PCM all UNVERIFIED until a specific cell is chosen |
| 14500 cylindrical + holder/leads | ≈750–1000 mAh (PWR-4; UNVERIFIED) | UNVERIFIED | UNVERIFIED | Off-board | Roughly half the runtime (§11.4); packing differs from req §2.1 |

**Pick: 803450 pouch**, because PWR-4 names it as the primary and the req §2.1 packing is drawn around it.
JST-PH lead polarity varies between vendors (UNVERIFIED lore) — this is why PWR-3 exists.

### 3.2 Cell protection (PWR-2)

| Option | Key specs | Cost (qty 1) | Size | JLCPCB | Risks |
|---|---|---|---|---|---|
| **DW01A + FS8205A (pick)** | Overcharge 4.25/4.30/4.35 V; overdischarge 2.40/2.50/2.60 V; overcurrent sense 130/150/170 mV; short sense 0.82/1.36/1.75 V; supply 3.0/6.0 µA [DW EC]. FET: 20 V, ±12 V V_GS, R_DS(on) 18/25 mΩ @4.5 V, 23/32 mΩ @2.5 V per FET [FS AMR, EC] | $0.0532 + $0.0717 | SOT-23-6 × 2 | Ext, 93,046 / Ext, 20,925 | Both are **extended**, not basic as PWR-2 says. Overcurrent trip depends on FET R_DS(on), so it is loose (below). Clone datasheets (PUOLOP, FUXINSEMI) |
| XB5353A (single chip, integrated FET) | Overdischarge current 2.1/3/3.9 A; R_SS(on) 54/63 mΩ; supply 2.8/6 µA; lists "Protection of Battery Cell Reverse" [XB53 EC, features] | $0.087 | SOT-23-5 | Ext, 3,920 | Low stock; reverse-protection behaviour is a feature bullet, not a characterised spec |
| XB8089D | Overcurrent 10 A; supply 6/12 µA [XB89 EC] | $0.165 | SOIC-8-EP | Ext, 7,694 | 10 A trip is far above anything this board should deliver |
| BQ29700 + external dual FET | OVP 4.275 V, UVP 2.800 V; supply 4/5.5 µA [BQ297 §7] | $0.545 + FET | WSON-6 | Ext, 3,201 | 10× cost, leadless package, low stock; no benefit for a first board |

**Pick: DW01A + FS8205A.** It is the example PWR-2 names, the most-stocked, leaded (inspectable with
a multimeter probe, TOOL-1), and ≈$0.12 total.

- **Overcurrent trip (derived):** sense threshold ÷ two FETs in series. Lowest: 130 mV ÷ (2 × 32 mΩ) ≈ 2.0 A;
  highest: 170 mV ÷ (2 × 18 mΩ) ≈ 4.7 A [DW EC; FS EC]. Delay 7/20 ms (typ/max) [DW EC].
  Short-circuit: ≥0.82 V ÷ 64 mΩ ≈ 12.8 A, delay 400/600 µs (typ/max) [DW EC].
  Worst-case load peak is ≈1.16 A (§11), a 1.7× margin to the lowest trip.
- **Overdischarge 2.40–2.60 V** is the backstop only; firmware shuts down at ≈3.35 V (PWR-12).
- **Circuit** follows the datasheet application circuit: R4 100 Ω + 100 nF on VCC, R5 1 kΩ on CS [DW application circuit].
- **Risk — first-connect latch.** Some DW01-type ICs are reported to stay in overdischarge state after
  the cell is first plugged in until a charger is applied. UNVERIFIED (field lore, not in [DW]); if it
  occurs, plugging USB in once clears it. Check at stage-3 bring-up.

### 3.3 Reverse-polarity protection (PWR-3)

| Option | Key specs | Cost | Size | JLCPCB | Risks |
|---|---|---|---|---|---|
| **AO3401A P-FET, high side (pick)** | V_DS −30 V, V_GS ±12 V, R_DS(on) 47/60 mΩ @−4.5 V, 60/85 mΩ @−2.5 V, V_GS(th) −0.5/−0.9/−1.3 V [AO AMR, EC] | $0.0812 | SOT-23 | **Basic**, 1,641,103 | Interaction with the charger when the cell is reversed (analysis below) |
| SI2301CDS P-FET | V_DS −20 V, R_DS(on) 0.090/0.112 Ω @−4.5 V, 0.110/0.142 Ω @−2.5 V [SI EC] | $0.0918 | SOT-23 | **Basic**, 300,481 | ≈1.7× the on-resistance for no gain |
| B5819W series Schottky | V_F 0.6 V @1 A; I_O 1 A [B58 EC] | $0.0262 | SOD-123 | **Basic** | Loses ≈0.6 V of a 3.0–4.2 V supply and blocks charging through it — not viable |
| XB5353A (protection IC with reverse protection) | see §3.2 | $0.087 | SOT-23-5 | Ext, 3,920 | Replaces Q1 + U4 + Q3 with one part, but low stock and thinly specified |

**Pick: AO3401A.** Drain to the connector's + pin, source to VBAT_RAW, **gate to the connector's − pin
(B−), not to system ground.** Voltage drop at 1.16 A ≤ 85 mΩ × 1.16 A ≈ 0.10 V (derived [AO EC]).

Why the gate goes to B−: with the gate on system ground the charger could turn Q1 on against a reversed
cell and reverse-bias U4 through R4. With the gate on B−:

- **Reversed cell, no USB.** The + pin sits at B− − V_cell. Q1's body diode is reverse-biased and the gate
  is at or above both channel terminals, so there is no conduction path. Stress ≤ V_cell ≤ 4.2 V, against
  −30 V V_DS and ±12 V V_GS [AO AMR].
- **Reversed cell, USB attached.** The charger lifts VBAT_RAW, which can partly enhance Q1; but any Q1
  current pulls VBAT_RAW back toward the gate potential and removes the drive, so Q1 cannot fully turn on.
  The bound is the charger's trickle current: BAT below 2.9 V typ puts the TP4056 in trickle mode at about
  one tenth of the programmed current — 120/130/140 mA at R_PROG 1.1 kΩ [TP EC], i.e. ≈50–58 mA at
  2.2 kΩ (derived, scaled). Q1 then dissipates ≤ ≈5.7 V × 58 mA ≈ 0.33 W (derived), ≈+41 °C at 125 °C/W
  max steady-state [AO Thermal Characteristics]. Peak stress ≈8.4 V (derived), inside the ratings.
  **The reversed cell is slowly discharged (≈50 mA) and is not protected by U4 in this state.**
- **Re-check against the BQ24074 (2026-11-14).** Before charging, the BQ24074 sources only
  I_BAT(SC) ≈1.3 mA into BAT until the pin rises above V_BAT(SC) 1.6/1.8/2.0 V; above that it pre-charges
  at K_PRECHG / R_ISET = 70/88/106 AΩ ÷ 1.8 kΩ ≈ 39/49/59 mA until V_LOWV 3.0 V [BQ §9.3.5, EC]. So the
  bound on current into a reversed cell is the same ≈50–60 mA as with the TP4056, and only ≈1.3 mA while
  Q1 holds VBAT_RAW below ≈1.6 V. **New concern:** the BQ24074 BAT pin is rated −0.3 V minimum [BQ AMR];
  the SPICE run must confirm that a reversed cell cannot pull VBAT_RAW below that through Q1 or U4's
  CS/VCC paths. If it can, add a small Schottky clamp from GND to VBAT_RAW (TODO (verify), OQ-A5).
- **Confidence:** this is reasoned analysis, not a datasheet guarantee. It should be simulated and then
  tested on a sacrificial board before the ASM-6 gate (see OQ-A5).

### 3.4 Charger (PWR-5, PWR-6, PWR-7, PWR-8, PWR-16)

**Decision (OQ-A3, 2026-11-14): BQ24074RGTR power-path charger.** The TP4056 option and its three
no-power-path limitations (charging never terminating with the switch ON, ≈1 W heat, no flashing without a
cell) are superseded. Earlier comparison: TP4056-42-ESOP8 $0.185 Ext-pref; TP4057 $0.142; MCP73831 $0.79;
**BQ24074RGTR, LCSC C54313, $2.119, QFN-16 3×3, Ext, 3,780 in stock (JLC API, 2026-11-14).**

Datasheet: TI SLUS810N [BQ]. Values below are for the **'74 variant** — it differs from '72/'73/'75 in OVP,
output regulation and pin 15.

| Pin | Connection | Why [BQ] |
|---|---|---|
| IN (13) | VBUS, 10 µF (C24) | Operates 4.35–10.5 V on the '74; 1–10 µF bypass |
| OUT (10, 11) | VSYS → Q2 source; 22 µF ceramic close | Regulated 4.4 V from USB; tied to BAT through the internal FET on battery; 4.7–47 µF bypass |
| BAT (2, 3) | VBAT_RAW (after Q1); 10 µF (C25) | 4.7–47 µF bypass; discharge (supplement) path rated 5 A continuous |
| ISET (16) | **R_ISET 1.8 kΩ 1 %** (R3, C25871) to GND | I_CHG = K_ISET / R_ISET = 890 AΩ / 1.8 kΩ ≈ **494 mA** typ (PWR-6); K_ISET 797/890/975 → 443–542 mA. (1.78 kΩ gives exactly 500 mA but has only ≈4k in stock.) |
| ILIM (12) | **R_ILIM 1.1 kΩ 1 %** (R31, C25860) to GND | I_IN(max) = K_ILIM / R_ILIM = 1610 / 1.1 kΩ ≈ **1.46 A** typ (allowed range 1.1–8 kΩ). Input current covers system load + charge; the battery supplements above it. VIN-DPM (4.5 V) backs current off on weak USB ports. |
| EN2 (5) / EN1 (6) | EN2 → VSYS (≤ 4.4 V; pin abs max 7 V); EN1 → GND | Selects ILIM-resistor mode. Do **not** tie EN2 to 3V3: with the switch OFF it would drop to the USB100 mode (100 mA) |
| CE (4) | GND | Charger always enabled (charges with the switch OFF, PWR-7) |
| TS (1) | **10 kΩ fixed resistor to GND** (R32) | Pouch cells have no NTC; datasheet's prescribed no-thermistor connection. Loses cell-temperature charge gating (accepted; cell protection and the charger's own thermal regulation remain) |
| TMR (14) | Not connected | Default safety timers |
| ITERM (15, '74 only) | Not connected | Default termination = 10 % of I_CHG = 50 mA |
| CHG (9) | → GPIO9, 100 kΩ pull-up to 3V3 | Open drain, low while charging (PWR-8) |
| PGOOD (7) | → GPIO10, 100 kΩ pull-up to 3V3 | Open drain, low when valid USB input present — **this is the USB-present signal (OQ-A14)**, so no separate VBUS divider is needed |
| VSS (8) + thermal pad | GND, thermal vias under the pad | **TODO (layout):** copper area for heat |

**Behavior (all states):**

| USB | Switch | Result |
|---|---|---|
| no | OFF | Board off; shelf drain ≈ U4 3–6 µA + U3 I_BAT(PDWN) 4.3/6.5 µA + Q2 ≤1 µA ≈ 8–14 µA |
| no | ON | VSYS = VBAT_RAW − ≤100 mV → board runs from cell |
| yes | OFF | Cell charges at 500 mA; terminates normally at 50 mA; board stays off |
| yes | ON | Board runs from USB at 4.4 V; cell charges with what is left of the 1.46 A input limit; termination works because system current does not flow through the charge path |
| yes, no cell | ON | Board runs and can be flashed (PWR-16) |

**Heat (derived):** charging dissipation ≈ (V_OUT − V_BAT) × I_CHG = (4.4 − 3.0) × 0.5 ≈ 0.7 W worst case,
plus the input FET drop × input current. The IC folds back charge current under thermal regulation, so the
risk is slow charging inside a closed hull, not damage.

**Knock-on effects of the change (TODO (verify) before layout):**
- **Battery gauge:** the divider sits on VBAT_SW (= VSYS). On battery it reads V_BAT − ≤100 mV (calibrated out
  by the FW-1 one-point calibration); **on USB it reads 4.4 V, not the cell.** Firmware therefore shows
  "charging / charged" from CHG + PGOOD while USB is present, and % only on battery. No extra parts.
- **Low-battery cutoff budget (PWR-12):** the BQ battery FET adds 50/100 mV at 1 A to the drop between the
  cell and the LDO input. Re-run the dropout budget (§4) — the 3.35 V cutoff may need to rise slightly.
- **Reverse-polarity analysis (§3.3, OQ-A5):** the "USB attached + reversed cell" case was analysed against
  the TP4056. Re-check it against the BQ24074's BAT pin (abs max −0.3 V) in the SPICE run.

### 3.5 Power switch (PWR-7, PWR-10)

No SMD slide switch stocked at JLCPCB that I found is rated for the load current (≈0.74 A sustained,
≈1.16 A peak, §11):

| Option | Key specs | Cost | Size | JLCPCB | Risks |
|---|---|---|---|---|---|
| **MSK12C02 slide + AO3401A load switch (pick)** | Switch 12 V DC 50 mA, ≤100 mΩ, 10,000 cycles [MSK]; carries gate current only. FET as §3.3 | $0.054 + $0.0812 | ≈1.4 mm high, side-actuated | Ext, 177,894 + Basic | Switch is not physically in the load path — "hard-disconnect" is by FET (OQ-A1). OFF leakage ≤1 µA I_DSS [AO EC] |
| JS102011SAQN slide, direct | 6 V DC 0.3 A [JS] | $0.854 | SMD | Ext, 2,667 | Rated 0.3 A — a quarter of the peak load |
| PCM12SMTR slide, direct | 0.3 A @6 V DC [PCM] | $1.27 | SMD | Ext, 8,954 | Same; rated 0.3 A |
| SS-12D10L5 slide, direct | Rating UNVERIFIED (image-only datasheet) | $0.322 | THT, 13 × 6.8 mm | Ext, 32,396 | Through-hole → violates ASM-1/ASM-4; rating unconfirmed |

**Pick: MSK12C02 driving Q2's gate.** Switch common = Q2 gate. ON throw → 100 kΩ → system ground.
OFF throw → R30 1 kΩ → VSYS (Q2 source, = U3 OUT since the 2026-11-14 charger change). R7 1 MΩ and a 1 µF capacitor sit gate-to-source.

- **OFF:** gate tied to source through 1 kΩ, with 1 MΩ as a second hold-off if the slider is mid-travel.
  Everything on VBAT_SW (regulator, amp, LED rail, gauge divider) is disconnected, as PWR-7 lists.
- **ON:** V_GS = −V_BAT × 1 MΩ/(1 MΩ + 100 kΩ) ≈ −0.909 × V_BAT → −2.73 V at 3.0 V (derived), beyond the
  −1.3 V max threshold and at the −2.5 V R_DS(on) test point [AO EC]. Gate-divider current ≈3.8 µA (derived).
- **Soft start (estimate):** τ = (100 kΩ ∥ 1 MΩ) × 1 µF ≈ 91 ms; estimated inrush into the 200 µF bulk
  ≈0.5 A peak, from g_FS 17 S [AO EC]. Without the 1 µF it would be ≈2.3 A, above the 2.0 A minimum
  protection trip (§3.2). **Estimate only — simulate at stage 0, measure at stage 3.**
- **Switch current** ≤ 4.2 V ÷ 1 kΩ = 4.2 mA (derived) vs 50 mA rating [MSK].
- **Shelf drain, switch OFF (derived):** U4 3.0/6.0 µA [DW EC] + U3 I_BAT(PDWN) 4.3/6.5 µA [BQ] + Q2 ≤1 µA [AO EC]
  ≈ 8–14 µA → years on any plausible cell; self-discharge dominates.

## 4. Regulation (PWR-9, PWR-10, PWR-11)

| Option | Key specs | Cost | Size | JLCPCB | Risks |
|---|---|---|---|---|---|
| **TLV75733PDBVR (pick)** | V_IN 1.45–5.5 V [TLV §5.3]; 1 A; dropout at 1 A 300/425 mV typ/max; current limit 1.2/1.55/1.78 A; I_GND 25/31 µA; accuracy ±1 % / ±1.5 % over temperature [TLV §5.5]; C_OUT 1–200 µF [TLV §5.3] | $0.208 | SOT-23-5 | Ext, 28,517 | V_IN abs max 6.0 V [TLV §5.1] — fine on a 1S cell. R_θJA 231.1 °C/W JEDEC [TLV §5.4] |
| AP2112K-3.3TRG1 | V_IN 2.5–6.0 V; I_OUT ≥600 mA; dropout 250/400 mV typ/max at 600 mA; I_Q 55/80 µA [AP EC] | $0.183 | SOT-25 | Ext, 10,762 | Dropout at 500 mA is not guaranteed ≤300 mV (only the 600 mA max of 400 mV is given) → PWR-9 not provably met |
| RT9080-33GJ5 | V_IN 1.2–5.5 V; dropout at 600 mA 0.31/0.53 V; I_Q 2/4 µA; current limit ≥610 mA [RT EC] | $0.120 | SOT-23-5 | Ext, 22,297 | Dropout too high; current limit barely above PWR-9's 600 mA |
| ME6217C33M5G | Dropout at 300 mA 100/180 mV; I_SS 100/130 µA [ME EC] | $0.098 | SOT-23-5 | Ext, 60,294 | No dropout figure at 500 mA; thin datasheet |
| TPS63802 buck-boost | V_IN 1.3–5.5 V; 2 A; I_Q 11 µA [TPS §8.3, §8.5]; needs 0.47 µH + ≥22 µF | $0.908 | VSON-10 3 × 2 mm | Ext, 23,143 | Rejected by requirements §3.3 (complexity, leadless, layout-sensitive) |

**Pick: TLV75733PDBVR.** Requirements §3.3 already chose an LDO over buck-boost; among LDOs this is the
only one whose datasheet lets PWR-9 be shown to hold: 1 A rating ≥ 600 mA, and dropout at 500 mA
≈150/213 mV typ/max (derived, linear scaling of the 1 A figure [TLV §5.5]) ≤ 300 mV.

- **Capacitors:** C21 1 µF at the input (≥1 µF required), C28 22 µF + C27 10 µF + 100 nF at the output —
  ≈32 µF nominal, inside the 1–200 µF stable range [TLV §5.3] and meeting PWR-11's ≥22 µF.
  **3 × 100 µF 10 V** (C29, C30, C32; GRM31CR61A107MEA8L) on VBAT_SW next to the amp and zone connectors (OQ-A9). Murata's DC-bias curve could not be retrieved (JS-only product page), so per the owner's rule ("add a third if the derated value is short") a third part is fitted: typical 1206 100 µF X5R retention at 4 V is roughly 40–60 % (industry-typical, UNVERIFIED), i.e. ≈120–180 µF effective for three, ≥ PWR-11's 100 µF.
  DC-bias derating of the 100 µF 6.3 V X5R parts at 4.2 V is UNVERIFIED [SAM gives nominal only].
  Alternative considered: tantalum TAJB107K006 (Basic, C16133, $0.2515, case B 3.5 × 2.8 × 1.9 mm):
  ESR 1.7 Ω max at 100 kHz and 224 mA RMS ripple rating [TAJ] — rejected, because 1.7 Ω × the ≈0.46 A
  amplifier peak is ≈0.8 V of sag (derived) and the class-D ripple current exceeds its rating.
- **Headroom at end of discharge (derived).** Series resistance cell → LDO input: Q1 ≤85 mΩ + Q2 ≤85 mΩ
  [AO EC, −2.5 V column] + Q3 ≤64 mΩ [FS EC] + JST-PH 2 × ≤10 mΩ [PH] ≈ 0.25 Ω max, plus lead and cell
  resistance (UNVERIFIED). That is ≈0.19 V at 0.74 A and ≈0.29 V at 1.16 A. LDO dropout at the 355 mA
  radio peak ≈151 mV max (scaled), so 3V3 stays ≥3.0 V [WROOM Table 6-2 minimum] down to
  VBAT_SW ≈ 3.15 V. The gauge divider reads VBAT_SW — downstream of this path drop — so the 3.35 V
  firmware cutoff (PWR-12) guards the LDO input directly, with ≈0.2 V margin. A load step between two
  ADC samples (up to ≈0.42 A more × 0.25 Ω ≈ 0.1 V) stays inside that margin. Below ≈3.45 V at
  VBAT_SW the 3V3 rail tracks the battery down (3.3 V is no longer regulated) but remains in the
  module's 3.0–3.6 V range.
- **Headroom re-run for the BQ24074 (2026-11-14, derived).** The charger's battery FET adds
  V_DO(BAT-OUT) 50/100 mV typ/max at 1 A [BQ] ≈ ≤0.10 Ω, so the cell → LDO-input path is ≈0.35 Ω max
  (Q3 64 + Q1 85 + BQ ≈100 + Q2 85 + JST 20 mΩ), plus cell and lead resistance (UNVERIFIED). Drop:
  ≈0.26 V at 0.74 A, ≈0.41 V at 1.16 A. **The 3.35 V cutoff (PWR-12) does not change:** the gauge reads
  VBAT_SW, downstream of the whole path, so the margin above the LDO's ≈3.15 V minimum input is still
  ≈0.2 V. The worst load step between ADC samples is now ≈0.42 A × 0.35 Ω ≈ 0.15 V — still inside that
  margin, but thinner (was 0.10 V). The cost is capacity left in the cell under heavy load: at the cutoff
  the cell sits at ≈3.6 V under the 0.74 A worst case, ≈3.42 V at the 200 mA typical load. Firmware
  should sample VBAT_SW at least every 100 ms near the cutoff and apply it to the lowest reading over a
  short window, so a load step cannot hide a brown-out (TODO (firmware)).
- **Thermal (derived).** (4.2 − 3.3) V × 0.175 A ≈ 0.16 W → ≈+37 °C at 231.1 °C/W (JEDEC) or ≈+16 °C at
  100.8 °C/W (EVM-style copper) [TLV §5.4].

## 5. MCU (MCU-1..7)

| Option | Key specs | Cost | Size | JLCPCB | Risks |
|---|---|---|---|---|---|
| **ESP32-S3-WROOM-1-N16R8 (pick)** | 16 MB quad flash, 8 MB octal PSRAM, PCB antenna, ambient −40..65 °C [WROOM Table 1-1]; VDD33 3.0/3.3/3.6 V, supply must deliver ≥0.5 A [WROOM Table 6-2] | $4.95 | 18.0 × 25.5 × 3.1 mm | Ext, 24,052 | 65 °C ambient limit (R8 variant); takes ≈28 % of the 45 × 36 mm board (derived) |
| ESP32-S3-WROOM-1-N16 | 16 MB flash, no PSRAM, −40..85 °C [WROOM Table 1-1] | $4.96 | same | Ext, 2,088 | Low stock; no PSRAM (MCU-1 names N16R8) |
| ESP32-S3-WROOM-1U-N16R8 | Same silicon, external-antenna connector | $4.84 | 19.2 × 18 mm (JLCPCB listing) | Ext, 26,162 | Needs an external antenna and cable inside the 65 × 40 × 30 mm cavity; hand assembly |
| ESP32-S3-MINI-1-N8 | 8 MB flash, no PSRAM, −40..85 °C [MINI Table 1-1] | $4.74 | 15.4 × 20.5 × 2.4 mm | Ext, 22,502 | 8 MB cannot hold 2 × ≤3 MB apps + ≥8 MB LittleFS (MCU-2) |
| ESP32-C6-MINI-1-N4 | Specs UNVERIFIED (datasheet not fetched) | $4.09 | — | Ext, 2,304 | Rejected by requirements §6.1 (single core, two audio streams) |

**Pick: ESP32-S3-WROOM-1-N16R8**, as MCU-1 specifies. It is the only stocked option that meets MCU-2's
partition budget with PSRAM, has native USB on GPIO19/20 (MCU-3) and matches the DevKitC-1-N16R8 used in
stage 1 (req §10.1), so firmware moves to the PCB with only a pin-map change (FW-2).

- **Support circuit:** EN pull-up R21 10 kΩ + C 1 µF to ground, as the module's reference schematic
  [WROOM §9 Peripheral Schematics]; RESET button SW3 across the capacitor; BOOT button SW2 on GPIO0
  (MCU-4). Decoupling 10 µF + 100 nF at pin 2.
- **Reset timing:** rails must be stable ≥50 µs before EN rises [S3 Figure 2-3, Table 2-13]; the
  10 kΩ × 1 µF = 10 ms RC provides this (derived).
- **MCU-7 clarification:** on this module GPIO26–34 are not brought out at all, and IO35–37 are present
  as pins but unusable on R8 variants [WROOM Table 3-1 notes]. Strapping pins are GPIO0, 3, 45, 46
  [WROOM Table 3-1].
- **Antenna:** module at a board edge with the antenna overhanging or facing free space (ENV-5). The
  numeric keep-out is in Espressif's hardware design guidelines, referenced from [WROOM §11.2] —
  the dimension itself is UNVERIFIED here and must be read at layout time.

### 5.1 Proposed pin map

| Function | GPIO | Module pin | Notes |
|---|---|---|---|
| Zone Z1–Z8 | 4, 5, 6, 7, 15, 16, 17, 8 | 4, 5, 6, 7, 8, 9, 10, 12 | Consecutive pins on one module edge. No pull at reset [S3 Table 2-1 note]; power-up glitch is low-level only on GPIO1–17 [S3 Table 2-2]; each has a 100 kΩ pull-down (LED-5) |
| VBAT sense | 1 (ADC1_CH0) | 39 | ADC1 channel; ADC2 channels cannot be used while Wi-Fi is active [S3 §4.2.2.1] |
| Status LED | 2 | 38 | Via 1 kΩ (UI-1) |
| I²S LRCLK / BCLK / DIN | 11 / 12 / 13 | 19 / 20 / 21 | |
| Amp SD_MODE | 14 | 22 | Direct drive; amp's internal 100 kΩ pull-down keeps it off through reset |
| Charger CHG / PGOOD (USB present) | 9 / 10 | 17 / 18 | BQ24074 open-drain outputs, 100 kΩ pull-ups to 3V3 (§3.4) |
| BOOT | 0 | 27 | SW2 to ground |
| RESET | EN | 3 | SW3; 10 kΩ + 1 µF |
| USB D− / D+ | 19 / 20 | 13 / 14 | Through U14 |
| UART0 TX / RX test pads | 43 / 44 | 37 / 36 | Plus 3V3, GND, EN, GPIO0 pads (MCU-6) |
| Unused → test pads (OQ-A14) | 3, 18, 21, 38–42, 47, 48 to pads; 45, 46 untouched | — | Strapping pins 3, 45, 46: 3 goes to a pad but must not be pulled at boot. GPIO18 avoided for zones (high-level power-up glitch [S3 Table 2-2]) |

Whether every pin above is on the DevKitC-1 headers (needed for FW-2's shared pin map) is UNVERIFIED —
the dev-kit pinout was not fetched.

## 6. LED drive (LED-1..8)

Each zone is identical (LED-1): GPIO → 100 kΩ pull-down → translator (A side 3V3, B side VBAT_SW) →
R_Z → SIG, with V+ = VBAT_SW and GND on the same 3-pin connector. The same circuit drives a plain LED
by PWM or an addressable-pixel data line (LED-2).

### 6.1 Buffer / translator

| Option | Key specs | Cost (8 zones) | Size | JLCPCB | Risks |
|---|---|---|---|---|---|
| **8 × SN74LVC1T45DBVR (pick)** | V_CCA/V_CCB 1.65–5.5 V; V_IH 2 V min, V_IL 0.8 V max at 3–3.6 V; I_OH/I_OL ±24 mA at 3–3.6 V, ±32 mA at 4.5–5.5 V [1T45 §5.3]; abs max output ±50 mA, V_CC/GND ±100 mA [1T45 §5.1]; I_CCA+I_CCB 4 µA max [1T45 §5.5] | 8 × $0.0827 = $0.66 | SOT-23-6 × 8 | Ext, 265,167 | 8 placements + 16 decoupling caps take board area |
| 4 × SN74LVC2T45DCUR | Same output and supply limits per device [2T45 §5.1] | 4 × $0.208 = $0.83 | VSSOP-8 × 4 | Ext, 86,691 | One fault takes out two zones; finer pitch; no real saving |
| 1 × SN74LVC8T245PWR | Continuous current through V_CC or GND ±100 mA [8T245 §5.1] | $0.334 | TSSOP-24 | Ext, 68,715 | **Not viable:** 8 zones × 20 mA = 160 mA exceeds the ±100 mA supply-pin limit |
| Direct 3.3 V GPIO, no buffer | V_OH ≥ 0.8 × VDD [WROOM Table 6-3] | $0 | — | — | Violates LED-2 / req §1.1 "every output buffered"; pixel V_IH not guaranteed (§10.1); a wiring fault reaches the MCU |

**Pick: 8 × SN74LVC1T45DBVR**, the class of part LED-2 names. DIR tied to V_CCA (A→B). One fault
damages one cheap SOT-23 instead of the module (req §1.1). Per-device supply current of one 20 mA zone is
well inside ±100 mA. If either supply is at 0 V the outputs are high-impedance [1T45 §7.3.6 V_CC Isolation], so zones stay dark while the LDO ramps.

- **MCU → translator levels:** ESP32 V_OH ≥ 0.8 × 3.3 V = 2.64 V ≥ 2 V V_IH; V_OL ≤ 0.1 × 3.3 V = 0.33 V
  ≤ 0.8 V V_IL [WROOM Table 6-3; 1T45 §5.3]. Pass.
- **Output level:** V_OH ≥ V_CCB − 0.1 V at −100 µA; ≥2.4 V at −24 mA with V_CCB = 3 V [1T45 §5.5] →
  output resistance ≤25 Ω (derived).
- **Reset state:** zone GPIOs have no internal pull at reset [S3 Table 2-1 note]; the 100 kΩ pull-down
  holds the translator input low (LED-5). GPIO load per zone ≈ 3.3 V ÷ 100 kΩ + ≤2 µA input leakage
  [1T45 §5.5] ≈ 35 µA (derived).

### 6.2 Series resistor R_Z

| Option | Short SIG→GND at V_CCB 4.2 V (derived) | Resistor dissipation in a sustained short (derived) | Red LED (V_F 1.8 V) at 4.2 V (derived) | JLCPCB | Verdict |
|---|---|---|---|---|---|
| 68 Ω 0402 (LED-3 example) | 62 mA — **above** ±50 mA abs max [1T45 §5.1] | 259 mW vs 1/16 W rating [UNI] | ≤35 mA | Ext (C25131) | LED-8's "within the translator's absolute max" does not hold |
| 100 Ω 1206 | 42 mA — inside ±50 mA abs max | 176 mW vs 1/4 W rating [UNI] | ≤24 mA | **Basic** (C17901), $0.0059 | Survives an indefinite short on paper |
| **120 Ω 1206 (pick, OQ-A2)** | 35 mA — inside ±50 mA abs max | 147 mW vs 1/4 W rating [UNI] | ≤20 mA | 1206W4F1200T5E, LCSC# TODO (verify) — expected Basic | Meets LED-4's 20 mA for red; dimmer whites accepted by the owner |
| 100 Ω 0603 / 0805 | 42 mA | 176 mW vs 1/10 W / 1/8 W [UNI] | ≤24 mA | Basic | Resistor overheats in a sustained short |

**Pick: 120 Ω, 1206** (owner, 2026-11-14: dimmer is fine, lights rarely run at full brightness). The short-circuit numbers ignore the translator's own output resistance, so they
are upper bounds. 35 mA is above the *recommended* ±24/±32 mA [1T45 §5.3] but inside the absolute
maximum, which is what LED-8 ("SHOULD survive") asks for.

- **LED current:** bounded by (V_CCB − V_F) ÷ 120 Ω (figures below were computed for 100 Ω; scale by 0.83). Red (V_F 1.8–2.4 V at 20 mA, taking [KT] as a
  representative red LED): ≤24 mA at 4.2 V — at the translator's 24 mA recommended figure and slightly
  over LED-4's 20 mA; 120 Ω would hold exactly 20 mA (OQ-A2). White/blue (V_F ≈3.0 V, UNVERIFIED — no
  LED chosen): ≈12 mA at 4.2 V falling to ≈4 mA at 3.4 V, which is the dimming PWR-15 compensates for.
- **Pixel data through R_Z:** 120 Ω × ≈100 pF (cable + pixel input, estimate) ≈ 10 ns, plus translator
  delay ≤5.8 ns [1T45 §5.8, A→B, V_CCB 3.3 V column] — negligible against the 200 ns minimum T0H [WS].
- **V+ pin:** direct from VBAT_SW; JST-SH contacts are rated 0.7 A [SH] ≥ LED-7's 300 mA. **There is no
  per-zone current limit on V+**: a V+→GND short on a zone lead is cleared only by the cell protection
  at 2.0–4.7 A (§3.2). **Resolved (OQ-A15): F1, one resettable PTC (≈0.5–1 A hold) on the shared zone V+ bus** — part and JLC stock TODO (verify); keep its voltage drop at 300 mA in the pixel-supply budget.

## 7. Audio (AUD-1..7)

| Option | Key specs | Cost | Size | JLCPCB | Risks |
|---|---|---|---|---|---|
| **MAX98357AETE+T (pick)** | V_DD 2.5–5.5 V; I_Q 2.4/2.85 mA at 3.7 V; shutdown 0.6/2 µA; 0.77 W at 1 % THD+N, 0.93 W at 10 %, into 8 Ω + 68 µH at 3.7 V; digital V_IH 1.3 V min, V_IL 0.6 V max; current limit 2.8 A typ [MAX EC] | $1.309 | TQFN-16, 3 × 3 mm | Ext, 31,426 | Leadless (joints not inspectable — relies on JLC reflow); LRCLK must be one of 8/16/32/44.1/48/88.2/96 kHz [MAX "LRCLK Polarity"] |
| NS4168 | V_DD 3.0–5.5 V; I_DD 13 mA typ; shutdown 1 µA; digital V_IH 0.7 × V_DD, V_IL 0.3 × V_DD; output specified into 4 Ω only (1.2 W at 3.6 V, 10 % THD) [NS §8] | $0.728 | ESOP-8 | Ext, 10,476 | **Logic-level fail:** with V_DD = 4.2 V, V_IH = 2.94 V but the ESP32 only guarantees 2.64 V [WROOM Table 6-3]. No 8 Ω specification. §7 and §8 of its datasheet disagree on the supply range (2.8–5.0 V vs 3.0–5.5 V) |

**Pick: MAX98357AETE+T** — this closes requirements OQ-5 on the electrical evidence.

- **Supply (explicit check):** 2.5–5.5 V [MAX EC] covers VBAT_SW 3.0–4.2 V with margin at both ends;
  UVLO is 1.5/1.8/2.3 V [MAX EC], below the DW01A cutoff. Decoupling 10 µF + 100 nF at V_DD
  [MAX typical application circuits], plus the 2 × 100 µF bulk (PWR-11).
- **Speaker impedance (explicit check):** the amp is characterised into 8 Ω at 3.7 V and into 4 Ω at
  5 V [MAX EC]. AUD-3's 8 Ω speaker is the specified case: 0.77 W at 1 % THD+N ≥ AUD-2's ≈0.5 W.
  Scaling by V² (derived): ≈0.63 W at 3.35 V, ≈0.51 W at 3.0 V. A 4 Ω speaker would roughly double
  the current and is not budgeted — stay at 8 Ω.
- **Logic levels:** ESP32 V_OH ≥ 2.64 V ≥ 1.3 V; V_OL ≤ 0.33 V ≤ 0.6 V [WROOM Table 6-3; MAX EC]. Inputs
  are rated to 6 V regardless of V_DD [MAX AMR]. Pass at every battery voltage.
- **Gain:** GAIN_SLOT tied to V_DD = 6 dB [MAX Table 8]. Output = dBFS + 2.1 dB + gain [MAX], so
  full scale → 8.1 dBV ≈ 2.54 V rms ≈ 0.81 W into 8 Ω (derived) — just past the 1 % THD point at 3.7 V.
  The next step down (3 dB) gives ≈0.40 W, short of AUD-2; the next up (9 dB) clips hard. Firmware
  volume (AUD-5) is capped ≈2 dB below full scale → ≈0.5 W (derived).
- **Shutdown (AUD-6):** SD_MODE wired straight to GPIO14. High (≥1.5 V) selects the left channel
  [MAX Table 5]; firmware sends the mono mix on the left slot. For shutdown the pin is set
  high-impedance: the amp's internal 92–108 kΩ pull-down against the GPIO's 50 nA leakage gives ≈5 mV
  (derived), under the 80 mV minimum shutdown threshold [MAX EC; WROOM Table 6-3]. That also makes the
  amp default-off through reset and deep sleep. SD_MODE's abs max is V_DD + 0.3 V [MAX AMR]; 3V3 is
  derived from the same rail and can never exceed it.
- **Sample rate:** 22.05 kHz is explicitly **not supported** [MAX "LRCLK Polarity": "22.05kHz … NOT supported"]. Requirements §6.1 talks
  about 22 kHz clips — output must run at 44.1 kHz (double each sample in firmware) or clips must be
  stored at 16/32/44.1 kHz. See OQ-A7.
- **Layout note:** filterless class-D output; keep the speaker pair short, twisted, and away from the
  antenna end. No ferrites are in the BOM (EMI impact UNVERIFIED until stage 3).

## 8. Wireless and control (WEB-1..7, UI-1/2, SA-1..4, MCU-3..6)

| Option | Key specs | Cost | Size | JLCPCB | Risks |
|---|---|---|---|---|---|
| **Wi-Fi SoftAP + web UI on the module's PCB antenna (pick)** | 2.4 GHz Wi-Fi; TX peak 355 mA (802.11b, 20.5 dBm), 286 mA (HT20 MCS7); RX 95 mA (HT20) [WROOM Table 6-4] | $0 extra | Part of U1; needs antenna keep-out (ENV-5) | — | Antenna sits inside a resin/plastic hull next to a pouch cell and speaker magnet — range UNVERIFIED until the stage-2 mock-up |
| External antenna (WROOM-1U + u.FL antenna) | Same radio | Module −$0.11, plus antenna (price UNVERIFIED) | Module 19.2 × 18 mm (JLCPCB listing) | Ext, 26,162 | Antenna and cable are hand-fitted (against ASM-4's intent) and must be routed in the cavity |
| BLE control (same hardware) | Radio present in U1 | $0 | — | — | Out of scope (req §13); a browser cannot drive BLE on all phones (UNVERIFIED) |
| Physical buttons / IR receiver | — | parts + hull cut-outs | — | — | Out of scope (req §13) |

**Pick: module PCB antenna, Wi-Fi only.** No additional RF, level-shifting or control hardware is
required; everything in WEB-1..7 and SA-1..4 is firmware. The radio's contribution to the hardware
design is its supply demand: the 3V3 rail must source the 355 mA TX peak [WROOM Table 6-4] against
the module's "≥0.5 A" supply guidance [WROOM Table 6-2] — met by the 1 A LDO with 22 µF + 10 µF at
its output (§4).

On-board control elements:

| Element | Part | Circuit | Check |
|---|---|---|---|
| Power switch SW1 | MSK12C02 | §3.5 | ENV-4: side-actuated, placed on the USB-C edge |
| BOOT SW2, RESET SW3 | TS-1187A-B-A-B (Basic, 848,805, $0.0215) | GPIO0 → GND; EN → GND | MCU-4. Contact rating UNVERIFIED (image-only datasheet); carries <1 mA |
| Status LED D1 | KT-0603R red (Basic) | GPIO2 → 1 kΩ → LED → GND | (3.3 − 1.8) V ÷ 1 kΩ ≤ 1.5 mA (derived) vs I_F 25 mA max [KT]; UI-1 |
| USB data | USBLC6-2SC6 on D+/D− | In line at the connector | Leakage 10/150 nA typ/max at 5.25 V; breakdown ≥6 V; line capacitance 3.5 pF max [ESD Table 2]. MCU-3: native USB-Serial/JTAG on GPIO19/20 |
| UART test pads | — | GPIO43/44, 3V3, GND, EN, GPIO0 | MCU-6, TOOL-1 |

## 9. Connectors (PWR-1, PWR-5, LED-6, LED-7, AUD-3, ENV-3, ENV-4)

### 9.1 Battery (PWR-1)

| Option | Key specs | Cost | Size | JLCPCB | Risks |
|---|---|---|---|---|---|
| **S2B-PH-SM4-TB, side entry (pick)** | 2 A (AWG #24), 100 V, contact resistance ≤10 mΩ [PH] | $0.226 | 5.5 mm high [PH] | Ext, **2,902** | Low stock — may be gone by order day |
| B2B-PH-SM4-TB, top entry | Same electrical ratings [PH] | $0.205 | 6.6 mm high, 8.0 mm mated [PH] | Ext, 88,671 | Lead exits straight up into the foam and pouch above the board (req §2.1) |

**Pick: side entry**, so the battery lead leaves horizontally under the pouch. 2 A ≥ the 1.16 A
worst-case peak (§11). Fallback is the top-entry part if stock runs out — decide at order time (OQ-A10).

### 9.2 LED zones Z1–Z8 (LED-6, LED-7) and speaker (AUD-3)

| Option | Key specs | Cost | Size | JLCPCB | Risks |
|---|---|---|---|---|---|
| **BM03B-SRSS-TB, top entry (pick)** | 0.7 A DC, 50 V, ≤20 mΩ, AWG #30 wire [SH] | $0.276 (× 8 = $2.21) | 4.9 mm high, 2.9 mm deep [SH] | Ext, 9,659 | Requirements assume ≈4.3 mm; leads exit upward toward the foam — bend clearance UNVERIFIED, check in the stage-2 cardboard mock-up |
| BM03B-SRSS-TBT, top entry (alternate reel) | Same [SH] | $0.332 | Same | Ext, 44,376 | Stock fallback only |
| SM03B-SRSS-TB, side entry | Same electrical [SH] | $0.475 (× 8 = $3.80) | 2.95 mm high, 4.85 mm deep [SH] | Ext, 27,210 | All eight must sit on board edges with plug clearance beyond the 45 × 36 mm outline; +$1.59 per board |
| SM03B-GHS-TB (JST-GH, locking) | Ratings UNVERIFIED (datasheet not fetched) | $0.242 | — | Ext, 13,333 | Not the family LED-6 specifies |

**Pick: BM03B-SRSS-TB (zones) and BM02B-SRSS-TB (speaker, Ext, 13,083, $0.163).** Top entry lets the
eight connectors sit in a row or block anywhere on the board instead of competing with USB-C, the
switch and the antenna for edge length (ENV-4, ENV-5). 0.7 A [SH] ≥ LED-7's 300 mA per zone and ≥ the
≈0.46 A speaker peak (§11). Pin order V+ / SIG / GND with silk Z1–Z8 (LED-6). Zone and speaker
connectors have different pin counts, so they cannot be cross-plugged.

### 9.3 USB-C (PWR-5, MCU-3)

| Option | Key specs | Cost | Size | JLCPCB | Risks |
|---|---|---|---|---|---|
| **TYPE-C-31-M-12 (pick)** | 16-pin USB 2.0; 5 A, 20 V [HRO] | $0.187 | 8.94 × 7.35 mm, 3.26 mm high [HRO] | Ext, 290,711 | Clone-grade drawing; shell tabs are through-board (machine-placed, still SMT assembly at JLC — process detail UNVERIFIED) |
| USB4105-GF-A | 5.00 A collectively on VBUS pins, 20 V DC [USB4105] | $1.02 | — | Ext, 18,579 | 5× cost for no functional difference here |

**Pick: TYPE-C-31-M-12** with R1, R2 = 5.1 kΩ from CC1 and CC2 to ground (PWR-5), so both C-to-C and
A-to-C cables supply 5 V. D+/D− pairs (A6/B6, A7/B7) are joined on the board so either plug
orientation works.

## 10. Explicit compatibility checks

### 10.1 WS2812B-type pixels: supply voltage and data level across the full discharge

Pixels are off-board and builder-supplied; two representative parts were checked.

| | WS2812B (XL-5050RGBC-WS2812B) | SK6812MINI-E |
|---|---|---|
| Supply V_DD min/typ/max | 3.5 / 5.0 / 5.5 V [WS] | 3.7 / 5.0 / 5.5 V [SK] |
| Data V_IH / V_IL | 0.7 × V_DD / 0.3 × V_DD [WS] | 0.65 × V_DD / 0.3 × V_DD [SK] |
| Current per channel | 12 mA typ → 36 mA per pixel at full white (derived) [WS] | 10 / 12 / 14.5 mA (B/G/R) [SK] |
| Standby current | 0.3 mA typ [WS] | 0.6 mA [SK] |
| T0H | 0.20 / 0.295 / 0.35 µs [WS] | 0.25 / 0.30 / 0.40 µs [SK] |

**Data level — PASS at every battery voltage.** The translator's B side is powered from the same
VBAT_SW rail as the pixel, so the logic high tracks the pixel supply:

| VBAT_SW | Pixel V_IH needed (0.7 × V_DD) [WS] | Translator V_OH ≥ V_CCB − 0.1 V [1T45 §5.5] | Margin (derived) | Direct 3.3 V GPIO, V_OH ≥ 2.64 V [WROOM Table 6-3] |
|---|---|---|---|---|
| 4.20 V | 2.94 V | 4.10 V | 1.16 V | **short by 0.30 V** |
| 3.70 V | 2.59 V | 3.60 V | 1.01 V | 0.05 V |
| 3.50 V | 2.45 V | 3.40 V | 0.95 V | 0.19 V |
| 3.35 V | 2.35 V | 3.25 V | 0.90 V | 0.29 V |
| 3.00 V | 2.10 V | 2.90 V | 0.80 V | 0.54 V |

Logic low: translator V_OL ≤ 0.1 V at 100 µA, ≤ 0.55 V even at 24 mA [1T45 §5.5], against
V_IL = 0.3 × V_DD ≥ 0.90 V. Pass. Timing: §6.2.

The last column is the stage-1 dev-kit case (requirements §10.1). The requirement's argument
"0.7 × 4.2 V = 2.94 V < 3.3 V" assumes the GPIO reaches the full 3.3 V; the datasheet only guarantees
0.8 × VDD = 2.64 V. An unloaded CMOS output normally sits close to the rail, so it will very likely
work, but it is **not within spec on paper** above V_DD ≈ 3.77 V (derived). Fine for a dev-kit stage;
the PCB does not depend on it.

**Supply voltage — OUT OF DATASHEET SPEC for part of the discharge.** The rail is the raw cell
(PWR-10): 4.2 V down to the 3.35 V cutoff (PWR-12), minus up to ≈0.2 V of path drop under load (§4).

| Pixel | In spec | Out of spec |
|---|---|---|
| WS2812B, min 3.5 V [WS] | VBAT_SW ≥ 3.5 V | 3.5 V → 3.35 V: the tail of the discharge |
| SK6812MINI-E, min 3.7 V [SK] | VBAT_SW ≥ 3.7 V | 3.7 V → 3.35 V: a large part of the discharge (share depends on the cell's curve — UNVERIFIED) |

Requirement A-2 already accepts this as an assumption ("datasheets list 3.5 V minimum"); note the
SK6812MINI-E figure is 3.7 V, not 3.5 V. Expected failure mode below minimum is colour shift (blue
and green fade first) rather than damage — UNVERIFIED, not stated in either datasheet. **Action:**
in stage 1, run the exact pixel part from a bench supply at 3.3–3.7 V before committing (OQ-A4).
A boost converter for the pixel rail would fix this but contradicts req §3.3 and is not proposed.

### 10.2 Per-GPIO and total MCU current

| GPIO load | Current (derived) | Basis |
|---|---|---|
| Zone outputs × 8 | ≈35 µA each, 0.28 mA total | 100 kΩ pull-down + ≤2 µA translator input [1T45 §5.5] |
| Status LED (GPIO2) | ≤1.5 mA | (3.3 − 1.8) V ÷ 1 kΩ [KT] |
| Amp SD_MODE (GPIO14) | ≤36 µA | 3.3 V ÷ 92 kΩ min pull-down [MAX EC] |
| CHRG, STDBY (GPIO9, 10) | ≈33 µA each, sunk by the charger, not the GPIO | 100 kΩ pull-ups |
| I²S × 3, ADC, USB, buttons | CMOS inputs / dedicated functions; negligible | — |
| **Largest single pin** | **≤1.5 mA** | |
| **Total sourced by all GPIOs** | **<2 mA** | |

Limits: per-pin source 40 mA typ, sink 28 mA typ [WROOM Table 6-3]; default drive strength 20 mA
(10 mA on GPIO17/18) [S3 Table 2-1 note]; cumulative I/O output current 1500 mA max [S3 Table 5-1].
The datasheets give no per-pin absolute maximum, only these typ figures. The heaviest pin uses <8 % of
the default drive strength and the total is ≈0.1 % of the cumulative limit. **PASS** — by design no
GPIO drives a load directly (req §1.1).

### 10.3 Audio amplifier supply and speaker impedance

| Check | Requirement / condition | Datasheet | Result |
|---|---|---|---|
| Supply range | VBAT_SW 3.0–4.2 V | V_DD 2.5–5.5 V [MAX EC] | PASS |
| Absolute maximum | 4.2 V (4.35 V at protection overcharge max [DW EC]) | V_DD 6 V [MAX AMR] | PASS |
| Speaker impedance | 8 Ω (AUD-3) | Characterised into 8 Ω + 68 µH at 3.7 V [MAX EC] | PASS |
| Output power | ≈0.5 W into 8 Ω at 3.7 V (AUD-2) | 0.77 W at 1 % THD+N [MAX EC] | PASS; ≈0.51 W at 3.0 V (derived) |
| Speaker power rating | 0.5–1 W (AUD-3) | Up to 0.93 W at 10 % THD+N [MAX EC] | A 0.5 W speaker can be overdriven → firmware cap (§7); prefer a 1 W part (OQ-A7) |
| Logic levels | 3.3 V I²S | V_IH 1.3 V, V_IL 0.6 V [MAX EC] | PASS |
| Shutdown current | ≈0 µA (AUD-6) | 0.6/2 µA [MAX EC] | PASS |
| Peak supply current | ≈0.46 A = 3.7 V ÷ 8 Ω (derived) | Current limit 2.8 A typ [MAX EC] | PASS; covered by 2 × 100 µF + 10 µF local bulk |

## 11. Power budget and runtime

**How to read this.** LED, pixel and amplifier figures come from datasheet values. The MCU figures are
**estimates**: the module datasheet gives radio TX/RX currents [WROOM Table 6-4] but not an average for
a SoftAP serving a web page, so a TX duty cycle is assumed and stated. Stage 1 must replace these with
measurements. Loadout is the requirements' reference: 6 PWM zones + 2 pixel zones of 3 pixels.

### 11.1 3V3 rail (U2 output)

| Load | Idle | Typical effect | Worst sustained | Instantaneous peak |
|---|---|---|---|---|
| ESP32-S3 module (estimate) | ≈100 mA — RX 95 mA, 2 % TX at 355 mA | ≈111 mA — 6 % TX | ≈173 mA — 30 % TX | 355 mA [WROOM Table 6-4] |
| Status LED, pull-downs, pull-ups, translator A side | ≈1 mA | ≈1.5 mA | ≈2 mA | ≈2 mA |
| **3V3 total** | **≈101 mA** | **≈112 mA** | **≈175 mA** | **≈357 mA** |

Capacity: 1 A, current limit ≥1.2 A [TLV §5.5] → the peak uses ≈36 % of the rating.

### 11.2 VBAT_SW rail (after the switch)

| Load | Idle | Typical effect | Worst sustained | Instantaneous peak | Basis |
|---|---|---|---|---|---|
| LDO input (= 3V3 load + 25 µA ground current [TLV §5.5]) | 101 mA | 112 mA | 175 mA | 357 mA | §11.1 |
| PWM zones, 6 used | 0 | 40 mA | 120 mA | 120 mA | Typical from requirements §3.4; worst 6 × 20 mA (LED-4). All 8 zones as PWM: 160 mA |
| Pixels, 6 | 1.8 mA | ≈50 mA (estimate, ≈25 % average level) | 216 mA | 216 mA | 0.3 mA standby, 36 mA full white each [WS] |
| Amplifier | ≈0 (0.6 µA shutdown) | ≈32 mA — 0.1 W average (estimate) at 90 % efficiency (assumed) + 2.4 mA I_Q | ≈233 mA — 0.77 W continuous | ≈465 mA — 3.7 V ÷ 8 Ω + I_Q | [MAX EC] |
| Divider, Q2 gate, translator B side | ≈0.05 mA | ≈0.05 mA | ≈0.05 mA | ≈0.05 mA | 14 µA + 3.8 µA + 8 × ≤4 µA [1T45 §5.5] |
| **VBAT_SW total** | **≈103 mA** | **≈234 mA** | **≈744 mA** | **≈1.16 A** | |

Cross-checks:

- Requirements §3.4 estimated ≈200 mA typical and ≈610 mA worst case. Typical agrees within the
  estimate's accuracy; **worst case is ≈22 % higher here**, mainly because 0.77 W of audio costs ≈233 mA,
  not 200 mA, and 30 % radio TX duty is assumed.
- Every series element is rated above the 1.16 A peak: JST-PH 2 A [PH]; AO3401A −4 A [AO AMR];
  FS8205A 6 A [FS AMR]; protection trip ≥2.0 A (§3.2).
- The 1.16 A peak is a coincidence of a radio burst, an audio crest and all LEDs at full white; the
  2 × 100 µF bulk (PWR-11) exists for exactly this.

### 11.3 Other rails

| Rail | Condition | Current | Basis |
|---|---|---|---|
| VBUS | Charging, switch OFF | 443/494/542 mA + I_CC 1.1/1.5 mA | [BQ], R_ISET 1.8 kΩ |
| VBUS | Switch ON | system load + charge, limited to ≈1.46 A by R_ILIM 1.1 kΩ | [BQ] |
| VBAT_RAW | Switch OFF (shelf) | ≈8–14 µA | §3.4/§3.5 |
| VBAT_SW | Switch ON, after PWR-12 deep sleep, no pixels fitted | ≈0.09 mA max (derived) | Module deep sleep 8 µA typ [WROOM Table 6-7] + LDO 25/31 µA [TLV §5.5] + divider 11 µA + gate 3 µA + translators ≤32 µA + amp ≤2 µA [MAX EC] + U4 ≤6 µA |
| VBAT_SW | Same, 6 pixels fitted | ≈1.9 mA | + 6 × 0.3 mA pixel standby [WS] |

Pixel standby dominates the post-shutdown drain: a unit left switched ON after low-battery shutdown
reaches the DW01A's 2.4–2.6 V cut-off in days rather than months (OQ-A4).

### 11.4 Estimated runtime

Usable capacity is taken as 90 % of nominal down to the 3.35 V cutoff, per requirements §3.3.
Cell capacity itself is UNVERIFIED (no cell selected), so these are estimates on estimates.

| Scenario | Current | 803450, 1500 mAh → 1350 mAh usable | 14500, 750–1000 mAh → 675–900 mAh usable |
|---|---|---|---|
| Idle (Wi-Fi up, effects off) | ≈103 mA | ≈13 h | ≈6.5–8.7 h |
| Typical effect | ≈234 mA | **≈5.8 h** | ≈2.9–3.8 h |
| Worst case sustained | ≈744 mA | **≈1.8 h** | ≈0.9–1.2 h |
| Charge time at 500 mA | — | ≈3 h constant-current + taper (estimate ≈3.5–4 h) | ≈1.5–2 h + taper |

- Typical use meets the 3 h target (req §3.4) with ≈2× margin on the pouch and is marginal on a 14500.
- **Continuous worst case does not meet 3 h on either cell.** If that matters, the lever is a firmware
  current budget (cap simultaneous pixel brightness and volume), not hardware (OQ-A12).

## 12. Envelope, assembly and cost

### 12.1 Height (ENV-3: ≤10 mm including components, one-sided)

| Tallest parts | Height | Source |
|---|---|---|
| JST-PH side entry (J2) | 5.5 mm | [PH] |
| JST-SH top entry (J3–J11) | 4.9 mm (requirements assume ≈4.3 mm) | [SH] |
| USB-C (J1) | 3.26 mm | [HRO] |
| ESP32-S3-WROOM-1 (U1) | 3.1 mm | [WROOM Table 1-1] |
| 100 µF 1206 MLCC (C29, C30) | 1.8 mm | [SAM] |
| Slide switch (SW1) | ≈1.4 mm | [MSK] |

All parts are ≤5.5 mm plus a 1.6 mm board (board thickness assumed, UNVERIFIED until ordered) →
≈7.1 mm, inside 10 mm. **The binding constraint is not the parts but the zone-lead bend radius above
the top-entry JST-SH connectors** (UNVERIFIED) — check in the stage-2 cardboard mock-up (ASM-6).

### 12.2 Area (ENV-1: ≤45 × 36 mm = 1620 mm²)

The module alone is 18.0 × 25.5 = 459 mm² (≈28 %). Eight JST-SH footprints (≈3 × 6 mm each with
pads, estimate) ≈ 150 mm². With USB-C, the switch, JST-PH, 14 ICs/FETs and ≈62 passives (mostly 0402)
the placement looks feasible on one side but is **not proven** until a first placement pass in KiCad.
That pass should happen before this document is finalised (OQ-A10).

### 12.3 Assembly (ASM-1..5)

- All 92 placements are SMD, top side only; no through-hole parts for the builder (ASM-1, ASM-4).
- 2-layer board assumed (ASM-3); the ESP32 module and class-D amp are both 2-layer-friendly per
  their reference designs (UNVERIFIED for this exact layout).
- Leadless parts: MAX98357A (TQFN-16). Everything else has visible leads or terminations for
  multimeter probing (TOOL-1).

### 12.4 Library classes and cost (ASM-2, COST-1)

| Class | Unique parts | Parts |
|---|---|---|
| Basic | 17 lines | AO3401A, all resistors, all capacitors, red LED, tactile switches |
| Extended | 13 + PTC (TBD) | ESP32-S3 module, **BQ24074RGTR**, TLV75733P, DW01A, FS8205A, MAX98357A, SN74LVC1T45, USBLC6-2, TYPE-C-31-M-12, S2B-PH, BM03B-SRSS, BM02B-SRSS, MSK12C02 (costs below predate the 2026-11-14 changes — re-quote after layout, OQ-A6) |

| Cost item (qty 5) | Per board | Basis |
|---|---|---|
| Parts, JLCPCB qty-1 prices | $11.14 | `bom-draft.csv` |
| Extended-part setup fees, 13 unique × ≈$3 ÷ 5 boards | ≈$7.80 | Fee amount and preferred-part waiver **UNVERIFIED** |
| PCB, PCBA setup, stencil, shipping | UNVERIFIED | Needs a real JLCPCB quote |
| **Rough total** | **≈$25–30** (estimate) | Inside COST-1's $40 and the requirements' $25–35 expectation, pending the quote |

The extended-part fees are the second-largest cost item. Note that PWR-2's "JLC basic parts" claim
for DW01A + FS8205A is not true on 2026-11-10 — both are extended.

## 13. Findings against requirements.md

These are places where the datasheets disagree with the requirements text, or where a requirement
cannot be met as written. None are fixed in `requirements.md`; they are for the owner to decide.

| # | Requirement | Finding | Proposed action |
|---|---|---|---|
| F-1 | PWR-2 | DW01A and FS8205A are extended, not basic, at JLCPCB (API, 2026-11-10) | Reword; accept the fee |
| F-2 | PWR-7 | No JLC-stocked SMD slide switch is rated for the ≈1.16 A peak (§3.5) | Accept FET-gated switch (OQ-A1) |
| F-3 | PWR-9 | AP2112K-3.3 does not guarantee ≤300 mV dropout at 500 mA [AP EC] | Name TLV75733P only |
| F-4 | PWR-13 | 1 MΩ / 1 MΩ needs the ADC's 0–2900 mV range: ±50 mV at the pin = ±100 mV at VBAT [S3 Table 5-6]; 200 kΩ / 100 kΩ uses the ±10 mV range = ±30 mV at VBAT | Change the example (OQ-A8) |
| F-5 | LED-3, LED-8 | 68 Ω gives 62 mA into a short, above the translator's ±50 mA absolute maximum [1T45 §5.1]; LED-8's "within absolute max" does not hold. A 0402/0603 resistor also cannot dissipate the short | 100 Ω, 1206 (OQ-A2) |
| F-6 | LED-4 | With 68 Ω a red LED at 4.2 V draws up to 35 mA, above the 20 mA zone limit | 100–120 Ω (OQ-A2) |
| F-7 | §10.1 | Stage-1 direct-drive "within spec" argument assumes V_OH = 3.3 V; the datasheet guarantees 2.64 V [WROOM Table 6-3] | Reword as "typically works" |
| F-8 | A-2 | SK6812MINI-E minimum supply is 3.7 V [SK], not 3.5 V | Update the assumption (OQ-A4) |
| F-9 | §6.1, AUD-4 | 22.05 kHz LRCLK is explicitly not supported by the MAX98357A [MAX "LRCLK Polarity"] | 44.1 kHz output or 16/32/44.1 kHz clips (OQ-A7) |
| F-10 | FW-1 | "Left/right test tone" — the amplifier is mono | Single test tone (OQ-A7) |
| F-11 | MCU-7 | GPIO26–34 are not pinned out on the module; IO35–37 are pinned but unusable on R8 [WROOM Table 3-1] | Clarify wording |
| F-12 | ENV-3 | JST-SH top-entry height is 4.9 mm [SH], not ≈4.3 mm | Update the figure |
| F-13 | §3.4 | Worst case is ≈744 mA sustained / ≈1.16 A peak, not ≈610 mA (§11.2) | Update the estimate (OQ-A12) |
| F-14 | OQ-5 | Closed by this document: MAX98357A (§7) | Mark closed if accepted |

## 14. Sources

All datasheets below were downloaded and read on 2026-11-10. LCSC copies are used where the
manufacturer site was blocked or the part is a Chinese-vendor part with no other source.

| Key | Part / document | Revision | URL |
|---|---|---|---|
| WROOM | ESP32-S3-WROOM-1 / WROOM-1U Datasheet | v1.8 | https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf |
| S3 | ESP32-S3 Series Datasheet | v2.2 | https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf |
| MINI | ESP32-S3-MINI-1 / MINI-1U Datasheet | v1.7 | https://www.espressif.com/sites/default/files/documentation/esp32-s3-mini-1_mini-1u_datasheet_en.pdf |
| MAX | MAX98357A/MAX98357B (Analog Devices) | 19-6779 Rev 11 | https://datasheet.lcsc.com/datasheet/pdf/9dd365a93d191d8d6f09766b0e3ea172.pdf?productCode=C910544 |
| NS | NS4168 (Nsiway) | V1.2 | https://datasheet.lcsc.com/datasheet/pdf/d3e8c6e77860e1345ae29923805289c4.pdf?productCode=C910588 |
| 1T45 | SN74LVC1T45 (TI) | SCES515N | https://www.ti.com/lit/ds/symlink/sn74lvc1t45.pdf |
| 2T45 | SN74LVC2T45 (TI) | SCES516N | https://www.ti.com/lit/ds/symlink/sn74lvc2t45.pdf |
| 8T245 | SN74LVC8T245 (TI) | SCES584D | https://www.ti.com/lit/ds/symlink/sn74lvc8t245.pdf |
| TLV | TLV757P (TI) | SBVS322C | https://www.ti.com/lit/ds/symlink/tlv757p.pdf |
| AP | AP2112 (Diodes) | DS39724 Rev 2-2 | https://www.diodes.com/assets/Datasheets/AP2112.pdf |
| RT | RT9080 (Richtek) | DS9080-05 | https://datasheet.lcsc.com/datasheet/pdf/2701e0c3a2b84b9965f7a61f2c14e970.pdf?productCode=C841192 |
| ME | ME6217 (Microne) | as published on LCSC | https://datasheet.lcsc.com/datasheet/pdf/3d38df48f5744f9f5473666520a2674d.pdf?productCode=C427602 |
| TPS | TPS63802 (TI) | ZHCSJ11D | https://www.ti.com/lit/ds/symlink/tps63802.pdf |
| BQ | TI BQ24072/3/4/5/9 datasheet (BQ24074 variant used) | SLUS810N, Oct 2021 | https://www.ti.com/lit/ds/symlink/bq24074.pdf |
| TP | TP4056 (NanJing Top Power) — superseded 2026-11-14, kept for the comparison | REV_2.4 | https://datasheet.lcsc.com/datasheet/pdf/d328ee101cbb1f3719611cd61db479c2.pdf?productCode=C16581 |
| TP57 | TP4057 (NanJing Top Power) | as published on LCSC | https://datasheet.lcsc.com/datasheet/pdf/a06f26a534534913a54f86485e56d5f7.pdf?productCode=C12044 |
| MCP | MCP73831/2 (Microchip) | DS20001984G | https://datasheet.lcsc.com/datasheet/pdf/c7ec00710e742b7fb9393e620ae1332b.pdf?productCode=C424093 |
| BQ | BQ24074 (TI) | SLUS810N | https://www.ti.com/lit/ds/symlink/bq24074.pdf |
| DW | DW01A (PUOLOP) | Rev B, 2016-4-12 | https://datasheet.lcsc.com/datasheet/pdf/0d2b2b5e8d1207bf276387cb4ff3a495.pdf?productCode=C351410 |
| FS | FS8205A (FUXINSEMI) | Ver 2.1 | https://datasheet.lcsc.com/datasheet/pdf/5d5d336ed528176185784e3faf8fb551.pdf?productCode=C908265 |
| XB89 | XB8089D (XySemi) | REV0.7 | https://datasheet.lcsc.com/datasheet/pdf/db530d3161a3463895737f9585084d1a.pdf?productCode=C79928 |
| XB53 | XB5353A (XySemi) | REV0.4 | https://datasheet.lcsc.com/datasheet/pdf/c17a78852ebc4428bb7f125f4725039c.pdf?productCode=C80628 |
| BQ297 | BQ29700 (TI) | as published on LCSC | https://datasheet.lcsc.com/datasheet/pdf/2eb82e7000dd0ebafe2a37c842a90ea8.pdf?productCode=C183096 |
| AO | AO3401A (Alpha & Omega) | Rev 3.1 | https://datasheet.lcsc.com/datasheet/pdf/fee353dd1e9e0bc90b295f14f381aa4c.pdf?productCode=C15127 |
| SI | SI2301CDS (Vishay) | Doc 68741, S10-2430-Rev. C | https://datasheet.lcsc.com/datasheet/pdf/f80dd06337c57ccfc2bd2185d147b710.pdf?productCode=C10487 |
| B58 | B5819W (Jiangsu Changjiang) | as published on LCSC | https://datasheet.lcsc.com/datasheet/pdf/4ac2c059be7c462694ab0715ce987a85.pdf?productCode=C8598 |
| WS | XL-5050RGBC-WS2812B (XINGLIGHT) | as published on LCSC | https://datasheet.lcsc.com/datasheet/pdf/40b65f6fea0587c0483bbbd4496835fd.pdf?productCode=C2843785 |
| SK | SK6812MINI-E (OPSCO) | Rev B/1 | https://datasheet.lcsc.com/datasheet/pdf/0907a34a701b4aef568078003f6e9eae.pdf?productCode=C5149201 |
| PH | JST PH series (S2B-PH-SM4-TB; B2B-PH-SM4-TB) | JST catalog | https://datasheet.lcsc.com/datasheet/pdf/2474e9261ed7c5f86b55d373e724589d.pdf?productCode=C295747 ; https://datasheet.lcsc.com/datasheet/pdf/6c6fce4367275d84ec6a99bf09f1250f.pdf?productCode=C160352 |
| SH | JST SH series (BM03B/SM03B/BM02B-SRSS-TB) | JST catalog | https://datasheet.lcsc.com/datasheet/pdf/e03a82bcc66e3c3589917313e54d45e2.pdf?productCode=C160389 |
| MSK | MSK12C02 (SHOUHAN) | drawing | https://datasheet.lcsc.com/datasheet/pdf/5162155576bfd231c35aa9a893d25c8c.pdf?productCode=C431540 |
| JS | JS102011SAQN (C&K) | as published on LCSC | https://datasheet.lcsc.com/datasheet/pdf/facc684708fd3febef29363c3f43748d.pdf?productCode=C221660 |
| PCM | PCM12SMTR | as published on LCSC | https://datasheet.lcsc.com/datasheet/pdf/677f227074f0d201058521a10a93c772.pdf?productCode=C221841 |
| HRO | TYPE-C-31-M-12 | drawing | https://datasheet.lcsc.com/datasheet/pdf/9e56b777c022540fcce7c7f67825f55e.pdf?productCode=C165948 |
| USB4105 | USB4105-GF-A (GCT) | drawing | https://datasheet.lcsc.com/datasheet/pdf/76718c6dacc0a6125953c12b5ebc82df.pdf?productCode=C3020560 |
| ESD | USBLC6-2 (ST) | as published on LCSC | https://datasheet.lcsc.com/datasheet/pdf/0d3a2ab954b34651a0695e7ccf534db0.pdf?productCode=C7519 |
| KT | KT-0603R | as published on LCSC | https://datasheet.lcsc.com/datasheet/pdf/011ec3e8cb1e825f6961d29bc4db4c7a.pdf?productCode=C2286 |
| UNI | Uniroyal thick-film chip resistors | as published on LCSC | https://datasheet.lcsc.com/datasheet/pdf/0a975aaa49b7c97f38a963127be4a823.pdf?productCode=C25741 |
| SAM | Samsung MLCC (CL-series) | as published on LCSC | https://datasheet.lcsc.com/datasheet/pdf/02336ea48ea44ca18c72517dd3cb7b47.pdf?productCode=C15008 |
| TAJ | Kyocera AVX TAJ tantalum (considered for bulk, rejected) | — | https://datasheets.kyocera-avx.com/TAJ.pdf |

Not verified from a datasheet (image-only or not fetched): TS-1187A contact rating, SS-12D10L5
rating, XC6220 (image-only, not used), ESP32-C6-MINI-1 specifications, DevKitC-1 pinout, Espressif
Hardware Design Guidelines (antenna keep-out), JLCPCB fee schedule.

## 15. Open questions — answered 2026-11-14

All fifteen were answered by the owner. The questions as originally asked are in git history.

| # | Decision | Applied in |
|---|---|---|
| OQ-A1 | Slide switch drives the Q2 P-FET; the switch carries gate current only | §3.5, PWR-7 |
| OQ-A2 | R_Z = 120 Ω 1206 | §6.2, LED-3/4/8 |
| OQ-A3 | Power-path charger BQ24074RGTR | §3.4, PWR-16 |
| OQ-A4 | Accept pixels below their minimum supply; measure on the stage-1 dev kit; no pixel-power FET | A-2 |
| OQ-A5 | Keep the Q1 P-FET topology; SPICE + one sacrificial board before quantity — re-check against the BQ24074 | §3.3, §3.4 |
| OQ-A6 | Accept extended parts; produce Gerber/BOM/CPL after layout for a real JLC quote | §12.4 |
| OQ-A7 | 44.1 kHz output, one mono test tone, 6 dB gain + firmware cap ≈0.5 W, **1 W speaker** | §7, AUD-3/4, FW-1 |
| OQ-A8 | 200 kΩ / 100 kΩ divider + one-point calibration in FW-1 | PWR-13 |
| OQ-A9 | 10 V-rated bulk MLCCs (part TODO (verify) at JLC; add a third if no 10 V 100 µF is stocked) | PWR-11, BOM |
| OQ-A10 | Connector orientation decided during layout from the 65 × 40 × 30 mm cavity; **prefer top-entry** where it fits | §9.2 — layout |
| OQ-A11 | Genuine ST USBLC6-2SC6 | §9.3 |
| OQ-A12 | **No** firmware power cap; 3 h target is for typical use | §11.4, §3.4 of requirements |
| OQ-A13 | **Adafruit #2011, 2000 mAh LiPo, 60 × 36 × 7 mm, JST-PH, own PCM** (chosen 2026-11-14 from the candidates below) | PWR-4, BOM BT1 |
| OQ-A14 | Spare GPIOs (3, 18, 21, 38–42, 47, 48) to test pads; USB-present = BQ24074 PGOOD on GPIO10 | §3.4, MCU-7 |
| OQ-A15 | One resettable PTC on the shared zone V+ | §6.2, LED-9 |

### Remaining before layout

1. ~~Verify LCSC#/stock~~ — done 2026-11-14 (BOM). Bulk caps: three fitted instead of verifying the derating curve (§4).
2. ~~Dropout budget with the BQ battery FET~~ — done (§4): 3.35 V cutoff unchanged; load-step margin 0.15 V of 0.2 V.
3. Reverse-battery re-check — bound unchanged (§3.3); **SPICE still required** for the BQ24074 BAT pin's −0.3 V limit (OQ-A5).
4. ~~PTC drop~~ — nSMD075 is 90 mΩ min / 500 mΩ max after a trip → 27–150 mV at 300 mA on zone V+, which lowers pixel supply further below spec (A-2). Accepted with A-2; re-measure on the dev kit.
5. ~~Cell~~ — Adafruit #2011 chosen (OQ-A13). Confirm its discharge rating covers the 1.16 A peak when bought.

### Cell candidates (OQ-A13, 2026-11-14)

All have a 2-pin JST-PH lead and their own protection board (a second layer under U4/Q3 — adds ≈50–60 mΩ of
path resistance and cuts off at ≈2.75–3.0 V, both below the firmware's 3.35 V cutoff, so harmless).

| Option | Size (mm) | Capacity | Discharge | Pros / cons |
|---|---|---|---|---|
| **Adafruit #2011 LiPo 2000 mAh** | 60 × 36 × 7 | 2000 mAh | not stated; Adafruit's own cells are typically 1C (≈2 A) — UNVERIFIED | Fits the 65 × 40 cavity footprint flat; known-good JST-PH **polarity** (matches the board); reputable; sold at Micro Center. Some resellers list it retired (replacement CE04378) — check stock. ≈8 h typical, ≈2.4 h worst case |
| Generic **803450 1500 mAh** with JST-PH 2.0 (eBay/Amazon/LiPo makers) | 8 × 34 × 50 | 1500 mAh | sellers list 1C (1.5 A) max; some 0.5C | The size requirements.md was written around; cheap. Sellers disagree on specs and **lead polarity is not guaranteed** — check with the multimeter before plugging in (PWR-3 protects the board anyway). Avoid listings rated 0.5C (750 mA < 1.16 A peak) |
| 603450 / 503562 1200 mAh class with PCM + JST-PH | ≈6 × 34 × 50–62 | 1200 mAh | 1C (1.2 A) | Thinner, fallback if height is tight; meets the 1.2 A peak with no margin; ≈6 h typical |
