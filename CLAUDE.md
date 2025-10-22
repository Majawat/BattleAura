# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BattleAura is an ESP32-S3 based modular lighting and audio effects system for Warhammer 40K miniatures. The project features a zone-based configuration architecture, web-based control interface, and support for both standard LEDs and WS2812B RGB strips. Built with PlatformIO using the Arduino framework.

## Development Environment

This project uses **PlatformIO** for building and uploading to the ESP32:

- Target board: `seeed_xiao_esp32s3`
- Framework: Arduino
- Key dependencies: ArduinoJson, FastLED, ESPAsyncWebServer, DFRobotDFPlayerMini

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

# Upload filesystem (LittleFS files)
~/.platformio/penv/bin/platformio run -t uploadfs
```

### Build Targets

- **Flash usage:** Keep under 70%
- **RAM usage:** Keep under 50%
- **Success indicator:** "[SUCCESS]" message at end

### CRITICAL: Always Build Before Committing

- Test compilation with `pio run`
- Fix any errors BEFORE committing code
- Never commit code that doesn't compile

### Development Workflow

```bash
# Check for connected devices
~/.platformio/penv/bin/platformio device list

# Run with verbose output for debugging
~/.platformio/penv/bin/platformio run -v

# Specify environment explicitly
~/.platformio/penv/bin/platformio run -e seeed_xiao_esp32s3
```

## New Architecture Overview

### Modular Zone-Based Design with VFX + Scenes Terminology

The system is built around a modular, zone-based architecture with clear terminology separation:

- **Zone**: Physical LED output (PWM or WS2812B) with configuration
- **Group**: Logical collection of zones (e.g., "Engines", "Weapons")
- **VFX**: Code classes that create visual effects (CandleVFX, WeaponFireVFX)
- **Scene**: User configuration mapping VFX to groups with audio (e.g. "Candle Ambiance" scene)
- **Priority System**: Global > Active > Ambient scenes

### Terminology Clarification

- **VFX** = Visual effects code (technical implementation in C++)
- **Scene** = User-created mapping of VFX to groups with audio settings (configuration data)
- This eliminates confusion between code classes and user configurations

### Core Components

- **VFXManager**: Manages scene priorities, transitions, and VFX lifecycle
- **LedController**: Abstracts PWM vs WS2812B hardware differences
- **AudioController**: Hardware Serial DFPlayer Mini integration
- **ConfigManager**: LittleFS-based persistent configuration storage
- **WebServer**: Real-time WebSocket + HTTP API control interface

### Hardware Support

- **ESP32-S3 (Seeed Xiao ESP32-S3)**: Main controller with WiFi and Bluetooth
- **DFPlayer Mini**: MP3 audio playback from SD card via Hardware Serial
- **Standard LEDs**: PWM-controlled single-color LEDs
- **WS2812B RGB**: Addressable RGB LED strips/pixels
- **Battery**: Portable power with 5-6 hour runtime

### Pin Configuration

Pin assignments are fully configurable via web interface:

- **GPIO 43 (D6)**: DFPlayer TX (ESP32 TX -> DFPlayer RX - Hardware Serial)
- **GPIO 44 (D7)**: DFPlayer RX (ESP32 RX <- DFPlayer TX - Hardware Serial)
- **GPIO 1-9**: User-configurable zones (9 zones max with audio)
- **GPIO 1-9, 43-44**: All available if audio disabled (11 zones max)

### Audio System

MP3 files stored on SD card, user manually configures file numbers during setup:

- Files named 0001.mp3, 0002.mp3, etc.
- User enters file number and description in web interface
- System provides test buttons to verify audio files
- Effects can trigger audio with automatic ambient/active management

## Project Structure

```
├── src/
│   ├── main.cpp                    # Setup, WiFi, main loop
│   ├── effects/
│   │   ├── EffectManager.h/cpp     # Effect priority and lifecycle
│   │   ├── BaseEffect.h            # Abstract effect base class
│   │   └── library/
│   │       ├── CandleEffect.h/cpp  # Candle flicker effect
│   │       ├── EngineEffect.h/cpp  # Engine idle/rev effects
│   │       ├── ConsoleEffect.h/cpp # Console/data effects
│   │       ├── WeaponEffect.h/cpp  # Weapon flash effects
│   │       └── GlobalEffect.h/cpp  # Taking hits, destroyed
│   ├── hardware/
│   │   ├── LedController.h/cpp     # PWM + WS2812B abstraction
│   │   ├── AudioController.h/cpp   # Hardware Serial DFPlayer
│   │   └── PinManager.h/cpp        # GPIO allocation/validation
│   ├── web/
│   │   ├── WebServer.h/cpp         # AsyncWebServer + routes
│   │   ├── WebSocketHandler.h/cpp  # Real-time communication
│   │   └── WebInterface.h          # Embedded HTML/CSS/JS
│   └── config/
│       ├── Configuration.h/cpp     # LittleFS persistent storage
│       ├── ZoneConfig.h           # Zone/pin definitions
│       └── EffectConfig.h         # Effect-to-group mappings
├── data/                          # (Future: static files if needed)
├── partitions.csv                 # Custom partition table
├── platformio.ini                 # PlatformIO configuration
└── CLAUDE.md                      # This documentation
```

## Data Models

### Zone Definition

```cpp
struct Zone {
    String name;              // "Engine LEDs Left"
    uint8_t gpio;            // 2-10, 20-21 (if audio disabled)
    enum Type { PWM, WS2812B } type;
    uint8_t ledCount;        // For WS2812B strips
    String groupName;        // "Engines"
    uint8_t brightness;      // 0-255 max brightness
    bool enabled;
};
```

### Effect Configuration

```cpp
struct EffectConfig {
    String name;             // "MachineGun"
    enum Type { AMBIENT, ACTIVE, GLOBAL } type;
    std::vector<String> groups;  // Groups this effect applies to
    uint16_t audioFile;      // 0 = none, else file number
    uint32_t duration;       // ms, 0 = loop/ambient
    JsonDocument parameters; // Effect-specific params
};
```

## API Endpoints

### REST API

- `GET /api/zones` - Get zone configuration
- `POST /api/zones` - Update zone configuration
- `GET /api/effects` - Get effect configuration
- `POST /api/effects` - Update effect configuration
- `POST /api/trigger` - Trigger effect by name
- `POST /api/audio` - Audio control (play/stop/volume)

### WebSocket Protocol

```json
// Client -> Server (trigger effect)
{
  "cmd": "trigger",
  "effect": "MachineGun",
  "group": "Weapons"
}

// Server -> Client (state update)
{
  "type": "state",
  "zones": [...],
  "activeEffects": ["MachineGun"],
  "volume": 75
}
```

## Web Interface Structure

### Main Control Page (`/`)

- Real-time WebSocket connection
- Dynamic effect buttons based on configured zones
- Audio controls and volume
- System status display

### Configuration Pages

- `/config/zones` - Zone/pin setup and GPIO assignment
- `/config/effects` - Effect-to-group mappings and audio files
- `/config/device` - WiFi, hostname, system settings
- `/config/system` - OTA updates, diagnostics, factory reset

## Development Phases

### Phase 1 - Core System

- [x] Project structure and build system
- [x] Basic zone management and PWM LED control
- [x] Simple web interface for testing
- [x] OTA firmware updates
- [x] CandleFlicker and EngineIdle effects

### Phase 2 - Full Effects

- [x] WS2812B RGB support via FastLED
- [x] Complete effect library implementation
- [x] Hardware Serial DFPlayer audio integration
- [x] Effect synchronization and priority system

### Phase 3 - Configuration UI

- [x] Full web configuration interface
- [x] LittleFS persistent storage
- [x] Zone/group management UI
- [x] Effect testing and preview

### Phase 4 - Integration & Polish

- [ ] BattleSync WebSocket client (future)
- [x] Advanced effects and transitions
- [ ] Performance optimizations
- [ ] Documentation and user guides

## Development Notes

### Memory Constraints (ESP32-S3 8MB Flash)

- **File System**: LittleFS (SPIFFS deprecated)
- **Web Assets**: Embedded as PROGMEM strings, not files
- **Partition Table**: Custom for dual OTA + storage (1.5MB per OTA partition)
- **PSRAM**: 8MB available for FastLED buffers and dynamic allocations
- **Target**: <1.5MB firmware per OTA partition (currently ~1MB)

### Audio Configuration

- DFPlayer uses Hardware Serial (more reliable than SoftwareSerial)
- Files must be 0001.mp3, 0002.mp3, etc. in SD card root
- User manually maps file numbers during configuration
- Web interface provides test buttons to verify files
- System handles audio conflicts (pause ambient for active effects)

### Effect System Design

- **BaseEffect**: Abstract class for all effect implementations
- **Priority Levels**: Global (taking hits) > Active (weapons) > Ambient (idle)
- **Group Targeting**: Effects target logical groups, not specific pins
- **Hardware Adaptation**: Same effect works on PWM (brightness) or RGB (color+brightness)

## Claude Development Rules

### **Core Architecture Rules:**

1. **Follow the zone-based modular design** - Effects target groups, not hardcoded pins
2. **Respect the priority system** - Global > Active > Ambient, with proper restoration
3. **Abstract hardware differences** - Same effect code for PWM and WS2812B zones
4. **Maintain configuration-driven approach** - No hardcoded zone assignments
5. **Use LittleFS for persistence** - SPIFFS is deprecated
6. **Hardware Serial for audio** - DFPlayer on GPIO43/44, more reliable than SoftwareSerial
7. **Embed web assets as PROGMEM** - Don't use filesystem for HTML/CSS/JS due to space and updating concerns
8. **Leverage PSRAM** - Use available 8MB PSRAM for large buffers and FastLED operations

### **Change Management Rules:**

9. **Build and test before committing** - Always run `~/.platformio/penv/bin/platformio run`
10. **Increment firmware version** - Update version string for functional changes
11. **Commit frequently with clear messages** - Explain the "why" not just "what"
12. **Consider memory usage** - Monitor flash/RAM, keep firmware under 1.5MB per OTA partition
13. **Follow existing patterns** - Use established code structure and naming
14. **Update CLAUDE.md** - Keep this document current as project evolves

### **Development Workflow Rules:**

15. **Implement phases sequentially** - Complete current phase before moving on
16. **Test each component** - Verify core functionality before integration
17. **Ask clarifying questions** - If requirements unclear, ask before implementing
18. **Document breaking changes** - Explain impacts of architectural changes
19. **Remove dead code aggressively** - Clean up old code that doesn't fit new architecture
20. **Use the todo system** - Track progress on multi-step implementations

### **Code Quality Rules:**

21. **Follow C++ best practices** - RAII, const correctness, proper memory management
22. **Avoid ESP32 naming conflicts** - Don't use DISABLED, LOW, HIGH in enums (reserved by Arduino)
23. **Prefer composition over inheritance** - Keep effect system flexible
24. **Handle errors gracefully** - Fail safely, provide fallbacks where possible
25. **Use semantic naming** - Classes/functions should clearly express intent
26. **Comment complex logic** - Explain algorithms and hardware interactions
27. **Validate GPIO usage** - Only use GPIO 1-9 (or 1-9, 43-44 if audio disabled)
28. **Test on actual hardware** - Simulator cannot test audio, LEDs, or WiFi properly

## Current Status

**Hardware**: Migrated to ESP32-S3 (Seeed Xiao ESP32-S3)
**Active Phase**: Phase 4 - Integration & Polish
**Firmware**: ~1MB (67.5% of OTA partition)

### Recent Changes

- Migrated from ESP32-C3 to ESP32-S3 hardware
- Updated GPIO configuration (1-9, with 43-44 available if audio disabled)
- Enabled 8MB PSRAM for improved performance
- Corrected UART pins for DFPlayer (TX=GPIO43, RX=GPIO44)
- Updated partition table configuration
- All core systems operational (zones, VFX, audio, web interface)

### Next Development Priorities

1. BattleSync WebSocket client integration (future)
2. Performance optimizations leveraging PSRAM
3. User documentation and setup guides
4. Field testing and bug fixes
