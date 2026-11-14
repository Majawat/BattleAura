"""BattleAura v2 board, as code (SKiDL).

Source of truth for every connection is docs/architecture.md (section numbers in comments) and the
requirement IDs in docs/requirements.md. Run with the repo's hardware venv:

    KICAD9_SYMBOL_DIR=/usr/share/kicad/symbols KICAD9_FOOTPRINT_DIR=/usr/share/kicad/footprints \
        .venv-hw/bin/python hardware/battleaura_v2.py

Outputs hardware/battleaura_v2.net (KiCad netlist for layout) and runs SKiDL's ERC.
"""

import os
import builtins
from skidl import ERC, KICAD9, Net, Part, Pin, generate_netlist, lib_search_paths, set_default_tool

set_default_tool(KICAD9)
NC = builtins.NC  # SKiDL installs its no-connect net in builtins
HERE = os.path.dirname(os.path.abspath(__file__))
lib_search_paths[KICAD9].append(os.path.join(HERE, "lib"))

R0402 = "Resistor_SMD:R_0402_1005Metric"
R1206 = "Resistor_SMD:R_1206_3216Metric"
C0402 = "Capacitor_SMD:C_0402_1005Metric"
C0603 = "Capacitor_SMD:C_0603_1608Metric"
C0805 = "Capacitor_SMD:C_0805_2012Metric"
C1206 = "Capacitor_SMD:C_1206_3216Metric"


def R(value, fp=R0402, lcsc=""):
    return Part("Device", "R", value=value, footprint=fp, LCSC=lcsc)


def C(value, fp=C0402, lcsc=""):
    return Part("Device", "C", value=value, footprint=fp, LCSC=lcsc)


def cap_to(a, b, value, fp=C0402, lcsc="C1525"):
    c = C(value, fp, lcsc)
    c[1] += a
    c[2] += b
    return c


# ---- nets ------------------------------------------------------------------------------------
GND = Net("GND")            # system ground = Q3 S2 side (pack negative)
CELL_N = Net("CELL_N")      # cell negative, before the protection FETs (U4 GND, Q3 S1)
CELL_P = Net("CELL_P")      # connector +, before reverse-polarity FET Q1
VBAT_RAW = Net("VBAT_RAW")  # after Q1; U3 BAT, U4 VCC (via R4)
VBUS = Net("VBUS")
VSYS = Net("VSYS")          # U3 OUT: 4.4 V on USB, ~VBAT on battery
VBAT_SW = Net("VBAT_SW")    # after the Q2 load switch
V3V3 = Net("+3V3")
ZONE_VP = Net("ZONE_V+")    # shared zone V+ after the PTC (LED-9)
for n in (GND, VBUS, VBAT_SW, V3V3, VSYS, VBAT_RAW, CELL_N, ZONE_VP):
    n.drive = 99  # power nets: suppress "no driver" ERC noise on supplies fed through passives

# ---- J2 battery, U4/Q3 protection, Q1 reverse polarity (§3.1-3.3) ----------------------------
# Polarity matches Adafruit LiPo cells (checked 2026-11-14): Adafruit's Feather footprint puts "+" on the
# contact that is on the LEFT when facing the connector opening, which JST's S2B-PH-SM4-TB drawing (and
# KiCad's footprint, which follows it) numbers circuit 1. So pin 1 = +, pin 2 = -. Silkscreen +/- at J2.
J2 = Part("Connector_Generic", "Conn_01x02", ref="J2", value="BATT",
          footprint="Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal", LCSC="C295747")
J2[1] += CELL_P
J2[2] += CELL_N

U4 = Part("Battery_Management", "DW01A", ref="U4", LCSC="C351410")
Q3 = Part("battleaura", "FS8205A", ref="Q3", LCSC="C908265")
R4 = R("100", lcsc="C25076")
R5 = R("1k", lcsc="C11702")
R4[1] += VBAT_RAW
R4[2] += U4["VCC"]
U4["VCC"].net.drive = 99   # fed through R4 by design (DW01A application circuit)
cap_to(U4["VCC"], CELL_N, "100n")
U4["GND"] += CELL_N
U4["OD"] += Q3["G1"]
U4["OC"] += Q3["G2"]
U4["CS"] += R5[1]
R5[2] += GND
U4["TD"] += NC                # open: internal pull-down to VSS (DW01A datasheet block diagram)
Q3["S1"] += CELL_N
Q3["S2"] += GND
Q3[2, 5] += Net("Q3_D")     # common drain, both pins joined; not connected elsewhere

Q1 = Part("Transistor_FET", "AO3401A", ref="Q1", LCSC="C15127")
Q1["D"] += CELL_P
Q1["S"] += VBAT_RAW
Q1["G"] += CELL_N            # gate to battery-connector negative, not GND (§3.3)

# ---- J1 USB-C + U14 ESD (§9.3) ---------------------------------------------------------------
J1 = Part("Connector", "USB_C_Receptacle_USB2.0_16P", ref="J1",
          footprint="Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12", LCSC="C165948")
USB_DP, USB_DN = Net("USB_D+"), Net("USB_D-")
J1["VBUS"] += VBUS
J1["GND"] += GND
J1["SHIELD"] += GND
J1["D+"] += USB_DP           # A6/B6 joined: either plug orientation
J1["D-"] += USB_DN
J1["SBU1"] += NC
J1["SBU2"] += NC
for cc, lcsc in (("CC1", "C25905"), ("CC2", "C25905")):
    r = R("5.1k", lcsc=lcsc)
    r[1] += J1[cc]
    r[2] += GND
U14 = Part("Power_Protection", "USBLC6-2SC6", ref="U14", LCSC="C7519")
U14["VBUS"] += VBUS
U14["GND"] += GND
U14[1, 6] += USB_DN          # I/O1 pass-through
U14[3, 4] += USB_DP          # I/O2 pass-through

# ---- U3 BQ24074 power-path charger (§3.4) ----------------------------------------------------
U3 = Part("Battery_Management", "BQ24074RGT", ref="U3", LCSC="C54313")
CHG_N, PGOOD_N = Net("CHG_N"), Net("PGOOD_N")
U3["IN"] += VBUS
U3["OUT"] += VSYS
U3["BAT"] += VBAT_RAW
U3["VSS"] += GND
U3["EN2"] += VSYS            # ILIM-resistor mode; NOT 3V3 (would drop to USB100 with switch OFF)
U3["EN1"] += GND
U3["~{CE}"] += GND           # always charge
U3["~{CHG}"] += CHG_N
U3["~{PGOOD}"] += PGOOD_N    # USB present (OQ-A14)
r = R("1.8k", lcsc="C25871");  r[1] += U3["ISET"]; r[2] += GND   # ~494 mA charge
r = R("1.1k", lcsc="C25860");  r[1] += U3["ILIM"]; r[2] += GND   # ~1.46 A input limit
r = R("10k", lcsc="C25744");   r[1] += U3["TS"];   r[2] += GND   # no thermistor
U3["TMR"] += NC              # open: default safety timers
U3["ITERM"] += NC            # open: default 10 % termination
cap_to(VBUS, GND, "10u", C0603, "C19702")
cap_to(VBAT_RAW, GND, "10u", C0603, "C19702")
cap_to(VSYS, GND, "22u", C0805, "C45783")

# ---- Q2 load switch + SW1 (§3.5) -------------------------------------------------------------
Q2 = Part("Transistor_FET", "AO3401A", ref="Q2", LCSC="C15127")
Q2_G = Net("Q2_G")
Q2["S"] += VSYS
Q2["D"] += VBAT_SW
Q2["G"] += Q2_G
r = R("1M", lcsc="C26083");    r[1] += Q2_G; r[2] += VSYS
cap_to(Q2_G, VSYS, "1u", C0402, "C52923")                         # soft start
SW1 = Part("Switch", "SW_SPDT", ref="SW1", footprint="Button_Switch_SMD:SW_SPDT_Shouhan_MSK12C02",
           LCSC="C431540")
SW1["B"] += Q2_G             # common
r = R("100k", lcsc="C25741");  r[1] += SW1["A"]; r[2] += GND      # ON throw
r = R("1k", lcsc="C11702");    r[1] += SW1["C"]; r[2] += VSYS     # OFF throw

# ---- VBAT_SW rail: bulk, gauge, PTC (§4, PWR-11/13, LED-9) -----------------------------------
for _ in range(3):
    cap_to(VBAT_SW, GND, "100u 10V", C1206, "C883598")
VBAT_SENSE = Net("VBAT_SENSE")
r = R("200k", lcsc="C25764");  r[1] += VBAT_SW; r[2] += VBAT_SENSE
r = R("100k", lcsc="C25741");  r[1] += VBAT_SENSE; r[2] += GND
cap_to(VBAT_SENSE, GND, "100n")
F1 = Part("Device", "Polyfuse", ref="F1", value="0.75A hold", footprint="Fuse:Fuse_1206_3216Metric",
          LCSC="C70078")
F1[1] += VBAT_SW
F1[2] += ZONE_VP

# ---- U2 LDO (§4) -----------------------------------------------------------------------------
U2 = Part("Regulator_Linear", "TLV75733PDBV", ref="U2", LCSC="C485517")
U2["IN"] += VBAT_SW
U2["EN"] += VBAT_SW
U2["GND"] += GND
U2["OUT"] += V3V3
cap_to(VBAT_SW, GND, "1u", C0402, "C52923")
cap_to(V3V3, GND, "22u", C0603, "C59461")

# ---- U1 ESP32-S3-WROOM-1-N16R8 (§5, pin map §5.1) --------------------------------------------
U1 = Part("RF_Module", "ESP32-S3-WROOM-1", ref="U1", value="ESP32-S3-WROOM-1-N16R8", LCSC="C2913202")
U1["3V3"] += V3V3
U1["GND"] += GND
cap_to(V3V3, GND, "10u", C0603, "C19702")
cap_to(V3V3, GND, "100n")
EN = Net("EN")
U1["EN"] += EN
r = R("10k", lcsc="C25744");   r[1] += EN; r[2] += V3V3
cap_to(EN, GND, "1u", C0402, "C52923")
SW3 = Part("Switch", "SW_Push", ref="SW3", value="RESET",
           footprint="Button_Switch_SMD:SW_Push_1P1T_XKB_TS-1187A", LCSC="C318884")
SW3[1] += EN
SW3[2] += GND
IO0 = Net("IO0")
U1["IO0"] += IO0
SW2 = Part("Switch", "SW_Push", ref="SW2", value="BOOT",
           footprint="Button_Switch_SMD:SW_Push_1P1T_XKB_TS-1187A", LCSC="C318884")
SW2[1] += IO0
SW2[2] += GND
U1["USB_D-"] += USB_DN
U1["USB_D+"] += USB_DP
U1["IO1"] += VBAT_SENSE
U1["IO9"] += CHG_N
U1["IO10"] += PGOOD_N
for n in (CHG_N, PGOOD_N):
    r = R("100k", lcsc="C25741"); r[1] += n; r[2] += V3V3
# status LED (UI-1)
D1 = Part("Device", "LED", ref="D1", value="red", footprint="LED_SMD:LED_0603_1608Metric", LCSC="C2286")
r = R("1k", lcsc="C11702");    r[1] += U1["IO2"]; r[2] += D1["A"]
D1["K"] += GND

# ---- U5 MAX98357A audio (§7) -----------------------------------------------------------------
U5 = Part("Audio", "MAX98357A", ref="U5", LCSC="C910544")
U5["VDD"] += VBAT_SW
U5["GND"] += GND
U5["PAD"].func = Pin.types.PASSIVE   # exposed pad is GND; stock symbol types it UNSPECIFIED
U5["PAD"] += GND
U5["GAIN_SLOT"] += VBAT_SW   # tied to VDD = 6 dB
for nc in (5, 6, 12, 13):
    U5[nc] += NC
U5["LRCLK"] += U1["IO11"]
U5["BCLK"] += U1["IO12"]
U5["DIN"] += U1["IO13"]
U5["~{SD_MODE}"] += U1["IO14"]
cap_to(VBAT_SW, GND, "10u", C0603, "C19702")
cap_to(VBAT_SW, GND, "100n")
J11 = Part("Connector_Generic", "Conn_01x02", ref="J11", value="SPEAKER",
           footprint="Connector_JST:JST_SH_BM02B-SRSS-TB_1x02-1MP_P1.00mm_Vertical", LCSC="C160388")
J11[1] += U5["OUTP"]
J11[2] += U5["OUTN"]

# ---- LED zones Z1-Z8 (§6, LED-1..9) ----------------------------------------------------------
ZONE_GPIO = ["IO4", "IO5", "IO6", "IO7", "IO15", "IO16", "IO17", "IO8"]
for k, gpio in enumerate(ZONE_GPIO, start=1):
    u = Part("Logic_LevelTranslator", "SN74LVC1T45DBV", ref=f"U{5 + k}", LCSC="C7843")
    zin, zsig = Net(f"Z{k}_IN"), Net(f"Z{k}_SIG")
    u["VCCA"] += V3V3
    u["VCCB"] += VBAT_SW
    u["GND"] += GND
    u["DIR"] += V3V3          # A -> B
    u["A"] += zin
    U1[gpio] += zin
    r = R("100k", lcsc="C25741"); r[1] += zin; r[2] += GND        # LED-5 pull-down
    rz = R("120", R1206, "C17909"); rz[1] += u["B"]; rz[2] += zsig
    cap_to(V3V3, GND, "100n")
    cap_to(VBAT_SW, GND, "100n")
    j = Part("Connector_Generic", "Conn_01x03", ref=f"J{2 + k}", value=f"Z{k}",
             footprint="Connector_JST:JST_SH_BM03B-SRSS-TB_1x03-1MP_P1.00mm_Vertical", LCSC="C160389")
    j[1] += ZONE_VP           # V+ / SIG / GND (LED-6)
    j[2] += zsig
    j[3] += GND

# ---- test pads (MCU-6, OQ-A14) ---------------------------------------------------------------
pads = {"TXD0": U1["TXD0"], "RXD0": U1["RXD0"], "3V3": V3V3, "GND": GND, "EN": EN, "IO0": IO0}
for io in ["IO3", "IO18", "IO21", "IO38", "IO39", "IO40", "IO41", "IO42", "IO47", "IO48"]:
    pads[io] = U1[io]
for name, target in pads.items():
    tp = Part("Connector", "TestPoint", value=name, footprint="TestPoint:TestPoint_Pad_D1.0mm")
    tp[1] += target
for io in ["IO45", "IO46", "IO35", "IO36", "IO37"]:   # strapping / octal PSRAM: leave open
    U1[io] += NC

if __name__ == "__main__":
    # Stable tags = refs, so KiCad's "Update PCB from netlist" keeps placements across re-runs.
    # Refs are deterministic because parts are created in a fixed order above.
    for part in builtins.default_circuit.parts:
        part.tag = part.ref
        if getattr(part, "LCSC", ""):
            part.fields["LCSC"] = part.LCSC   # JLC assembly BOM column
    ERC()
    generate_netlist(file_=os.path.join(HERE, "battleaura_v2.net"))
