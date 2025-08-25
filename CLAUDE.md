# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BattleAura is an ESP32-based modular lighting and audio effects system for Warhammer 40K miniatures. The project features a configuration-driven architecture, web-based control interface, and support for both standard LEDs and WS2812B RGB strips. Built with PlatformIO using the Arduino framework.

## Development Environment

This project uses **PlatformIO** for building and uploading to the ESP32:
- Target board: `seeed_xiao_esp32c3`
- Framework: Arduino
- Key dependencies: DFRobotDFPlayerMini, EspSoftwareSerial, ArduinoJson, Adafruit NeoPixel, ESPAsyncWebServer

## Essential Commands

**IMPORTANT: PlatformIO is available at `~/.platformio/penv/bin/platformio` in this environment**

### Build and Upload
```bash
# Build the project (ALWAYS RUN THIS BEFORE COMMITTING)
~/.platformio/penv/bin/platformio run

# Clean build
~/.platformio/penv/bin/platformio run -t clean

# Build with verbose output  
~/.platformio/penv/bin/platformio run -v

# Upload filesystem (SPIFFS files)
~/.platformio/penv/bin/platformio run -t uploadfs
```

### Build Targets
- **Flash usage:** Keep under 70% (currently ~62%)
- **RAM usage:** Keep under 50% (currently ~12%)
- **Success indicator:** "[SUCCESS]" message at end

### CRITICAL: Always Build Before Committing
- Test compilation with `~/.platformio/penv/bin/platformio run`
- Fix any errors BEFORE committing code
- Never commit code that doesn't compile

### Development Workflow
```bash
# Check for connected devices
platformio device list

# Run with verbose output for debugging
platformio run -v

# Specify environment explicitly
platformio run -e seeed_xiao_esp32c3
```

## Architecture Overview

### Modular Design
The system is built around a modular, configuration-driven architecture:

- **ConfigManager**: JSON-based configuration system with SPIFFS storage
- **EffectManager**: Lighting effects engine supporting multiple LED types
- **AudioManager**: DFPlayer Mini integration with error handling
- **Web Interface**: Real-time control via WebSocket and HTTP APIs
- **OTA Updates**: Over-the-air firmware updates

### Hardware Support
- **ESP32 (Seeed Xiao ESP32-C3)**: Main controller with WiFi
- **DFPlayer Mini**: MP3 audio playback from SD card
- **Standard LEDs**: PWM-controlled single-color LEDs
- **WS2812B RGB**: Addressable RGB LED strips/pixels
- **14500/Li-ion Battery**: Portable power with 5-6 hour runtime

### Pin Configuration
Pin assignments are fully configurable via web interface:
- **GPIO 20**: DFPlayer RX (fixed)
- **GPIO 21**: DFPlayer TX (fixed)
- **All other pins**: User-configurable for LEDs/effects

### Audio System
MP3 files stored on SD card in `/audio_files/` directory:
- **0001**: Tank Idle
- **0002**: Tank Idle 2
- **0003**: Machine Gun
- **0004**: Flamethrower
- **0005**: Taking Hits
- **0006**: Engine Revving
- **0007**: Explosion
- **0008**: Rocket Launcher
- **0009**: Kill Confirmed

## Code Architecture

### Core Components

**Configuration System (`src/config.h`, `src/config.cpp`)**:
- JSON-based configuration with SPIFFS persistence
- Pin assignments, WiFi credentials, audio settings
- Runtime configuration changes via web interface
- Default fallback configurations

**Effects Engine (`src/effects.h`, `src/effects.cpp`)**:
- Modular effect system supporting multiple LED types
- Built-in effects: Candle Flicker, Engine Pulse, Machine Gun, Flamethrower, Rocket Launcher, Taking Damage, Explosion, Console RGB, Static On/Off
- Real-time effect triggering and control
- Brightness and color control per pin

**Audio Manager (`src/audio.h`, `src/audio.cpp`)**:
- DFPlayer Mini integration with error handling
- Support for looping and one-shot audio playback
- Volume control and playback status monitoring
- Graceful fallback when audio hardware unavailable

**Main Application (`src/main.cpp`)**:
- WiFi connection with AP fallback mode
- Web server with REST API endpoints
- WebSocket real-time communication
- OTA update functionality
- Main update loop coordination

**Web Interface (`data/index.html`, `data/app.js`)**:
- Real-time control interface with WebSocket communication
- Pin configuration display and control
- Audio playback controls with volume adjustment
- Global effect triggers
- Mobile-responsive design

### Key Features

**Configuration-Driven Design**:
- No hardcoded pin assignments
- JSON configuration stored in SPIFFS
- Web-based configuration interface
- Runtime reconfiguration without reflashing

**Multi-LED Support**:
- Standard PWM LEDs for brightness control
- WS2812B RGB LEDs for full color effects
- Mixed pin configurations supported
- Per-pin brightness and color control

**Networking**:
- WiFi station mode with credentials storage
- AP fallback for initial configuration
- WebSocket for real-time communication
- HTTP REST API for programmatic control
- OTA updates with password protection

**Effect System**:
- Hardcoded effects library (configurable in future versions)
- Synchronized audio and lighting effects
- Duration-based temporary effects
- Continuous background effects
- Global effect broadcasting

## Project Structure

```
├── src/
│   ├── main.cpp           # Main application and networking
│   ├── config.h/.cpp      # Configuration management system
│   ├── effects.h/.cpp     # Lighting effects engine
│   └── audio.h/.cpp       # Audio playback system
├── data/
│   ├── index.html         # Web interface HTML
│   └── app.js            # Web interface JavaScript
├── audio_files/          # MP3 sound effects
├── kicad_files/          # PCB design files
├── platformio.ini        # PlatformIO configuration
└── CLAUDE.md             # This documentation
```

## API Endpoints

### REST API
- `GET /config` - Retrieve current configuration
- `POST /config` - Update configuration
- `POST /trigger` - Trigger pin effect with optional audio
- `POST /audio` - Audio control (play/stop/volume)

### WebSocket Commands
```json
{
  "command": "trigger_effect",
  "pin": 2,
  "effect": 3,
  "duration": 1000,
  "audio": 3
}
```

## Development Notes

- Configuration persists across reboots via SPIFFS
- Web interface falls back to HTTP if WebSocket fails
- OTA password: "battlesync"
- AP mode password: "battlesync"
- Serial baud rate: 115200
- Designed for 35mm H × 60mm W × 40mm D cavity constraint
- Target completion: October 9th, 2025

## ESP32 Framework Naming Conflicts (CRITICAL)
**NEVER use these names in enums - they are Arduino/ESP32 macros:**
- `DISABLED` → use `PIN_DISABLED`
- `LOW` → use `PIN_LOW`
- `HIGH` → use `PIN_HIGH`
- Always check enum names against ESP32 framework before using

## Physical Constraints
- **Battery Life**: 5-6 hours on 14500 Li-ion
- **Cavity Size**: 35mm H × 60mm W × 40mm D
- **Components**: Battery, switch, LEDs, wiring, fiber optics, DFPlayer, MCU, perfboard