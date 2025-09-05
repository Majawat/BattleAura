#pragma once

#include <Arduino.h>
#include "../hardware/LedController.h"
#include "../config/Configuration.h"

namespace BattleAura {

enum class VFXPriority {
    AMBIENT = 0,    // Background VFX (candle flicker, engine idle)
    ACTIVE = 1,     // Player-triggered VFX (weapon fire, engine rev)
    GLOBAL = 2      // System-wide VFX (taking damage, shutdown)
};

class BaseVFX {
public:
    BaseVFX(LedController& ledController, Configuration& config, 
            const String& name, VFXPriority priority)
        : ledController(ledController), config(config), 
          vfxName(name), priority(priority), enabled(false) {}
    
    virtual ~BaseVFX() = default;
    
    // Pure virtual methods - must be implemented by subclasses
    virtual void begin() = 0;
    virtual void update() = 0;
    
    // VFX control
    virtual void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
    // Target zone control
    void setTargetZones(const std::vector<Zone*>& zones) { targetZones = zones; }
    const std::vector<Zone*>& getTargetZones() const { return targetZones; }
    bool hasTargetZones() const { return !targetZones.empty(); }
    
    // VFX metadata
    const String& getName() const { return vfxName; }
    VFXPriority getPriority() const { return priority; }
    
    // Duration-based VFX (0 = continuous)
    virtual void trigger(uint32_t duration = 0) {
        if (duration > 0) {
            triggerTime = millis();
            triggerDuration = duration;
            setEnabled(true);
        } else {
            setEnabled(true);
        }
    }
    
    virtual void stop() {
        setEnabled(false);
        triggerDuration = 0;
    }
    
    // Check if timed VFX should stop
    bool shouldStop() const {
        return triggerDuration > 0 && 
               (millis() - triggerTime >= triggerDuration);
    }

protected:
    LedController& ledController;
    Configuration& config;
    String vfxName;
    VFXPriority priority;
    bool enabled;
    
    // Target zones (empty = all zones, for backward compatibility)
    std::vector<Zone*> targetZones;
    
    // Duration-based triggering
    uint32_t triggerTime = 0;
    uint32_t triggerDuration = 0;
};

} // namespace BattleAura