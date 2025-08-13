# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BattleAura is an Arduino-based project that adds LED effects and sound to wargamming miniatures using an ESP32 microcontroller, DFPlayer Mini for audio, and various LEDs. The project uses the PlatformIO build system with the Arduino framework.

## Development Environment

This project uses **PlatformIO** for building and uploading to the ESP32:
- Target board: `seeed_xiao_esp32c3`
- Framework: Arduino
- Main dependencies: DFRobotDFPlayerMini, EspSoftwareSerial

## Essential Commands

### Build and Upload
```bash
# Build the project
platformio run

# Upload to device (ensure device is connected)
platformio run --target upload

# Build and upload in one command
platformio run -t upload

# Clean build artifacts
platformio run -t clean

# Open serial monitor
platformio device monitor

# Run tests (requires ESP32 connected)
platformio test
```

### Development Workflow
```bash
# Check for connected devices
platformio device list

# Run with verbose output for debugging
platformio run -v

# Specify environment explicitly
platformio run -e seeed_xiao_esp32c3
```

## Hardware Architecture

### Core Components
- **ESP32 (Seeed Xiao ESP32-C3)**: Main controller
- **DFPlayer Mini**: MP3 audio playback from SD card
- **LEDs**: Various effects on different pins
- **Speaker**: 8Ω speaker connected to DFPlayer

### Pin Assignments
- **D0-D5, D8-D10**: LED control pins for different effects
- **D6 (GPIO21)**: TX to DFPlayer RX  
- **D7 (GPIO20)**: RX from DFPlayer TX

### Audio System
- MP3 files stored on SD card in `audio_files/` directory
- Files numbered 0001-0009 for different sound effects:
  - 0002: Tank engine idle (loops)
  - 0003-0004: Weapon fire sounds
  - 0005: Taking damage
  - 0006: Engine revving
  - 0007: Destruction sound
  - 0008: Limited weapon fire
  - 0009: Unit kill confirmation

## Code Architecture

### Main Components (`src/main.cpp`)

**Audio Constants**: Defined audio file mappings (`AUDIO_IDLE`, `AUDIO_WEAPON_FIRE_1`, etc.)

**LED Effects**:
- `candleFlicker()`: Creates realistic candle flicker effect with random intensity variations
- `enginePulse()`: Breathing effect for engine LEDs (implemented but not currently used)

**Hardware Initialization**:
- Serial communication setup (9600 baud)
- LED pin configuration
- DFPlayer initialization with volume control
- Automatic startup with looping idle sound

**Main Loop**:
- Continuous candle flicker effect on LEDs 1-3
- DFPlayer status monitoring and error handling

### Key Implementation Details

- Uses SoftwareSerial for DFPlayer communication
- LED effects use `analogWrite()` for PWM brightness control
- Random number generation for natural-looking flicker effects
- Comprehensive DFPlayer error handling and status reporting

## Project Structure

```
├── src/main.cpp           # Main Arduino sketch
├── platformio.ini         # PlatformIO configuration
├── audio_files/          # MP3 sound effects
├── kicad_files/          # PCB design files
├── include/              # Header files (currently empty)
├── lib/                  # Custom libraries (currently empty)
└── test/                 # Test files (currently empty)
```

## Development Notes

- The project is designed for tactile gaming enhancement with button triggers
- LED effects are designed to simulate realistic tank lighting (candles, engine glow)
- Audio system supports looping background sounds with triggered effects
- Hardware is designed to fit inside wargamming models
- Future plans include WiFi connectivity and integration with "BattleSync" system