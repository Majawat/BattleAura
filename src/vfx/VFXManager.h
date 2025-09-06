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
    
    // VFX control by name
    bool triggerVFX(const String& vfxName, uint32_t duration = 0);
    bool enableVFX(const String& vfxName);
    bool disableVFX(const String& vfxName);
    bool isVFXEnabled(const String& vfxName) const;
    
    // VFX control by type
    void enableAmbientVFX();
    void disableAmbientVFX();
    void stopActiveVFX();
    void stopGlobalVFX();
    void stopAllVFX();
    
    // Status and debugging
    void printStatus() const;
    std::vector<String> getVFXNames() const;
    
private:
    LedController& ledController;
    Configuration& config;
    
    // VFX instances
    std::vector<std::unique_ptr<BaseVFX>> vfxInstances;
    
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
    void handleGlobalVFXPriority();
    void restorePreGlobalVFX();
    void initializeDefaultVFX();
};

} // namespace BattleAura