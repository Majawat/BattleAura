#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

namespace BattleAura {

enum class EffectType {
    AMBIENT,  // Always running (candle flicker, engine idle)
    ACTIVE,   // Triggered by user (weapon fire, engine rev) 
    GLOBAL    // System-wide effects (taking hits, destroyed)
};

enum class EffectState {
    IDLE,     // Effect not running
    RUNNING,  // Effect currently active
    STOPPING  // Effect finishing/fading out
};

struct EffectConfig {
    String name;                        // "CandleFlicker", "MachineGun", etc.
    EffectType type;                    // Ambient, Active, or Global
    std::vector<String> targetGroups;   // Groups this effect applies to
    uint16_t audioFile;                 // 0 = no audio, else file number (0001.mp3)
    String audioDescription;            // User's description of audio file
    uint32_t duration;                  // Duration in ms, 0 = infinite/ambient
    JsonDocument parameters;            // Effect-specific parameters  
    bool enabled;                       // Effect enabled/disabled
    
    EffectConfig() : type(EffectType::AMBIENT), audioFile(0), duration(0), enabled(true) {}
    
    EffectConfig(const String& _name, EffectType _type, uint32_t _duration = 0)
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
        return type == EffectType::AMBIENT;
    }
    
    bool isActive() const {
        return type == EffectType::ACTIVE;
    }
    
    bool isGlobal() const {
        return type == EffectType::GLOBAL;
    }
    
    bool isInfinite() const {
        return duration == 0;
    }
};

// Runtime effect instance
struct EffectInstance {
    const EffectConfig* config;  // Reference to configuration
    EffectState state;           // Current runtime state
    uint32_t startTime;          // When effect started (millis())
    uint32_t endTime;            // When effect should end (0 if infinite)
    
    EffectInstance() : config(nullptr), state(EffectState::IDLE), 
                      startTime(0), endTime(0) {}
                      
    EffectInstance(const EffectConfig* _config) 
        : config(_config), state(EffectState::IDLE), startTime(0), endTime(0) {}
    
    void start() {
        if (!config) return;
        
        state = EffectState::RUNNING;
        startTime = millis();
        
        if (config->duration > 0) {
            endTime = startTime + config->duration;
        } else {
            endTime = 0; // Infinite
        }
    }
    
    void stop() {
        state = EffectState::STOPPING;
    }
    
    bool isRunning() const {
        return state == EffectState::RUNNING;
    }
    
    bool isStopping() const {
        return state == EffectState::STOPPING;
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