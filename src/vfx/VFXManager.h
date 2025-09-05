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

class VFXManager {
public:
    VFXManager(LedController& ledController, Configuration& config);
    ~VFXManager() = default;
    
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
    
    // VFX instances
    std::vector<std::unique_ptr<BaseVFX>> vfxEffects;
    
    // Priority management
    struct VFXState {
        BaseVFX* vfx;
        bool wasEnabledBeforeGlobal;
        uint32_t globalStartTime;
    };
    
    std::vector<VFXState> vfxStates;
    BaseVFX* currentGlobalVFX = nullptr;
    
    // Helper methods
    BaseVFX* findVFX(const String& vfxName);
    const BaseVFX* findVFX(const String& vfxName) const;
    std::vector<Zone*> getZonesForGroups(const std::vector<String>& groupNames);
    void handleGlobalEffectPriority();
    void restorePreGlobalEffects();
    void initializeDefaultEffects();
};

} // namespace BattleAura