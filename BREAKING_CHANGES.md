# Breaking Changes - v2.8.0 VFX + Scenes Terminology

## Overview
Version 2.8.0 introduces complete terminology consistency by eliminating ALL "effect" terminology in favor of "VFX + Scenes" throughout the entire codebase.

## Breaking Changes

### 1. API Endpoint Changes
- `/api/effects` → `/api/vfx`  
- All effect-related endpoints renamed to use VFX terminology

### 2. JSON API Changes
- Request parameter `effectName` → `vfxName`
- All API calls must use new parameter names

### 3. Configuration File Format Changes
- JSON key `"effectConfigs"` → `"sceneConfigs"`
- Existing configuration files are not supported

### 4. Web Interface Changes
- All JavaScript variables and DOM element IDs updated from "effect*" to "vfx*"
- Forms and controls use new VFX + Scenes terminology

### 5. Internal Method Name Changes
- `triggerEffect()` → `triggerVFX()`
- `enableEffect()` → `enableVFX()`
- `isEffectEnabled()` → `isVFXEnabled()`
- All manager methods updated

### 6. Data Structure Changes
- `EffectState` enum → `VFXState`
- `EffectInstance` struct → `VFXInstance`
- All internal storage uses VFX/Scene terminology

## Impact Assessment
- **Configuration**: Existing configuration files will not load
- **API Clients**: External clients must update to use new parameter names
- **Functionality**: Core functionality unchanged, only naming/terminology

## Rationale
Eliminates confusion between technical VFX classes and user-facing Scene configurations by using consistent terminology throughout the system.