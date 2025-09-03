#pragma once

#include <Arduino.h>
#include "../hardware/LedController.h"
#include "../config/Configuration.h"

namespace BattleAura {

enum class EffectPriority {
    AMBIENT = 0,    // Background effects (candle flicker, engine idle)
    ACTIVE = 1,     // Player-triggered effects (weapon fire, engine rev)
    GLOBAL = 2      // System-wide effects (taking damage, shutdown)
};

class BaseEffect {
public:
    BaseEffect(LedController& ledController, Configuration& config, 
               const String& name, EffectPriority priority)
        : ledController(ledController), config(config), 
          effectName(name), priority(priority), enabled(false) {}
    
    virtual ~BaseEffect() = default;
    
    // Pure virtual methods - must be implemented by subclasses
    virtual void begin() = 0;
    virtual void update() = 0;
    
    // Effect control
    virtual void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
    // Effect metadata
    const String& getName() const { return effectName; }
    EffectPriority getPriority() const { return priority; }
    
    // Duration-based effects (0 = continuous)
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
    
    // Check if timed effect should stop
    bool shouldStop() const {
        return triggerDuration > 0 && 
               (millis() - triggerTime >= triggerDuration);
    }

protected:
    LedController& ledController;
    Configuration& config;
    String effectName;
    EffectPriority priority;
    bool enabled;
    
    // Duration-based triggering
    uint32_t triggerTime = 0;
    uint32_t triggerDuration = 0;
};

} // namespace BattleAura