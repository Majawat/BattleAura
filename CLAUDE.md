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

## **CORE VISION: Modular Configuration-Driven Effects System**

### **The Problem We're Solving:**
Same firmware should work for Tank, Beast, Dreadnought, Daemon Engine - just with different configurations. Players configure once during setup, then have simple effect buttons during gameplay.

### **Pin Configuration System:**
Each pin has:
- **Type**: "Engine", "Weapon", "Candle", "Console", "Damage", "Ambient"
- **Group**: "Engine1", "Engine2", "Weapon1", "Candles", etc.
- **Pin Mode**: PWM (single color), WS2812B (RGB), Digital (on/off)
- **Brightness**: Per-pin brightness (0-255) to balance different LED types
- **Ambient Effect**: Default always-on effect (Candle flicker, Engine idle, etc.)

### **Effect System Design:**

**Two Priority Levels:**
- **Ambient Effects**: Always running (candle flicker, engine idle, console scrolling)
- **Active Effects**: Triggered by player, interrupt ambient temporarily (weapon fire, engine rev, taking damage)

**Effect Triggering:**
- `/effect/engine/idle` → finds all pins with type="Engine" → applies engine idle
- `/effect/weapon/fire` → finds all pins with type="Weapon" → applies weapon flash  
- `/effect/damage/hit` → finds ALL pins → applies damage effect appropriate to pin type

**Pin Type Considerations:**
- **PWM LEDs**: Can only change brightness (dim, bright, flicker, pulse)
- **WS2812B RGB**: Can change color AND brightness (red flash, blue glow, color cycles)
- **Taking Damage Example**: PWM pins flicker rapidly, RGB pins flash red, then restore previous effects

### **User Experience:**

**Setup Phase (Config Page):**
Pin assignments like current tank:
- Pin 0: Type="Candle", Group="Candles", Mode=PWM, Brightness=150, Ambient=Flicker
- Pin 3: Type="Console", Group="Console", Mode=WS2812B, Brightness=80, Ambient=DataScroll  
- Pin 8: Type="Engine", Group="Engine1", Mode=PWM, Brightness=200, Ambient=Idle
- Pin 9: Type="Engine", Group="Engine2", Mode=PWM, Brightness=180, Ambient=Idle

**Gameplay Phase (Main Page):**
Dynamic buttons based on configured types:
- "Engine Rev" (appears if any Engine pins configured)
- "Weapon Fire" (appears if any Weapon pins configured) 
- "Taking Damage" (always available)
- "Candle Mode" (appears if any Candle pins configured)

**Same firmware, different miniature:**
Beast config: Pin 0="Eyes", Pin 1="Claws", Pin 2="Wounds"
→ Main page shows: "Eyes Glow", "Claw Strike", "Taking Wounds"

### **Technical Implementation:**
- Effects target types/groups, not specific pins
- Pin brightness configured during setup to balance different LED types
- Ambient effects start automatically, active effects interrupt then restore
- Effect behavior adapts to pin capabilities (PWM vs RGB)

### **RGB Strip Handling:**
**One Pin = One Physical Thing**
- Each WS2812B pin represents one complete feature (Console, Brazier, Engine, etc.)
- Effects control all LEDs on that strip as a coordinated unit
- Examples:
  - Pin 3: "Console" (5 LEDs) → Console effect manages all 5 for data scrolling
  - Pin 4: "Brazier" (8 LEDs) → Brazier effect makes each LED flicker independently
  - Pin 5: "Engine" (12 LEDs) → Engine effect pulses all 12 together for rev
- Configuration: Pin has LED count, effect handles coordination
- No strip subdivision - if you want separate features, use separate pins/strips

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

## Claude Development Rules

### **Core Architecture Rules:**
1. **ALWAYS read CLAUDE.md first** - Understand the modular vision before changing ANY code
2. **Think holistically towards end goal** - Don't do quick fixes that break the core user experience
3. **Preserve the priority system** - Ambient effects should always run, active effects should interrupt and restore
4. **Never break the type-based system** - Effects should target pin types, not hardcoded pins
5. **Maintain the user workflow** - Configure once (setup), then use effect buttons (gameplay)

### **Change Management Rules:**
6. **Build and test before committing** - Ensure changes compile and don't break existing functionality
7. **Update firmware version** - Always increment version for any functional change
8. **Commit changes frequently and properly** - With descriptive messages explaining the "why"
9. **Consider side effects** - How does this change affect other parts of the system?
10. **Follow existing patterns** - Don't reinvent wheels, use established code patterns

### **Problem-Solving Rules:**
11. **Fix root causes, not symptoms** - If RGB color doesn't work, fix the effects system, don't add bypasses
12. **Preserve existing working features** - Don't break ambient effects while fixing manual controls
13. **Test against the core vision** - Will a Tank user with candle flickers still have them after my change?
14. **Ask "Does this help the end goal?"** - If it's just a band-aid, it's probably wrong

### **Code Quality Rules:**
15. **Ensure best practices** - Clean, maintainable code, not just "working" code
16. **Document breaking changes** - If you must break something, document why and how to restore it
17. **Keep flash usage reasonable** - Monitor memory usage, optimize when needed
18. **Remove old code aggressively** - We're pre-alpha, break backward compatibility to clean up architecture