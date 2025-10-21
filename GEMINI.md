# BattleAura Project Plan

This document outlines the software implementation plan for the BattleAura Effects Controller, a project to create a modular LED and sound effects controller for Warhammer miniatures.

## 1. Project Overview

The goal is to build a highly configurable effects controller using an ESP32-C3/S3 microcontroller. The system will feature a web-based interface for configuration and real-time control of LED and sound effects.

## 2. Core Architecture

The software will be organized into the following components:

-   `main.cpp`: The main application entry point, responsible for setup and the main loop.
-   `/vfx`: Manages all visual effects, including their priorities and transitions.
    -   `VFXManager.h/cpp`: The core of the visual effects system.
    -   `BaseVFX.h`: An abstract base class for all visual effects.
    -   `/library`: Individual visual effect implementations.
-   `/hardware`: Provides an abstraction layer for interacting with hardware components.
    -   `LedController.h/cpp`: A unified interface for controlling both PWM and WS2812B LEDs.
    -   `AudioController.h/cpp`: A wrapper for the DFPlayer Mini MP3 player.
-   `/web`: Implements the web-based configuration and control interface.
    -   `WebServer.h/cpp`: Based on the ESP32 AsyncWebServer.
    -   `WebInterface.h`: Contains the HTML, CSS, and JavaScript for the web UI, embedded as PROGMEM strings.
-   `/config`: Handles persistent storage of configuration data.
    -   `Configuration.h/cpp`: Manages loading and saving of configuration data to LittleFS.
    -   `SceneConfig.h`: Defines the structure for effect-to-zone mappings.
    -   `ZoneConfig.h`: Defines the structure for pin assignments and groups.

## 3. Data Models

The following data structures will be used to model the system's configuration:

-   **`Zone`**: Represents a physical LED output, with properties for name, GPIO pin, type (PWM or WS2812B), LED count, group, brightness, and enabled status.
-   **`Group`**: A logical collection of zones.
-   **`SceneConfig`**: Defines a visual effect scene, including its name, type (ambient, active, or global), target groups, associated audio file, duration, and effect-specific parameters.
-   **`EffectInstance`**: A runtime instance of a visual effect, with a pointer to its configuration, a start time, a state, and an implementation.

## 4. Effect System

The visual effect system is managed by the `VFXManager`, which handles effect priorities:

1.  **Global effects** (e.g., taking hits, destroyed)
2.  **Active effects** (e.g., weapon firing)
3.  **Ambient effects** (e.g., idle animations)

The `BaseVFX` class provides a common interface for all effect implementations, with methods for initialization, updating, and applying the effect to the LEDs.

## 5. Hardware Abstraction Layer

-   **`LedController`**: Provides a unified interface for setting the brightness and color of zones, regardless of whether they are PWM or WS2812B LEDs.
-   **`AudioController`**: A wrapper for the DFRobotDFPlayerMini library, managing audio playback and handling potential conflicts between ambient and active sound effects.

## 6. Web Interface

The web interface will be embedded in the firmware to save flash memory space. While currently embedded as `PROGMEM` strings, future iterations may explore serving a modern Single-Page Application (SPA) from LittleFS for enhanced development and user experience.

-   **Assets Strategy**: All HTML, CSS, and JavaScript will be minified, gzipped, and stored as `PROGMEM` strings.
-   **UI Framework**: The UI will be built with vanilla JavaScript to minimize its footprint.
-   **Configuration Pages**: The web interface will include pages for configuring zones, effects, and system settings.

## 7. Configuration and Storage

-   **File System**: LittleFS will be used for storing configuration files due to its reliability and performance.
-   **Partition Scheme**: A custom partition scheme will be used to allocate space for two OTA application partitions and a LittleFS filesystem.
-   **Configuration Files**: The configuration will be stored in three JSON files: `config.json`, `zones.json`, and `effects.json`.

## 8. Development Roadmap

The project will be implemented in the following phases:

1.  **Phase 1 - Core System**: Basic pin management, LED control (PWM), OTA updates, and a simple web interface for testing.
2.  **Phase 2 - Full Effects**: WS2812B support, a complete library of effects, and audio integration.
3.  **Phase 3 - Configuration**: The full web-based configuration interface and persistent storage.
4.  **Phase 4 - Polish**: BattleSync integration, advanced effects, and performance optimizations.

## 9. PlatformIO Configuration

```ini
[env:esp32-c3]
platform = espressif32
board = seeed_xiao_esp32c3
framework = arduino
monitor_speed = 115200
board_build.filesystem = littlefs
board_build.partitions = partitions.csv
lib_deps =
    bblanchon/ArduinoJson@^7.4.2
    fastled/FastLED@^3.10.3
    dfrobot/DFRobotDFPlayerMini@^1.0.6
    esp32async/ESPAsyncWebServer@^3.8.1

build_flags =
    -DCORE_DEBUG_LEVEL=3

lib_ignore = 
    WebServer

; Memory optimization
board_build.flash_mode = dio
board_build.f_cpu = 160000000L
board_build.f_flash = 80000000L
```

## 10. Implementation Notes

-   **Memory Optimization**: Careful use of `PROGMEM` and pre-allocated buffers will be necessary to stay within the ESP32-C3's memory limits.
-   **Pin Allocation**: GPIO pins are a limited resource and will be carefully managed.
-   **DFPlayer Configuration**: Audio files must be named `XXXX.mp3` and stored on the root of the SD card.
-   **Future Hardware Target**: The project aims to target ESP32-S3 devices in future iterations, leveraging its enhanced capabilities.
-   **OTA Update Strategy**: The current OTA update mechanism supports separate firmware and filesystem uploads via the web interface. Future plans include exploring a single-file OTA update process for improved user experience.