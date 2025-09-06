#include "VFXManager.h"

namespace BattleAura {

VFXManager::VFXManager(LedController& ledController, Configuration& config)
    : ledController(ledController), config(config) {
}

bool VFXManager::begin() {
    Serial.println("VFXManager: Initializing...");
    
    // Clear existing VFX
    vfxInstances.clear();
    vfxStates.clear();
    currentGlobalVFX = nullptr;
    
    // Create VFX instances
    vfxInstances.push_back(std::unique_ptr<BaseVFX>(new CandleVFX(ledController, config)));
    vfxInstances.push_back(std::unique_ptr<BaseVFX>(new EngineIdleVFX(ledController, config)));
    vfxInstances.push_back(std::unique_ptr<BaseVFX>(new WeaponFireVFX(ledController, config)));
    vfxInstances.push_back(std::unique_ptr<BaseVFX>(new DamageVFX(ledController, config)));
    
    // Initialize VFX states for priority management
    vfxStates.resize(vfxInstances.size());
    for (size_t i = 0; i < vfxInstances.size(); i++) {
        vfxStates[i].vfx = vfxInstances[i].get();
        vfxStates[i].wasEnabledBeforeGlobal = false;
        vfxStates[i].globalStartTime = 0;
        
        // Initialize each VFX
        vfxInstances[i]->begin();
    }
    
    // Start default ambient VFX based on configuration
    initializeDefaultVFX();
    
    Serial.printf("VFXManager: Initialized with %d VFX\n", vfxInstances.size());
    return true;
}

void VFXManager::update() {
    // Handle global VFX priority management
    handleGlobalVFXPriority();
    
    // Update all active VFX
    for (auto& vfx : vfxInstances) {
        vfx->update();
        
        // Auto-disable timed VFX that have completed
        if (vfx->isEnabled() && vfx->shouldStop()) {
            vfx->stop();
            Serial.printf("VFXManager: Auto-stopped timed VFX '%s'\n", vfx->getName().c_str());
        }
    }
}

bool VFXManager::triggerVFX(const String& vfxName, uint32_t duration) {
    BaseVFX* vfx = findVFX(vfxName);
    if (!vfx) {
        Serial.printf("VFXManager: VFX '%s' not found\n", vfxName.c_str());
        return false;
    }
    
    // Get scene configuration to determine target groups
    const SceneConfig* sceneConfig = config.getSceneConfig(vfxName);
    if (!sceneConfig) {
        Serial.printf("VFXManager: No configuration found for VFX '%s', applying to all zones\n", vfxName.c_str());
        vfx->trigger(duration);
        return true;
    }
    
    // Get target zones based on configured groups
    std::vector<Zone*> targetZones = getZonesForGroups(sceneConfig->targetGroups);
    
    if (targetZones.empty()) {
        Serial.printf("VFXManager: No zones found for VFX '%s' target groups\n", vfxName.c_str());
        return false;
    }
    
    Serial.printf("VFXManager: Triggering VFX '%s' on %d zones for %dms\n", 
                 vfxName.c_str(), targetZones.size(), duration);
    
    // Set target zones for this VFX
    vfx->setTargetZones(targetZones);
    vfx->trigger(duration);
    return true;
}

bool VFXManager::enableVFX(const String& vfxName) {
    BaseVFX* vfx = findVFX(vfxName);
    if (!vfx) {
        Serial.printf("VFXManager: VFX '%s' not found\n", vfxName.c_str());
        return false;
    }
    
    vfx->setEnabled(true);
    Serial.printf("VFXManager: Enabled VFX '%s'\n", vfxName.c_str());
    return true;
}

bool VFXManager::disableVFX(const String& vfxName) {
    BaseVFX* vfx = findVFX(vfxName);
    if (!vfx) {
        Serial.printf("VFXManager: VFX '%s' not found\n", vfxName.c_str());
        return false;
    }
    
    vfx->setEnabled(false);
    Serial.printf("VFXManager: Disabled VFX '%s'\n", vfxName.c_str());
    return true;
}

bool VFXManager::isVFXEnabled(const String& vfxName) const {
    const BaseVFX* vfx = findVFX(vfxName);
    return vfx ? vfx->isEnabled() : false;
}

void VFXManager::enableAmbientVFX() {
    Serial.println("VFXManager: Enabling ambient VFX");
    for (auto& vfx : vfxInstances) {
        if (vfx->getPriority() == VFXPriority::AMBIENT) {
            vfx->setEnabled(true);
        }
    }
}

void VFXManager::disableAmbientVFX() {
    Serial.println("VFXManager: Disabling ambient VFX");
    for (auto& vfx : vfxInstances) {
        if (vfx->getPriority() == VFXPriority::AMBIENT) {
            vfx->setEnabled(false);
        }
    }
}

void VFXManager::stopActiveVFX() {
    Serial.println("VFXManager: Stopping active VFX");
    for (auto& vfx : vfxInstances) {
        if (vfx->getPriority() == VFXPriority::ACTIVE) {
            vfx->stop();
        }
    }
}

void VFXManager::stopGlobalVFX() {
    Serial.println("VFXManager: Stopping global VFX");
    for (auto& vfx : vfxInstances) {
        if (vfx->getPriority() == VFXPriority::GLOBAL) {
            vfx->stop();
        }
    }
    currentGlobalVFX = nullptr;
    restorePreGlobalVFX();
}

void VFXManager::stopAllVFX() {
    Serial.println("VFXManager: Stopping all VFX");
    for (auto& vfx : vfxInstances) {
        vfx->stop();
    }
    currentGlobalVFX = nullptr;
}

void VFXManager::printStatus() const {
    Serial.println("=== VFXManager Status ===");
    Serial.printf("Total VFX: %d\n", vfxInstances.size());
    Serial.printf("Current global VFX: %s\n", currentGlobalVFX ? currentGlobalVFX->getName().c_str() : "None");
    
    for (const auto& vfx : vfxInstances) {
        String priorityStr = (vfx->getPriority() == VFXPriority::AMBIENT) ? "AMBIENT" :
                            (vfx->getPriority() == VFXPriority::ACTIVE) ? "ACTIVE" : "GLOBAL";
        Serial.printf("  '%s': %s (%s)\n", 
                     vfx->getName().c_str(), 
                     vfx->isEnabled() ? "ENABLED" : "DISABLED",
                     priorityStr.c_str());
    }
}

std::vector<String> VFXManager::getVFXNames() const {
    std::vector<String> names;
    for (const auto& vfx : vfxInstances) {
        names.push_back(vfx->getName());
    }
    return names;
}

// Private methods

BaseVFX* VFXManager::findVFX(const String& vfxName) {
    for (auto& vfx : vfxInstances) {
        if (vfx->getName() == vfxName) {
            return vfx.get();
        }
    }
    return nullptr;
}

const BaseVFX* VFXManager::findVFX(const String& vfxName) const {
    for (const auto& vfx : vfxInstances) {
        if (vfx->getName() == vfxName) {
            return vfx.get();
        }
    }
    return nullptr;
}

void VFXManager::handleGlobalVFXPriority() {
    // Check if any global VFX is active
    BaseVFX* activeGlobal = nullptr;
    for (auto& vfx : vfxInstances) {
        if (vfx->getPriority() == VFXPriority::GLOBAL && vfx->isEnabled()) {
            activeGlobal = vfx.get();
            break;
        }
    }
    
    // Global VFX started
    if (activeGlobal && currentGlobalVFX != activeGlobal) {
        Serial.printf("VFXManager: Global VFX '%s' taking priority\n", activeGlobal->getName().c_str());
        
        // Store current VFX states
        for (size_t i = 0; i < vfxInstances.size(); i++) {
            vfxStates[i].wasEnabledBeforeGlobal = vfxInstances[i]->isEnabled();
            vfxStates[i].globalStartTime = millis();
            
            // Disable non-global VFX
            if (vfxInstances[i]->getPriority() != VFXPriority::GLOBAL) {
                vfxInstances[i]->setEnabled(false);
            }
        }
        
        currentGlobalVFX = activeGlobal;
    }
    
    // Global VFX ended
    if (!activeGlobal && currentGlobalVFX) {
        Serial.printf("VFXManager: Global VFX '%s' ended, restoring previous VFX\n", 
                     currentGlobalVFX->getName().c_str());
        currentGlobalVFX = nullptr;
        restorePreGlobalVFX();
    }
}

void VFXManager::restorePreGlobalVFX() {
    // Restore VFX that were enabled before global VFX started
    for (size_t i = 0; i < vfxInstances.size(); i++) {
        if (vfxInstances[i]->getPriority() != VFXPriority::GLOBAL) {
            vfxInstances[i]->setEnabled(vfxStates[i].wasEnabledBeforeGlobal);
        }
    }
}

void VFXManager::initializeDefaultVFX() {
    // Enable default ambient VFX based on configured scene configs
    auto sceneConfigs = config.getAllSceneConfigs();
    
    for (auto* sceneConfig : sceneConfigs) {
        if (sceneConfig->type == SceneType::AMBIENT) {
            // Enable ambient VFX by default
            enableVFX(sceneConfig->name);
        }
    }
    
    // If no scene configs exist, enable candle flicker as default
    if (sceneConfigs.empty()) {
        enableVFX("CandleFlicker");
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