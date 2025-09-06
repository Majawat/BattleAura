#include "VFXManager.h"

namespace BattleAura {

VFXManager::VFXManager(LedController& ledController, Configuration& config)
    : ledController(ledController), config(config) {
}

bool VFXManager::begin() {
    Serial.println("VFXManager: Initializing...");
    
    // Clear existing VFX
    vfxEffects.clear();
    vfxStates.clear();
    currentGlobalVFX = nullptr;
    
    // Create VFX instances
    vfxEffects.push_back(std::unique_ptr<BaseVFX>(new CandleVFX(ledController, config)));
    vfxEffects.push_back(std::unique_ptr<BaseVFX>(new EngineIdleVFX(ledController, config)));
    vfxEffects.push_back(std::unique_ptr<BaseVFX>(new WeaponFireVFX(ledController, config)));
    vfxEffects.push_back(std::unique_ptr<BaseVFX>(new DamageVFX(ledController, config)));
    
    // Initialize VFX states for priority management
    vfxStates.resize(vfxEffects.size());
    for (size_t i = 0; i < vfxEffects.size(); i++) {
        vfxStates[i].vfx = vfxEffects[i].get();
        vfxStates[i].wasEnabledBeforeGlobal = false;
        vfxStates[i].globalStartTime = 0;
        
        // Initialize each VFX
        vfxEffects[i]->begin();
    }
    
    // Start default ambient VFX based on configuration
    initializeDefaultEffects();
    
    Serial.printf("VFXManager: Initialized with %d VFX\n", vfxEffects.size());
    return true;
}

void VFXManager::update() {
    // Handle global VFX priority management
    handleGlobalEffectPriority();
    
    // Update all active VFX
    for (auto& vfx : vfxEffects) {
        vfx->update();
        
        // Auto-disable timed VFX that have completed
        if (vfx->isEnabled() && vfx->shouldStop()) {
            vfx->stop();
            Serial.printf("VFXManager: Auto-stopped timed VFX '%s'\n", vfx->getName().c_str());
        }
    }
}

bool VFXManager::triggerEffect(const String& effectName, uint32_t duration) {
    BaseVFX* vfx = findVFX(effectName);
    if (!vfx) {
        Serial.printf("VFXManager: VFX '%s' not found\n", effectName.c_str());
        return false;
    }
    
    // Get scene configuration to determine target groups
    const SceneConfig* effectConfig = config.getSceneConfig(effectName);
    if (!effectConfig) {
        Serial.printf("VFXManager: No configuration found for VFX '%s', applying to all zones\n", effectName.c_str());
        vfx->trigger(duration);
        return true;
    }
    
    // Get target zones based on configured groups
    std::vector<Zone*> targetZones = getZonesForGroups(effectConfig->targetGroups);
    
    if (targetZones.empty()) {
        Serial.printf("VFXManager: No zones found for VFX '%s' target groups\n", effectName.c_str());
        return false;
    }
    
    Serial.printf("VFXManager: Triggering VFX '%s' on %d zones for %dms\n", 
                 effectName.c_str(), targetZones.size(), duration);
    
    // Set target zones for this VFX
    vfx->setTargetZones(targetZones);
    vfx->trigger(duration);
    return true;
}

bool VFXManager::enableEffect(const String& effectName) {
    BaseVFX* vfx = findVFX(effectName);
    if (!vfx) {
        Serial.printf("VFXManager: VFX '%s' not found\n", effectName.c_str());
        return false;
    }
    
    vfx->setEnabled(true);
    Serial.printf("VFXManager: Enabled VFX '%s'\n", effectName.c_str());
    return true;
}

bool VFXManager::disableEffect(const String& effectName) {
    BaseVFX* vfx = findVFX(effectName);
    if (!vfx) {
        Serial.printf("VFXManager: VFX '%s' not found\n", effectName.c_str());
        return false;
    }
    
    vfx->setEnabled(false);
    Serial.printf("VFXManager: Disabled VFX '%s'\n", effectName.c_str());
    return true;
}

bool VFXManager::isEffectEnabled(const String& effectName) const {
    const BaseVFX* vfx = findVFX(effectName);
    return vfx ? vfx->isEnabled() : false;
}

void VFXManager::enableAmbientEffects() {
    Serial.println("VFXManager: Enabling ambient VFX");
    for (auto& vfx : vfxEffects) {
        if (vfx->getPriority() == VFXPriority::AMBIENT) {
            vfx->setEnabled(true);
        }
    }
}

void VFXManager::disableAmbientEffects() {
    Serial.println("VFXManager: Disabling ambient VFX");
    for (auto& vfx : vfxEffects) {
        if (vfx->getPriority() == VFXPriority::AMBIENT) {
            vfx->setEnabled(false);
        }
    }
}

void VFXManager::stopActiveEffects() {
    Serial.println("VFXManager: Stopping active VFX");
    for (auto& vfx : vfxEffects) {
        if (vfx->getPriority() == VFXPriority::ACTIVE) {
            vfx->stop();
        }
    }
}

void VFXManager::stopGlobalEffects() {
    Serial.println("VFXManager: Stopping global VFX");
    for (auto& vfx : vfxEffects) {
        if (vfx->getPriority() == VFXPriority::GLOBAL) {
            vfx->stop();
        }
    }
    currentGlobalVFX = nullptr;
    restorePreGlobalEffects();
}

void VFXManager::stopAllEffects() {
    Serial.println("VFXManager: Stopping all VFX");
    for (auto& vfx : vfxEffects) {
        vfx->stop();
    }
    currentGlobalVFX = nullptr;
}

void VFXManager::printStatus() const {
    Serial.println("=== VFXManager Status ===");
    Serial.printf("Total VFX: %d\n", vfxEffects.size());
    Serial.printf("Current global VFX: %s\n", currentGlobalVFX ? currentGlobalVFX->getName().c_str() : "None");
    
    for (const auto& vfx : vfxEffects) {
        String priorityStr = (vfx->getPriority() == VFXPriority::AMBIENT) ? "AMBIENT" :
                            (vfx->getPriority() == VFXPriority::ACTIVE) ? "ACTIVE" : "GLOBAL";
        Serial.printf("  '%s': %s (%s)\n", 
                     vfx->getName().c_str(), 
                     vfx->isEnabled() ? "ENABLED" : "DISABLED",
                     priorityStr.c_str());
    }
}

std::vector<String> VFXManager::getEffectNames() const {
    std::vector<String> names;
    for (const auto& vfx : vfxEffects) {
        names.push_back(vfx->getName());
    }
    return names;
}

// Private methods

BaseVFX* VFXManager::findVFX(const String& vfxName) {
    for (auto& vfx : vfxEffects) {
        if (vfx->getName() == vfxName) {
            return vfx.get();
        }
    }
    return nullptr;
}

const BaseVFX* VFXManager::findVFX(const String& vfxName) const {
    for (const auto& vfx : vfxEffects) {
        if (vfx->getName() == vfxName) {
            return vfx.get();
        }
    }
    return nullptr;
}

void VFXManager::handleGlobalEffectPriority() {
    // Check if any global VFX is active
    BaseVFX* activeGlobal = nullptr;
    for (auto& vfx : vfxEffects) {
        if (vfx->getPriority() == VFXPriority::GLOBAL && vfx->isEnabled()) {
            activeGlobal = vfx.get();
            break;
        }
    }
    
    // Global VFX started
    if (activeGlobal && currentGlobalVFX != activeGlobal) {
        Serial.printf("VFXManager: Global VFX '%s' taking priority\n", activeGlobal->getName().c_str());
        
        // Store current VFX states
        for (size_t i = 0; i < vfxEffects.size(); i++) {
            vfxStates[i].wasEnabledBeforeGlobal = vfxEffects[i]->isEnabled();
            vfxStates[i].globalStartTime = millis();
            
            // Disable non-global VFX
            if (vfxEffects[i]->getPriority() != VFXPriority::GLOBAL) {
                vfxEffects[i]->setEnabled(false);
            }
        }
        
        currentGlobalVFX = activeGlobal;
    }
    
    // Global VFX ended
    if (!activeGlobal && currentGlobalVFX) {
        Serial.printf("VFXManager: Global VFX '%s' ended, restoring previous VFX\n", 
                     currentGlobalVFX->getName().c_str());
        currentGlobalVFX = nullptr;
        restorePreGlobalEffects();
    }
}

void VFXManager::restorePreGlobalEffects() {
    // Restore VFX that were enabled before global VFX started
    for (size_t i = 0; i < vfxEffects.size(); i++) {
        if (vfxEffects[i]->getPriority() != VFXPriority::GLOBAL) {
            vfxEffects[i]->setEnabled(vfxStates[i].wasEnabledBeforeGlobal);
        }
    }
}

void VFXManager::initializeDefaultEffects() {
    // Enable default ambient VFX based on configured scene configs
    auto effectConfigs = config.getAllSceneConfigs();
    
    for (auto* effectConfig : effectConfigs) {
        if (effectConfig->type == SceneType::AMBIENT) {
            // Enable ambient VFX by default
            enableEffect(effectConfig->name);
        }
    }
    
    // If no scene configs exist, enable candle flicker as default
    if (effectConfigs.empty()) {
        enableEffect("CandleFlicker");
    }
}

std::vector<Zone*> VFXManager::getZonesForGroups(const std::vector<String>& groupNames) {
    std::vector<Zone*> targetZones;
    
    for (const String& groupName : groupNames) {
        auto groupZones = config.getZonesByGroup(groupName);
        for (Zone* zone : groupZones) {
            // Avoid duplicates if zone is in multiple target groups
            bool alreadyAdded = false;
            for (Zone* existing : targetZones) {
                if (existing->id == zone->id) {
                    alreadyAdded = true;
                    break;
                }
            }
            if (!alreadyAdded) {
                targetZones.push_back(zone);
            }
        }
    }
    
    return targetZones;
}

} // namespace BattleAura