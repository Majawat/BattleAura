# BattleAura
### LED and Sound Controller for wargaming miniatures

## Parts

- Seeed Studio Xiao ESP32-S3 dev board
- DFRobot DFPlayer Mini
- microSD card
- LEDs (single LEDs and WS2812B)
- Speaker (8ohm)
- 14500 Battery

### Wiring Notes

- ESP32 GPIO43 (D6/TX) -> DFPlayer RX
- ESP32 GPIO44 (D7/RX) -> DFPlayer TX
- GPIO 1-9 pins to LEDs+, GND
- Speaker +/1 to DFPlayer
- Battery direct to ESP32-S3 battery terminals, WS2812B, and DFPlayer

## LEDS

### LED Pin assignment

- D0: Candle Fibers 1 - 3mm warm white, 3v
- D1: Candle Fibers 2 - 3mm warm white, 3v
- D2: Candle Fibers 3 - 3mm warm white, 3v
- D3: Console screen - WS2812B RGB, 5v
- D4: Machine Gun - Nano (1.6mm) warm white, 3v
- D5: Flamethrower - Nano (1.6mm) warm white, 3v
- D8: Engine stack 1 - Chip (3.2mm) warm white, 3v
- D9: Engine stack 2 - Chip (3.2mm) warm white, 3v
- D10: Brazier - Chip (3.2mm) warm white, 3v

## Audio files assignments

- 0001 - Tank engine idle 1
- 0002 - Tank engine idle 2
- 0003 - Machine gun fire
- 0004 - Flamethrower
- 0005 - Taking hits
- 0006 - Engine revving
- 0007 - Destroyed
- 0008 - Rocket Launcher
- 0009 - Killed other unit

 ### Audacity settings
  - Track > Mix > Mix stero down to Mono
  - Track > Resample > 22050 Hz
  - Effect > Normalize > -1dB
  - Export as MP3
