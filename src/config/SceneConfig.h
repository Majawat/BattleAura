#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

namespace BattleAura {

enum class SceneType {
    AMBIENT,  // Always running (candle flicker, engine idle)
    ACTIVE,   // Triggered by user (weapon fire, engine rev) 
    GLOBAL    // System-wide VFX (taking hits, destroyed)
};

enum class VFXState {
    IDLE,     // VFX not running
    RUNNING,  // VFX currently active
    STOPPING  // VFX finishing/fading out
};

struct SceneConfig {
    String name;                        // "CandleFlicker", "MachineGun", etc.
    SceneType type;                     // Ambient, Active, or Global
    std::vector<String> targetGroups;   // Groups this VFX applies to
    uint16_t audioFile;                 // 0 = no audio, else file number (0001.mp3)
    String audioDescription;            // User's description of audio file
    uint32_t duration;                  // Duration in ms, 0 = infinite/ambient
    JsonDocument parameters;            // VFX-specific parameters  
    bool enabled;                       // VFX enabled/disabled
    
    SceneConfig() : type(SceneType::AMBIENT), audioFile(0), duration(0), enabled(true) {}
    
    SceneConfig(const String& _name, SceneType _type, uint32_t _duration = 0)
        : name(_name), type(_type), audioFile(0), duration(_duration), enabled(true) {}
    
    void addTargetGroup(const String& groupName) {
        // Avoid duplicates
        for (const String& group : targetGroups) {
            if (group == groupName) return;
        }
        targetGroups.push_back(groupName);
    }
    
    void removeTargetGroup(const String& groupName) {
        targetGroups.erase(
            std::remove(targetGroups.begin(), targetGroups.end(), groupName),
            targetGroups.end()
        );
    }
    
    void setAudio(uint16_t fileNumber, const String& description = "") {
        audioFile = fileNumber;
        audioDescription = description;
    }
    
    bool hasAudio() const {
        return audioFile > 0;
    }
    
    bool isAmbient() const {
        return type == SceneType::AMBIENT;
    }
    
    bool isActive() const {
        return type == SceneType::ACTIVE;
    }
    
    bool isGlobal() const {
        return type == SceneType::GLOBAL;
    }
    
    bool isInfinite() const {
        return duration == 0;
    }
};

// Runtime VFX instance
struct VFXInstance {
    const SceneConfig* config;   // Reference to configuration
    VFXState state;              // Current runtime state
    uint32_t startTime;          // When VFX started (millis())
    uint32_t endTime;            // When VFX should end (0 if infinite)
    
    VFXInstance() : config(nullptr), state(VFXState::IDLE), 
                   startTime(0), endTime(0) {}
                      
    VFXInstance(const SceneConfig* _config) 
        : config(_config), state(VFXState::IDLE), startTime(0), endTime(0) {}
    
    void start() {
        if (!config) return;
        
        state = VFXState::RUNNING;
        startTime = millis();
        
        if (config->duration > 0) {
            endTime = startTime + config->duration;
        } else {
            endTime = 0; // Infinite
        }
    }
    
    void stop() {
        state = VFXState::STOPPING;
    }
    
    bool isRunning() const {
        return state == VFXState::RUNNING;
    }
    
    bool isStopping() const {
        return state == VFXState::STOPPING;
    }
    
    bool isExpired() const {
        if (endTime == 0) return false; // Infinite
        return millis() >= endTime;
    }
    
    uint32_t getElapsedTime() const {
        if (!isRunning()) return 0;
        return millis() - startTime;
    }
    
    uint32_t getRemainingTime() const {
        if (endTime == 0) return UINT32_MAX; // Infinite
        uint32_t now = millis();
        return (now >= endTime) ? 0 : (endTime - now);
    }
};

} // namespace BattleAura