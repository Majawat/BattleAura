# Simulations

## reverse_battery.cir — reversed cell at J2 (OQ-A5, architecture.md §3.3)

Run: `cd hardware/sim && ngspice -b reverse_battery.cir | grep RESULT` (ngspice 45.2).

**Models are behavioral, built from datasheet figures** — no vendor SPICE models exist for the DW01A,
FS8205A or the BQ24074 BAT pin:
- Q1 AO3401A / Q3 FS8205A: level-1 MOSFETs with body diodes; threshold swept over the datasheet range
  (P: 0.5 / 0.9 / 1.3 V; N: 0.5 / 1.0 V).
- U4 DW01A: OD/OC = VCC only inside its normal 2.5–4.3 V window, 4.5 µA supply, ESD diodes VCC↔GND and CS→VCC.
- U3 BQ24074 BAT pin: 1.3 mA short-detect below 1.8 V, 49 mA pre-charge up to 4.2 V, ESD diode from VSS.
- A pessimistic case forces both protection FETs on regardless of U4.

**Results (2026-11-14), 4.2 V cell inserted backwards, 300 ms after USB is applied:**

| Case | Q1 current | Q1 dissipation | min V(VBAT_RAW) |
|---|---|---|---|
| No USB, all corners | ≈10 pA | ≈0 | −4.4 µV |
| USB, U4 behavioral, low-threshold corners | 1.3–1.9 mA | ≤9 mW | −4.4 µV |
| USB, U4 behavioral, high-threshold corners | ≈48 mA (charger leaves short-detect above 1.8 V) | ≤0.27 W (≈+34 °C at 125 °C/W) | −4.4 µV |
| USB, protection FETs forced on | ≈1.3 mA (Q1 clamps VBAT_RAW below 1.8 V) | ≤8 mW | −4.4 µV |

Conclusion: the BQ24074 BAT pin stays far from its −0.3 V absolute minimum in every corner, so no clamp
diode is needed. Worst case matches the hand analysis: with USB attached a reversed cell is drained at
≈50 mA and Q1 runs ≈+34 °C. Still to do: the sacrificial-board test before ordering in quantity (OQ-A5).

Lesson recorded: SPICE MOSFET node order is **D G S B** — an early run had Q1's gate and source swapped
and reported a false "no current" result; it was caught because the forced-on case showed no conduction.
