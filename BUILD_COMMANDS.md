# BUILD COMMANDS FOR BATTLEARUA

## PlatformIO Build Commands

**BUILD THE FIRMWARE:**
```bash
~/.platformio/penv/bin/platformio run
```

**CLEAN BUILD:**
```bash
~/.platformio/penv/bin/platformio run -t clean
```

**BUILD WITH VERBOSE OUTPUT:**
```bash
~/.platformio/penv/bin/platformio run -v
```

**UPLOAD FILESYSTEM (SPIFFS):**
```bash
~/.platformio/penv/bin/platformio run -t uploadfs
```

## IMPORTANT NOTES

- **ALWAYS BUILD AFTER CODE CHANGES** - Don't commit without testing
- **PlatformIO is available** at `~/.platformio/penv/bin/platformio`
- **Build output shows:** Flash usage %, RAM usage %, any compilation errors
- **Success message:** "SUCCESS" at the end means it compiled properly
- **Firmware location:** `.pio/build/seeed_xiao_esp32c3/firmware.bin`

## Memory Targets

- **Flash usage:** Keep under 70% (we're at ~62%)
- **RAM usage:** Keep under 50% (we're at ~12%)

## Common ESP32 Naming Conflicts to AVOID

- `DISABLED` → use `PIN_DISABLED`
- `LOW` → use `PIN_LOW` 
- `HIGH` → use `PIN_HIGH`
- Any Arduino framework macro names

## Build Before Committing Checklist

1. Run build command
2. Check for compilation errors
3. Verify memory usage is reasonable
4. Only commit if build succeeds
5. Test incrementally - don't batch multiple features