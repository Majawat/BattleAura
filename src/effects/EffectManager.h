#pragma once

#include <Arduino.h>
#include <vector>
#include <memory>
#include "BaseEffect.h"
#include "library/CandleEffect.h"
#include "library/EngineIdleEffect.h"
#include "library/WeaponFireEffect.h"
#include "library/DamageEffect.h"
#include "../hardware/LedController.h"
#include "../config/Configuration.h"

namespace BattleAura {

class EffectManager {
public:
    EffectManager(LedController& ledController, Configuration& config);
    ~EffectManager() = default;
    
    // Initialization
    bool begin();
    
    // Main update loop
    void update();
    
    // Effect control by name
    bool triggerEffect(const String& effectName, uint32_t duration = 0);
    bool enableEffect(const String& effectName);
    bool disableEffect(const String& effectName);
    bool isEffectEnabled(const String& effectName) const;
    
    // Effect control by type
    void enableAmbientEffects();
    void disableAmbientEffects();
    void stopActiveEffects();
    void stopGlobalEffects();
    void stopAllEffects();
    
    // Status and debugging
    void printStatus() const;
    std::vector<String> getEffectNames() const;
    
private:
    LedController& ledController;
    Configuration& config;
    
    // Effect instances
    std::vector<std::unique_ptr<BaseEffect>> effects;
    
    // Priority management
    struct EffectState {
        BaseEffect* effect;
        bool wasEnabledBeforeGlobal;
        uint32_t globalStartTime;
    };
    
    std::vector<EffectState> effectStates;
    BaseEffect* currentGlobalEffect = nullptr;
    
    // Helper methods
    BaseEffect* findEffect(const String& effectName);
    const BaseEffect* findEffect(const String& effectName) const;
    void handleGlobalEffectPriority();
    void restorePreGlobalEffects();
    void initializeDefaultEffects();
};

} // namespace BattleAura