#pragma once

#include <Arduino.h>
#include <vector>
#include <memory>
#include "BaseVFX.h"
#include "library/CandleVFX.h"
#include "library/EngineIdleVFX.h"
#include "library/WeaponFireVFX.h"
#include "library/DamageVFX.h"
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
    std::vector<Zone*> getZonesForGroups(const std::vector<String>& groupNames);
    void handleGlobalEffectPriority();
    void restorePreGlobalEffects();
    void initializeDefaultEffects();
};

} // namespace BattleAura