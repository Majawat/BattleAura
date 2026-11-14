# BattleAura v2 — Hardware Requirements

**Status:** v0.6 — 2026-11-14 — architecture open questions OQ-A1…A15 decided (§12, Resolved) and datasheet findings F-1…F-14 from `architecture.md` §13 applied. Ready for schematic capture and layout.
**Scope:** A from-scratch design for BattleAura v2. It's a battery-powered lighting and sound controller that hides inside a tabletop vehicle and is **fully usable on its own** from a phone. It can **optionally** connect to BattleSync later for automatic effects.
**Starting point:** a paper design. No hardware, vehicle kit, cells, LEDs, or test equipment exist yet, and nothing from the earlier BattleAura repo is carried forward. Every physical figure here is therefore a published spec or an estimate. §11 lists them, and §10.1 says how each one is checked before money is spent on PCBs.
**Audience:** Me, plus hobbyists who find the open-hardware release online.

Keywords: **MUST** = required for v2.0. **SHOULD** = strongly wanted, can slip with a reason. **MAY** = nice to have. **LATER** = designed-for but not built in v2.0.
Each requirement has an ID (`PWR-3`) so issues, the schematic, and firmware can reference it.

---

## 1. Product summary

A small board, roughly matchbox-sized, that hides inside a tabletop vehicle hull. The first target is the **One Page Rules Battle Sisters Infernal Tank**. It has:

- **8 universal LED zones.** Software sets each one to plain single-color (PWM) *or* WS2812-style addressable.
- **I²S audio** from clips stored in flash, played through a small class-D amp and speaker at close-range volume.
- **Standalone control.** The board hosts its own Wi-Fi network and web UI. A phone connects and gets a control panel: buttons for effects and sounds, scenes, volume, battery level. No internet, server, or BattleSync needed.
- **Optional BattleSync link (LATER).** Firmware can connect out to BattleSync over WebSocket and fire effects automatically from battle events. The hardware needs nothing extra for this.
- **USB-C** for charging and flashing. A hard power switch. On-board battery protection and a battery gauge.
- Factory-assembled by **JLCPCB PCBA**. The builder only plugs in pre-crimped connectors.

### 1.1 Design principles

- **First-board success over optimization.** Where two designs both work, pick the one with fewer ways to be wrong (datasheet-guaranteed behavior, few unique parts, no analog margins), even if it costs a little more or loses a little efficiency.
- **No hand assembly of electronics.** The board arrives finished. The builder only plugs in connectors and splices wires.
- **Every output buffered and current-limited.** A wiring mistake shouldn't destroy the MCU.
- **No removable media.** Audio lives in on-board flash, not on a microSD/MP3-player module.
- **The board protects the cell.** Protection, gauge, and clean low-battery shutdown are on the board, so the cell choice is free.
- **No shared default credentials.**
- **Testable without lab gear.** The firmware can check the board itself (§10.1), because the only test instrument is a multimeter (no oscilloscope).

---

## 2. Physical envelope

| ID | Requirement |
|---|---|
| ENV-1 | **MUST** fit, together with the battery, speaker, and wiring, inside the **OPR Battle Sisters Infernal Tank** interior cavity: **65 mm deep × 40 mm wide × 30 mm tall** (measured). The hull must close with no visible external modification other than ENV-4 openings. |
| ENV-2 | PCB outline **≤ 45 × 36 mm**, long side along the 65 mm depth. That leaves a ≈ 20 × 40 mm floor strip at one end for the speaker and ≈ 2 mm per side for wiring. The MCU module alone is 18 × 25.5 mm (MCU-1). |
| ENV-3 | **≤ 10 mm** tall including components, so the battery can stack above the board within the 30 mm height (§2.1). Expected tallest parts: USB-C receptacle (≈ 3.3 mm), JST-SH vertical (≈ 4.9 mm per datasheet), slide switch. All components on one side. |
| ENV-4 | **MUST** put the USB-C receptacle and power switch on the **same board edge**, so a single slot in the hull underside or a hidden hatch reaches both. |
| ENV-5 | **MUST** keep the module antenna at a board edge, with the manufacturer's copper keep-out. **SHOULD** be oriented away from the battery, speaker magnet, and any metallic paint/foil. |
| ENV-6 | **MUST** include 2× M2 mounting holes, which mate with printed standoffs (ENV-8). |
| ENV-8 | The hull is **3D-printed from OPR's model file**, so integration is by **CAD, not drilling**. Make a private edited copy of the hull with:<br>• the USB-C + switch slot (ENV-4)<br>• a speaker grille<br>• board standoffs<br>• a battery retention lip/strap slot<br>Design the battery/board tray as a **separate original part** if possible, since that part *can* be published (§14.4). |
| ENV-7 | Battery and speaker live **off-board** on leads, so they can go wherever the hull has room. |

### 2.1 Cavity packing plan (65 D × 40 W × 30 H mm)

```
 side view (depth →)                         height
 ┌─────────────────────────────────────────┐  30
 │  (≈ 8 mm free: wire slack / foam)       │
 │  ┌──────────── pouch cell ───────────┐  │  ≈ 20
 │  │ 50 × 34 × 8 mm (803450)           │  │
 │  └───────────────────────────────────┘  │  ≈ 12
 │  ┌──────── PCB ≤ 45 × 36 ────────┐ ┌──┐ │  ≈ 10
 │  │ module · amp · connectors     │ │SPK│ │
 └──┴───────────────────────────────┴─┴──┴─┘   0
    0                            45   65 mm
```

- Board on the floor; the pouch cell sits on a 2 mm foam pad above the components.
- Speaker in the 20 mm strip at one end, facing a wall or the floor, with a grille printed into the hull (ENV-8, OQ-13).
- 14500 fallback: holder ≈ 57 × 17 × 16 mm stacks above the board at ≈ 26 mm total height (fits, but less slack).

---

## 3. Power

### 3.1 Battery

| ID | Requirement |
|---|---|
| PWR-1 | **MUST** run from a single **1S Li-ion/LiPo cell (3.0–4.2 V)**. A LiPo pouch, or a 14500/18650 in a holder, must all work through one **JST-PH 2.0 2-pin** connector. |
| PWR-2 | **MUST** include **on-board cell protection**: over-discharge cutoff (≈2.4–2.5 V), over-charge, over-current/short. Example: DW01A + FS8205A (JLC *extended* parts; the setup fee is accepted, OQ-A6). Unprotected cells are therefore safe to use. |
| PWR-3 | **MUST** survive a reverse-polarity battery lead without damage. Cheap pouch leads are sometimes wired backwards despite the keyed connector. Example: P-FET reverse-protection on the battery input. |
| PWR-4 | Recommended cell for the Infernal Tank: **803450-class LiPo pouch, ≈ 1500 mAh (8 × 34 × 50 mm), with a JST-PH 2.0 lead**. Why (cavity 65 × 40 × 30 mm):<br>• An **18650 does not fit**: the cell alone is 65 mm and its holder ≈ 77 mm.<br>• The **14500 you own** fits (holder ≈ 57 × 17 × 16 mm) but a genuine one holds only ≈ 750–1000 mAh. That covers the *typical* 3 h load (≈ 600 mAh + reserve) with thin margin, and not the worst case.<br>• The pouch fits flat above the board (§2.1) and gives ≈ 2× the 14500's capacity in the same space: typical-case runtime ≈ 6 h. Worst case ≈ 1.8 h (≈ 744 mA sustained, `architecture.md` §11), but that means every light at maximum with full-volume audio continuously, which no real scene does.<br>• On-board protection (PWR-2) makes a bare pouch safe electrically. Mechanically, pad it with foam and keep sharp sprue edges and screw tips away from it; a dented or puffy pouch gets replaced, never used.<br>**Keep the 14500s** for the stage 1 dev kit (§10.1), and as a fallback cell (a JST-PH holder lead plugs into the same connector).<br>Buy cells from a reputable vendor; distrust capacity claims above ≈ 1000 mAh for 14500 or "3.7 V 2500 mAh 14500" listings. **Check the pouch lead polarity with the multimeter** before first plug-in (PWR-3 protects the board either way). |

### 3.2 Charging and switching

| ID | Requirement |
|---|---|
| PWR-5 | **MUST** charge the cell from **USB-C** (5 V; CC pull-downs so C-to-C chargers supply power). |
| PWR-6 | Charge current **MUST** be safe for the smallest supported cell. Default **500 mA**, set by resistor. |
| PWR-16 | The charger **MUST** be **power-path** (BQ24074-class, OQ-A3): the board runs from USB while charging, charging terminates normally with the switch ON, and the board can be powered and flashed over USB with no cell fitted. A plain linear charger (TP4056) fails all three. |
| PWR-7 | **MUST** have a **slide switch** that disconnects the load (regulator, amp, LED rail, gauge divider) from the battery. The switch drives a P-MOSFET that does the disconnect (≤ 1 µA OFF leakage); the switch contact carries no load current, because no JLC-stocked SMD slide switch is rated for the ≈ 1.16 A peak (OQ-A1). Charging **MUST** still work with the switch OFF. |
| PWR-8 | Charge state (charging / full) and **USB present** **SHOULD** be readable by firmware via GPIO, and shown in the web UI (OQ-A14). |

### 3.3 Regulation — decision: low-dropout LDO, not buck-boost

| ID | Requirement |
|---|---|
| PWR-9 | 3.3 V logic rail from a **≥ 600 mA low-dropout LDO**, dropout ≤ 300 mV at 500 mA. Part: TLV75733P. AP2112K-3.3 is excluded: it does not guarantee ≤ 300 mV at 500 mA (F-3). |
| PWR-10 | LED rail (zone V+) and amp supply come straight from **VBAT (after switch)**. They are not regulated. |
| PWR-11 | Bulk capacitance: ≥ 100 µF on VBAT near the amp/LED rail, plus ≥ 22 µF on the LDO output close to the module. Use **10 V-rated** MLCCs for the VBAT bulk so DC-bias derating still leaves real capacitance at 4.2 V (OQ-A9). Wi-Fi transmit bursts and audio peaks must not brown out the MCU. |
| PWR-12 | **Low-battery shutdown (firmware, required by the LDO choice):** below a threshold (start ≈ 3.35 V under load, tune at stage 1/3, §10.1) the firmware turns off LEDs and audio, shows a warning on the web UI, then deep-sleeps. Charging or power-cycling recovers. |

**Why an LDO:**

- **Board failure risk.** An LDO is three parts and nearly impossible to lay out wrong. A buck-boost (e.g. TPS63802) is a switching regulator. It needs a correctly chosen inductor and a tight, datasheet-exact layout. It's the most common reason a first board fails to boot, and switching noise near the Wi-Fi antenna and audio amp is a second failure mode. Since JLC assembles everything, soldering skill isn't the risk. Design mistakes are, and the LDO has far fewer ways to go wrong.
- **What it costs.** The ESP32-S3 needs ≥ 3.0 V. With ≤ 0.3 V dropout, the board runs until the cell sags to ≈ 3.3 V under load. By then a Li-ion/LiPo cell has delivered ~90–95 % of its capacity. The buck-boost would recover that last ~5–10 %, which is ≈ 10–20 min of a ≈ 6 h typical runtime on the 1500 mAh pouch. That cost is small next to the risk it removes.
- **Common practice.** Most ESP32 battery dev boards (Seeed XIAO, Adafruit Feather, etc.) run straight off a 1S cell through an LDO.
- **Why PWR-12 matters.** Without it, the end of the battery causes a **brownout boot loop**: voltage sags, the MCU resets, load drops, voltage recovers, it reboots, Wi-Fi spikes the load, it resets again. LEDs flicker and the cell drains toward the protection cutoff. A buck-boost would have the same problem, just at a lower voltage. The battery gauge (§3.5) makes the clean shutdown possible. Hardware protection (PWR-2) stays as the last-resort backstop.

### 3.4 Power budget (estimate from datasheets; measured during dev-kit stage, §10.1)

| Load | Typical | Worst case | Notes |
|---|---|---|---|
| ESP32-S3, Wi-Fi AP + phone connected | 90 mA | 130 mA | Station mode to BattleSync similar |
| 8 LED zones | 40 mA | 160 mA | ≤ 20 mA per zone (LED-4) |
| Addressable pixels | 20 mA | 120 mA | e.g. 4 pixels; worst = 2 px full white |
| Audio, close-range | 50 mA | 200 mA | Ambient loop low; worst = effect at full 0.5 W |
| **Total** | **≈ 200 mA** | **≈ 744 mA sustained, ≈ 1.16 A peak** | Worst case recomputed in `architecture.md` §11.2 (F-13) |
| **3 h runtime** | **≈ 600 mAh** | **≈ 2230 mAh** | Before ~20 % reserve. **No firmware power cap** (OQ-A12): the 3 h target applies to typical use; a continuous worst case runs ≈ 1.8 h on the 1500 mAh pouch. |

### 3.5 Battery gauge — decision: VBAT divider into ADC

| ID | Requirement |
|---|---|
| PWR-13 | **MUST** measure battery voltage. Use a resistor divider from switched VBAT into an ADC pin, **200 kΩ / 100 kΩ + 100 nF** (OQ-A8): the ESP32-S3's ±10 mV ADC range gives ±30 mV at VBAT, and it draws ≈ 14 µA only while the switch is ON (nothing when OFF). The self-test page offers a one-point multimeter calibration (FW-1). |
| PWR-14 | Firmware converts voltage to an approximate % using a Li-ion discharge curve, smoothed to hide load sag. Shown in the web UI (and reported to BattleSync LATER). ±10 % accuracy is acceptable. |
| PWR-15 | **SHOULD** use the gauge reading to scale PWM duty so LED brightness stays roughly constant as the battery drains (§4 explains why brightness would otherwise fade). |

A dedicated fuel-gauge IC (e.g. MAX17048) would be more accurate under load. It's not worth an extra I²C part for this product.

---

## 4. LED outputs

| ID | Requirement |
|---|---|
| LED-1 | **MUST** provide **8 zones** with identical hardware. |
| LED-2 | Each zone **MUST** work in either mode, **selected in firmware with no jumpers or rework**:<br>• **PWM mode:** one LED (or two matched LEDs in parallel) from SIG to GND, anode on SIG. Brightness via LEDC PWM (≥ 1 kHz, ≥ 8-bit, flicker-free).<br>• **Addressable mode:** WS2812B/SK6812 chain; power from V+/GND, data on SIG. |
| LED-3 | Per-zone circuit — decision: **push-pull level translator** (74LVC1T45-class: A side 3.3 V, B side VBAT, DIR tied for A→B). GPIO → translator → **R_Z = 120 Ω, 1206** (OQ-A2) → SIG pin. The translator's output swings rail-to-rail at VBAT with nanosecond edges, so pixel data timing is guaranteed by the datasheet. The same pin also sources current for a PWM LED. One unique extended part serves all 8 zones. |
| LED-4 | Per-zone current: **≤ 20 mA** (translator rated ±24 mA at 3.0 V). R_Z = 120 Ω keeps every colour ≤ 20 mA at a full cell (red ≈ 18 mA, white ≈ 10 mA). Dimmer whites, and noticeably dim whites near empty, are accepted (OQ-A2). Firmware **MUST** support a per-zone max-brightness cap for low-Vf (red/amber) LEDs. Lights that need more current use more zones. |
| LED-5 | **MUST** pull every translator input low (≈ 100 kΩ), so zones stay off during boot and flashing. That means no LED flash at power-on, and no garbage data to pixels. |
| LED-6 | Each zone connector is a **3-pin JST-SH 1.0 mm: V+ / SIG / GND**. Same pinout on all 8. Silk labels Z1–Z8. PWM LEDs use only SIG + GND. |
| LED-7 | Zone V+ (pixel power) is shared VBAT, rated ≥ 300 mA per connector (≈ 10 pixels at moderate brightness). |
| LED-8 | **SHOULD** survive SIG shorted to GND or V+ during wiring. R_Z limits the current to ≈ 35 mA (4.2 V / 120 Ω), within the translator's ±50 mA absolute max, and the 1206 resistor dissipates the ≈ 0.15 W. |
| LED-9 | The shared zone V+ bus **MUST** pass through one **resettable PTC fuse** (≈ 0.5–1 A hold) so a shorted V+ lead trips it before wiring heats up, instead of relying on the cell protection's 2–4.7 A trip (OQ-A15). |

Reference loadout (Infernal Tank, provisional, used to sanity-check zone count and current): headlights ×2 (one zone), main flamer muzzle fire (WS2812, 2–3 px, orange/yellow flicker), secondary flamer/heavy weapon flash, exhaust stack glow ×2 (two zones), interior/candle or brazier flicker, a damage-fire effect (WS2812, 2–3 px), and a spare. That's 8 zones, each ≤ 20 mA or a short pixel chain. The final light positions come from the model itself (OQ-14).

### 4.1 Why not the low-side MOSFET design (former OQ-4)

The v0.1 draft gave each zone a MOSFET pulling SIG low, with a resistor pulling it up to the battery. It was cheap, it handled 100 mA, and it level-shifted pixel data for free. Working through the timing shows a real risk:

- **WS2812 pixels read bits from pulse width.** A "0" bit is a high pulse of only ~0.3–0.4 µs. A "1" bit is ~0.8 µs.
- **In the MOSFET design, the falling edge is fast** (the FET actively pulls down). **The rising edge is slow**: only the pull-up resistor charges the line's capacitance, which comes from the FET's own output capacitance (~75 pF for the AO3400 class), the wire, and the pixel input.
- **The numbers:** 2.2 kΩ × ~100 pF ≈ 220 ns time constant. The line needs ~260 ns just to rise past the pixel's "high" threshold. A 350 ns "0" pulse then looks like an ~90 ns pulse to the pixel, which is below its minimum. Result: random wrong colors, flicker, or dead pixels further down the chain.
- **Why it would have needed testing:** the capacitance numbers are estimates. Whether it works depends on the exact FET, wire length, and pixel brand. You could fix it with a smaller pull-up (wastes current) or a smaller FET (loses the 100 mA). But you'd only know after building one and checking on an oscilloscope, which is exactly the kind of iteration a first board should avoid.

The translator removes the question entirely, because both edges are actively driven. The trade is 20 mA per zone instead of 100 mA, which covers every light in the reference loadout.

---

## 5. Audio

| ID | Requirement |
|---|---|
| AUD-1 | **MUST** play audio over **I²S** into an on-board mono class-D amp (**MAX98357A**; OQ-5 closed). No microSD. |
| AUD-2 | Output target: **close-range, ≈ 0.5 W into 8 Ω** at 3.7 V. Audible to players leaning over the table in a moderately noisy store. |
| AUD-3 | Speaker: **8 Ω, 1 W rated** (headroom over the ≈ 0.5 W firmware cap, OQ-A7), off-board on a **2-pin JST-SH** lead. Must fit the ≈ 20 × 40 mm end strip (§2.1): 15 × 11 mm box speaker, or ≤ 20 mm round. |
| AUD-4 | Clips stored in **flash (LittleFS)**, uploadable from the web UI without reflashing. Budget ≥ 8 MB for audio (MCU-2). Output runs at **44.1 kHz**: the MAX98357A does not support 22.05 kHz LRCLK (F-9). Clips are stored at 44.1 kHz or resampled on upload (OQ-A7). Amp gain strap 6 dB, volume capped in firmware at ≈ 0.5 W. |
| AUD-5 | Volume adjustable in software (0–100 %), persisted. |
| AUD-6 | Amp has a GPIO-controlled shutdown, so it draws ≈ 0 µA and doesn't hiss when idle. |
| AUD-7 | **MUST** support **two simultaneous streams** (ambient loop + one-shot effect) via software mixing. |

---

## 6. MCU, memory, flashing

| ID | Requirement |
|---|---|
| MCU-1 | **ESP32-S3-WROOM-1-N16R8** module (16 MB flash, 8 MB octal PSRAM, 18 × 25.5 × 3.1 mm). It's the most common S3 configuration, so the matching dev kit (ESP32-S3-DevKitC-1-N16R8) is cheap and widely available for firmware development (§10.1). |
| MCU-2 | Flash layout target: 2 × ≤ 3 MB app slots (OTA) + ≥ 8 MB LittleFS for audio/config. |
| MCU-3 | Flashing and serial console over **USB-C** via the S3's native USB-Serial/JTAG. No USB-UART chip. Browser flashing (esptool-js / Web Serial) **MUST** work. |
| MCU-4 | Small on-board **BOOT (GPIO0) and RESET** buttons for recovery after a bad flash. Service-only, not reachable with the hull closed. |
| MCU-5 | **MUST** support Wi-Fi OTA updates (upload in web UI). **LATER:** pull from BattleSync's firmware host. |
| MCU-6 | **SHOULD** break out UART TX/RX + GND as test pads. |
| MCU-7 | GPIO budget: 8 zones + I²S BCLK/LRCLK/DOUT + amp SD + status LED + VBAT ADC + charge status ×2 + GPIO0 + USB D±. Avoid strapping pins (0, 3, 45, 46). GPIO26–34 are not pinned out on the WROOM module; IO35–37 are pinned out but used by the octal PSRAM on N16R8 (F-11). Unused GPIOs go to test pads (OQ-A14). |

### 6.1 How the memory decision was made (former OQ-7)

An ESP32 has three kinds of memory, and each is sized differently:

| Memory | What uses it | How you size it |
|---|---|---|
| **Flash** (storage) | Firmware ×2 (OTA needs a spare slot) + filesystem (audio clips, config, web UI files) | Add it up. A typical Arduino ESP32 build with Wi-Fi + async web server is ≈ 1 MB; adding an MP3 decoder, WebSocket, and TLS → budget 3 MB/slot. Audio: a 5 s mono 44.1 kHz MP3 clip at 64 kbps ≈ 40 KB, so 8 MB holds ~200 clips with room for long ambient loops. **≥ 14 MB → 16 MB part.** |
| **Internal SRAM** (~512 KB on-chip, ~300 KB free after boot) | Wi-Fi/TCP stack (~50–80 KB), web server + connections, MP3 decoder (~30 KB), audio/LED buffers, JSON config | You can't add it up reliably. **Measure** the free heap at the worst moment. |
| **PSRAM** (external RAM) | Big buffers, and anything that doesn't fit in SRAM | Needed when measured SRAM headroom is thin. The big spike is **TLS** (`wss://` to battlesync.me needs ~40–50 KB during the handshake) on top of everything else. |

**How you'd normally decide:** build the firmware on a dev board, disable PSRAM, and run the worst case: AP up, phone loading the web UI, ambient + effect audio mixing, a TLS connection opening. Log `heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)` and `heap_caps_get_largest_free_block()`. If the minimum free stays above ~60 KB and the largest block above ~32 KB, PSRAM is unnecessary. That requires firmware that doesn't exist yet.

**Why we skip that here:** the WROOM-1-N16R8 costs ≈ $1–2 more than a no-PSRAM module and removes the question entirely. Being wrong means a board respin, which costs far more than $2. The trade-offs are a module ~5 mm larger in each direction and 5 GPIOs lost to the PSRAM (still plenty, MCU-7). Run the measurement during dev-kit firmware work anyway (§10.1), to catch leaks and confirm the headroom.

**Why stay on the ESP32-S3:** it has native USB (no USB-UART chip), dual cores (audio decoding on one, Wi-Fi/web on the other), plenty of GPIO, and PSRAM options. ESP32-C3/C6 have one core, fewer GPIO, and no PSRAM option.

---

## 7. Physical controls & indicators

| ID | Requirement |
|---|---|
| UI-1 | Physical controls: **power slide switch** (PWR-7) only. BOOT/RESET (MCU-4) are service-only. |
| UI-2 | **On-board status LED** for power / charging / Wi-Fi state. Visible only with the hull open; pattern defined in firmware. |

---

## 8. Standalone operation & web UI

The device is a complete product without BattleSync.

| ID | Requirement |
|---|---|
| SA-1 | **MUST** run its own **Wi-Fi access point** on every boot (unless configured otherwise), with a captive portal / fixed address (e.g. `http://battleaura.local` + `192.168.4.1`) so a phone reaches the UI with no setup. |
| SA-2 | **MUST** play a configurable **power-on scene** (e.g. idle engine loop + base lights) with no phone connected at all. |
| SA-3 | **MUST NOT** ship a fixed default AP/OTA password. Generate one per device at first boot and show it on the serial console, *or* run a first-run setup that forces the user to set one. |
| SA-4 | **SHOULD** optionally join an existing Wi-Fi/phone hotspot as a station (needed later for BattleSync), falling back to AP if not found within N seconds. |

### 8.1 Web UI

| ID | Requirement |
|---|---|
| WEB-1 | **Control panel (primary screen):** a button for each effect/sound (e.g. "Machine gun", "Flamethrower", "Taking hits", "Destroyed"), scene buttons, a master volume slider, master brightness, and a battery % + charging indicator. Phone-first layout, usable one-handed at the table. |
| WEB-2 | **Effects** combine light + sound: e.g. "Machine gun" = sound 0003 + strobe on Z5 for 1.2 s. Users create/edit effects in the UI. |
| WEB-3 | **Zones:** name, mode (PWM / addressable), pixel count, max brightness, base animation (steady, flicker, candle, pulse, …). |
| WEB-4 | **Scenes:** saved base states (e.g. "Idle", "Moving", "Destroyed/smoking"). One is the power-on scene (SA-2). |
| WEB-5 | **Sounds:** upload / delete / preview clips; storage used vs free. |
| WEB-6 | **Device:** Wi-Fi settings, passwords, firmware OTA upload, config export/import (JSON), factory reset. |
| WEB-7 | UI assets served from flash; **no internet/CDN dependencies** (works in AP mode with no uplink). |

---

## 9. BattleSync integration (LATER — design-for, not built in v2.0)

The hardware needs nothing extra. All of this is firmware plus BattleSync server work.

| ID | Requirement |
|---|---|
| BS-1 | Device is a **WebSocket client**. It connects *out* to BattleSync (`wss://`, or `ws://` on LAN), so it works behind NAT/hotspots with no port-forwarding. |
| BS-2 | Auto-reconnect with backoff. Standalone controls (WEB-1) keep working while connected or disconnected. |
| BS-3 | **Pairing:** stable device ID (from MAC) linked to a unit in BattleSync; per-device auth token issued at pairing. |
| BS-4 | Events → effects mapping in the web UI: `unit.damaged`, `unit.status` (normal/shaken/routed), `unit.destroyed`, `unit.activated`, `battle.round`, `battle.ended` each map to a user-chosen effect (WEB-2). |
| BS-5 | Device → server: `hello` (id, firmware version), heartbeat, battery %. |
| BS-6 | Versioned JSON message schema (`"v": 1`), documented in both repos. |

**BattleSync-side dependencies:** BattleSync has **no WebSocket server** today. Its `battle_events` table is **never written**. BS-4 needs BattleSync's planned **M0 event foundation** (`recordEvent()` on every state change) first.

---

## 10. Cost, assembly, release

| ID | Requirement |
|---|---|
| COST-1 | **BOM ceiling: $40 per unit**, landed. Covers PCB + on-board parts + JLC assembly fees + shipping, amortized over 5 boards. Excludes battery, speaker, LEDs, pigtails. Expected ≈ $25–35. Mostly fixed setup and extended-part fees at qty 5. |
| ASM-1 | **Full SMD assembly by JLCPCB PCBA**, top side only. The builder **MUST NOT** hand-solder any SMD part. |
| ASM-2 | Hand work limited to: plugging in pre-crimped JST-SH/PH leads, and splicing LED/speaker wires to pigtail ends (solder + heat-shrink, or crimp butt splices). |
| ASM-3 | Prefer JLC **basic** parts. Each unique **extended** part (module, amp, translator, LDO, …) adds a one-time fee per order, so keep the count down. All parts in JLC stock when ordering. |
| ASM-4 | No through-hole parts for the builder. If one is unavoidable, it's within JLC's THT assembly service. |
| ASM-5 | 2-layer FR-4. 4-layer only if routing forces it. |
| ASM-6 | **Design review gate before ordering:** ERC/DRC clean; every IC checked against its datasheet's reference circuit; a module antenna keep-out check; a 3D model fit check against the envelope (ENV-2/3); and a 1:1 cardboard mock-up of the board (45 × 36 × 10 mm block), the battery, and the speaker, dry-fitted in the Infernal Tank cavity with the hull closed. |
| REL-1 | **Open hardware:** KiCad source, gerbers, JLC BOM + CPL, build guide published in this repo under **CERN-OHL-S-2.0** (hardware) and **GPL-3.0-or-later** (firmware) — §14. |
| REL-3 | **Attribution baked in:**<br>• Silkscreen carries `BattleAura`, the designer credit, `CERN-OHL-S-2.0`, and the repo URL as the license's "Source Location" notice, which redistributors must keep.<br>• The web UI has an **About** page with the copyright, license, and repo link, preserved via a GPLv3 §7(b) attribution notice.<br>• Every source file starts with an SPDX license header. |
| REL-2 | Build quantity **1–5** for personal use. No sale, so certification is out of scope. The pre-certified module covers the radio. |

### 10.1 Build-up and validation plan (no hardware or lab gear assumed)

The goal: by the time PCBs are ordered, everything has been tried that can be tried without them. The PCB stage should then only reveal layout mistakes.

| Stage | What | Proves | Gate to next stage |
|---|---|---|---|
| **0. Paper** | This doc → schematic + layout in KiCad; ASM-6 review. | Design matches datasheets; fits the envelope on screen. | Review checklist passes. |
| **1. Dev kit** | ESP32-S3-DevKitC-1-**N16R8** + MAX98357A breakout + speaker + a few single LEDs with resistors + short WS2812 strip + TP4056/DW01 charger-protection module + one of your 14500s in a holder. All solderless (breadboard + jumper wires; parts with pre-soldered headers). | Firmware: web UI, effects, audio mixing, AP mode, OTA, gauge math, low-battery shutdown, heap headroom (§6.1). **Measured current draw** replaces the §3.4 estimates. | Firmware feature-complete for v2.0; measured 3 h draw fits the chosen cell. |
| **2. Fit check** | Import the hull STL and the board's KiCad 3D export into CAD (FreeCAD/Fusion). Place the board, pouch, and speaker; cut the slot, grille, and standoffs (ENV-8). Print the edited hull section and dry-fit cardboard mock-ups. | Envelope (A-1), hull openings. | Everything fits in a test print with the hull closed. |
| **3. PCB rev A** | Order 5 assembled boards from JLC. | Layout. | Self-test (FW-1) passes on ≥ 4 of 5 boards. |
| **4. In-vehicle** | Install in the Infernal Tank; play a 3 h session. | Runtime, volume, Wi-Fi range through the hull. | Meets PWR-4, AUD-2. |

Stage 1 drives pixels straight from a 3.3 V GPIO. That typically works, but is not guaranteed: the module's V_OH is only guaranteed ≥ 2.64 V, below the pixel's 0.7 × VDD at a full cell (F-7). It is good enough for firmware development, so the dev kit doesn't need the translator. The translator only has to work on the PCB, where its behavior is guaranteed by the datasheet.

| ID | Requirement |
|---|---|
| FW-1 | **MUST** include a **self-test page** in the web UI for board bring-up without instruments: <br>• battery voltage + charge state readout<br>• each zone toggled in PWM mode, then a pixel color-sweep in addressable mode<br>• one mono test tone at each volume step (F-10)<br>• one-point battery-voltage calibration against a multimeter (PWR-13)<br>• free heap/flash<br>• Wi-Fi RSSI as seen by the phone's connection |
| FW-2 | Firmware **MUST** run on both the DevKitC (stage 1) and the custom PCB, using a pin-map selected at build time. |
| TOOL-1 | Tools: the **multimeter** you have (battery voltage, lead polarity, continuity of spliced wires), plus wire strippers, heat-shrink, and a soldering iron for splicing. No oscilloscope required. |

### 10.2 First-build cost estimate (everything, not just the board)

| Item | Estimate |
|---|---|
| Stage 1 dev kit (DevKitC N16R8, amp breakout, charger module, breadboard, jumpers, LED/pixel assortment, speakers) | ≈ $45 |
| 2× 803450 ≈ 1500 mAh LiPo pouch cells with JST-PH leads | ≈ $15 |
| 14500 holder with JST-PH lead (dev kit + fallback; cells already owned) | ≈ $3 |
| Infernal Tank | already owned |
| JLC rev A, 5 assembled boards incl. shipping | ≈ $125–175 (≈ $25–35/board, COST-1) |
| JST-SH/PH pre-crimped pigtail assortment, wire, heat-shrink | ≈ $20 |
| Multimeter (TOOL-1) | already owned |
| **Total to a working tank** | **≈ $210–260** |

---

## 11. Assumptions

| ID | Assumption | Risk if wrong |
|---|---|---|
| A-1 | The measured 65 × 40 × 30 mm cavity is a usable rectangular box: no internal ribs, pegs, or tapering walls that eat into it. The board, pouch, and speaker pack as in §2.1. | Smaller board, a 603450 (≈ 1100 mAh) pouch, or speaker relocation. |
| A-2 | WS2812B/SK6812 run acceptably from VBAT down to the ≈ 3.35 V shutdown point. Datasheets list 3.5 V minimum (WS2812B) and **3.7 V** (SK6812MINI-E, F-8); expect some blue dimming near empty. Accepted and measured on the stage-1 dev kit; firmware can raise the PWR-12 cutoff if pixels misbehave (OQ-A4). | Would need a 5 V boost rail for pixels (adds a switching regulator, the thing §3.3 avoids). |
| A-3 | 0.5 W into 8 Ω is loud enough at close range. | Bigger speaker / 4 Ω, more current. |
| A-4 | Load estimate in §3.4 is within ±50 %. | Battery sizing changes. |
| A-5 | JLC stocks the module, amp, translator, and LDO at order time. | Substitution and re-check. |
| A-6 | 20 mA per zone is enough for every light planned. | Use two zones per light, or revisit LED-3. |
| A-7 | A phone joining the device's own AP is the normal way to use it at a game store. | None for hardware. |
| A-8 | A painted plastic hull doesn't block Wi-Fi meaningfully. Metallic paint, foil, or a metal battery holder near the antenna could. | Range drops. |

---

## 12. Open questions

| ID | Question | How to resolve |
|---|---|---|
| OQ-13 | **Hull openings placement:** USB-C + switch slot on the underside, rear, or behind a removable panel? Grille location? The hull is printed, so this is decided in CAD at stage 2 (ENV-8). | Stage 2 |
| OQ-14 | **Light positions on the model:** which spots get lights, and how wires route from each to the cavity. Finalizes the reference loadout (§4) and zone assignment. | Me, with the model in hand |
| OQ-9 | BattleSync binding model: device ↔ army unit (persistent) or device ↔ battle unit (re-pair each battle)? WS server design/auth. | BattleSync design doc (LATER) |
| OQ-11 | Other vehicles later (Dreadnought, speeder)? Would need a smaller board or fewer zones. Out of scope for v2.0. | Later |

### Resolved

| Former | Resolution |
|---|---|
| OQ-10 license | CERN-OHL-S-2.0 + GPL-3.0-or-later, with attribution requirements (REL-3, §14.3). |
| OQ-1 hull size | Target is the OPR Infernal Tank; cavity measured at 65 × 40 × 30 mm (ENV-1, §2.1). |
| OQ-2 battery gauge | VBAT divider → ADC, % in firmware/UI (§3.5). |
| OQ-3 offline mode | Standalone is now the primary product: own AP + power-on scene (§8). |
| OQ-4 zone circuit validation | Replaced the MOSFET + pull-up zone with a push-pull translator; no analog margin left to test (§4.1). |
| OQ-6 regulator | Low-dropout LDO + firmware low-battery shutdown (§3.3). |
| OQ-7 memory | ESP32-S3-WROOM-1-N16R8 (§6.1). |
| OQ-8 pull-up waste | Obsolete with the translator design. |
| OQ-12 sudden blackout | Firmware warns + shuts down cleanly using the gauge (PWR-12); hardware cutoff is the backstop. |
| OQ-5 amp | MAX98357A (`architecture.md` §7). |
| OQ-A1 power switch | Slide switch drives a P-MOSFET load switch (PWR-7). |
| OQ-A2 zone resistor | 120 Ω 1206 (LED-3/4/8). |
| OQ-A3 charger | Power-path BQ24074-class (PWR-16). |
| OQ-A4 low-voltage pixels | Accepted; measured at stage 1 (A-2). |
| OQ-A5 reverse battery | P-MOSFET topology from `architecture.md` §3.3, verified by SPICE + one sacrificial board before ordering in quantity. |
| OQ-A6 extended parts | Accepted; real JLC quote from Gerber/BOM/CPL once layout exists. |
| OQ-A7 audio | 44.1 kHz, mono test tone, 6 dB gain + firmware cap, 1 W speaker (AUD-3/4, FW-1). |
| OQ-A8 gauge divider | 200 kΩ / 100 kΩ + calibration (PWR-13). |
| OQ-A9 bulk caps | 10 V-rated MLCCs (PWR-11). |
| OQ-A10 connectors | Decided during layout from the cavity measurements; prefer top-entry where it fits. |
| OQ-A11 USB ESD | Genuine ST USBLC6-2SC6. |
| OQ-A12 runtime | No firmware power cap; 3 h applies to typical use (§3.4). |
| OQ-A13 cell | Candidate cells to be researched and proposed to the owner (PWR-4). |
| OQ-A14 extras | Spare GPIOs to test pads; USB-present readable by firmware (PWR-8). |
| OQ-A15 V+ short | Resettable PTC on the shared zone V+ (LED-9). |

---

## 13. Out of scope (v2.0)

- BattleSync connection (designed-for, §9, built later).
- Bluetooth/BLE control.
- Physical user buttons, sensors (IMU, hall, IR).
- Magnetic/pogo charging; microSD.
- Selling units / regulatory certification.
- Vehicles other than the Infernal Tank. The board should suit any vehicle with a similar cavity, but only this one is validated.

---

## 14. Licensing — decided: CERN-OHL-S-2.0 + GPL-3.0-or-later

Hardware design files (KiCad, gerbers, BOM) and firmware are licensed **separately**. Hardware licenses cover physical manufacture; software licenses don't.

### 14.1 Hardware options

| License | In plain terms | Pros | Cons |
|---|---|---|---|
| **CERN-OHL-S v2** (strongly reciprocal) | Anyone may build, modify, or sell it, but if they distribute a modified version they must publish its full design files under the same license. | Improvements come back to the community; a seller can't take it closed. OSHWA-approved; written specifically for hardware. | Some commercial makers avoid reciprocal licenses. The "available components" wording takes a read. |
| **CERN-OHL-W v2** (weakly reciprocal) | Changes to *your* design must be shared, but it can be combined with someone else's closed parts/modules. | Middle ground; friendlier to people embedding it in a bigger product. | Weaker guarantee that derivatives stay open. |
| **CERN-OHL-P v2** (permissive) | Do anything, including a closed commercial version; just keep the notice. | Maximum adoption, zero friction. | Someone can sell a closed clone and owe you nothing but attribution. |
| **CC BY-SA 4.0** | Share-alike, like OHL-S, but a generic creative-works license. | Familiar to hobbyists (Adafruit/SparkFun use it). | Not written for hardware: it covers copying the *files*, and arguably not making boards from them. |
| **CC BY-NC-SA 4.0** | Like BY-SA, but no commercial use. | Popular in the miniatures community; stops people selling it. | **Not open source** by OSHWA/OSI definitions; "commercial" is fuzzy (selling 3 boards at cost?); can't be enforced against overseas cloners in practice. Also blocks *you* from accepting outside contributions you'd later sell. |

### 14.2 Firmware options

| License | Pros | Cons |
|---|---|---|
| **GPL-3.0-or-later** | Pairs with CERN-OHL-S: modified firmware shipped on a sold device must be published. Compatible with the libraries in play (ESP-IDF/Arduino Apache-2.0, FastLED/ArduinoJson MIT, ESPAsyncWebServer LGPL). | Some contributors prefer permissive licenses. |
| **MIT** | Simplest, most widely understood; pairs with CERN-OHL-P. | Anyone can ship closed firmware forks. |
| **Apache-2.0** | Like MIT plus an explicit patent grant. | Longer; the patent clause is irrelevant at hobby scale. |

### 14.3 Decision

**CERN-OHL-S-2.0 (hardware) + GPL-3.0-or-later (firmware).**

**Goal:** stop people from selling BattleAura as if they created it.
- **No open license forbids selling.** Only a non-commercial (NC) license tries to, and §14.1 explains why NC isn't used.
- **What every license here *does* forbid is removing the credit.** CERN-OHL-S and GPL go further: anyone who distributes boards or modified firmware must keep the attribution and Source Location notices, and must publish their full design and source under the same license. A reseller can't present it as their own closed product without violating the license, and the published source plus the silkscreen credit (REL-3) show where it came from.
- **Enforcement is realistically a takedown request** (marketplace/GitHub/DMCA) citing the license violation. The license makes that request clear-cut.
- **The name is separate.** If someone sells boards *under the BattleAura name*, that's a trademark question, not a license one. Registration isn't worthwhile at hobby scale, but consistent use of the name in the repo and on the silkscreen establishes prior use.

### 14.4 Third-party IP to keep out of the repo

- **OPR model files.** Don't publish the Infernal Tank STL or any modified version of it (e.g. with a battery door added) unless OPR's license permits redistribution. Publish *instructions* or a separate add-on part instead.
- **Trademarks.** Don't use Games Workshop or OPR names/logos in the product name, board silkscreen, or as branding. Saying "fits the OPR Battle Sisters Infernal Tank" as a description is fine.
- **Audio clips.** Ship only sounds you made or that carry a license allowing redistribution (e.g. CC0 from freesound.org); never game or film audio.

