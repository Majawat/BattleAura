# BattleAura

### LEDs & Sound for Warhammer

Just a fun project to add sound and lights to my Warhammer tanks. ESP32 + DFPlayer Mini + LEDs.

## What I Did

- MP3 sounds (engine, guns, etc.) from SD card
- Neopixel LEDs for glowy bits
- ESP32 does the brains, DFPlayer does the noise
- I just use buttons for triggers, but you could do more

## Parts I Used

- Seeed Studio Xaio ESP32-C3 dev board
- DFPlayer Mini
- microSD card (with MP3s)
- LEDs
- Speaker (8Ω)

## Wiring

ESP32 TX -> DFPlayer RX
ESP32 RX -> DFPlayer TX
GNDs together
5V to DFPlayer
Speaker to DFPlayer

### LED Pin assignment

- D0: Candle Fibers 1
- D1: Candle Fibers 2
- D2: Brazier
- D3: Console screen
- D4: Weapon 1
- D5: Weapon 2
- D8: Engine stack 1
- D9: Engine stack 2
- D10: ?

### Audio files assignments
 - 0002 - Tank engine idle
 - 0003 - Weapon 1 fire
 - 0004 - Weapon 2 fire
 - 0005 - Taking hits
 - 0006 - Engine revving
 - 0007 - Destroyed
 - 0008 - Limited big weapon
 - 0009 - Killed another unit

## How I Use It

1. PlatformIO in VS Code
2. MP3s on SD card (see `audio_files/` for what I used)
3. Edit `main.cpp` for whatever effects I want
4. Upload, plug in, press buttons, enjoy

## Features that are setup

- Tank engine idle: looping sound
- Candle function

## Planned features

- Add more MP3s, add LEDs
- Add WiFi and configuration
- Try new LED patterns
- Add connection to BattleSync
