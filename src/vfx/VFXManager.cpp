#include "VFXManager.h"

namespace BattleAura {

EffectManager::EffectManager(LedController& ledController, Configuration& config)
    : ledController(ledController), config(config) {
}

bool EffectManager::begin() {
    Serial.println("EffectManager: Initializing...");
    
    // Clear existing effects
    effects.clear();
    effectStates.clear();
    currentGlobalEffect = nullptr;
    
    // Create effect instances
    effects.push_back(std::unique_ptr<BaseEffect>(new CandleEffect(ledController, config)));
    effects.push_back(std::unique_ptr<BaseEffect>(new EngineIdleEffect(ledController, config)));
    effects.push_back(std::unique_ptr<BaseEffect>(new WeaponFireEffect(ledController, config)));
    effects.push_back(std::unique_ptr<BaseEffect>(new DamageEffect(ledController, config)));
    
    // Initialize effect states for priority management
    effectStates.resize(effects.size());
    for (size_t i = 0; i < effects.size(); i++) {
        effectStates[i].effect = effects[i].get();
        effectStates[i].wasEnabledBeforeGlobal = false;
        effectStates[i].globalStartTime = 0;
        
        // Initialize each effect
        effects[i]->begin();
    }
    
    // Start default ambient effects based on configuration
    initializeDefaultEffects();
    
    Serial.printf("EffectManager: Initialized with %d effects\n", effects.size());
    return true;
}

void EffectManager::update() {
    // Handle global effect priority management
    handleGlobalEffectPriority();
    
    // Update all active effects
    for (auto& effect : effects) {
        effect->update();
        
        // Auto-disable timed effects that have completed
        if (effect->isEnabled() && effect->shouldStop()) {
            effect->stop();
            Serial.printf("EffectManager: Auto-stopped timed effect '%s'\n", effect->getName().c_str());
        }
    }
}

bool EffectManager::triggerEffect(const String& effectName, uint32_t duration) {
    BaseEffect* effect = findEffect(effectName);
    if (!effect) {
        Serial.printf("EffectManager: Effect '%s' not found\n", effectName.c_str());
        return false;
    }
    
    // Get effect configuration to determine target groups
    const EffectConfig* effectConfig = config.getEffectConfig(effectName);
    if (!effectConfig) {
        Serial.printf("EffectManager: No configuration found for effect '%s', applying to all zones\n", effectName.c_str());
        effect->trigger(duration);
        return true;
    }
    
    // Get target zones based on configured groups
    std::vector<Zone*> targetZones = getZonesForGroups(effectConfig->targetGroups);
    
    if (targetZones.empty()) {
        Serial.printf("EffectManager: No zones found for effect '%s' target groups\n", effectName.c_str());
        return false;
    }
    
    Serial.printf("EffectManager: Triggering effect '%s' on %d zones for %dms\n", 
                 effectName.c_str(), targetZones.size(), duration);
    
    // Set target zones for this effect
    effect->setTargetZones(targetZones);
    effect->trigger(duration);
    return true;
}

bool EffectManager::enableEffect(const String& effectName) {
    BaseEffect* effect = findEffect(effectName);
    if (!effect) {
        Serial.printf("EffectManager: Effect '%s' not found\n", effectName.c_str());
        return false;
    }
    
    effect->setEnabled(true);
    Serial.printf("EffectManager: Enabled effect '%s'\n", effectName.c_str());
    return true;
}

bool EffectManager::disableEffect(const String& effectName) {
    BaseEffect* effect = findEffect(effectName);
    if (!effect) {
        Serial.printf("EffectManager: Effect '%s' not found\n", effectName.c_str());
        return false;
    }
    
    effect->setEnabled(false);
    Serial.printf("EffectManager: Disabled effect '%s'\n", effectName.c_str());
    return true;
}

bool EffectManager::isEffectEnabled(const String& effectName) const {
    const BaseEffect* effect = findEffect(effectName);
    return effect ? effect->isEnabled() : false;
}

void EffectManager::enableAmbientEffects() {
    Serial.println("EffectManager: Enabling ambient effects");
    for (auto& effect : effects) {
        if (effect->getPriority() == EffectPriority::AMBIENT) {
            effect->setEnabled(true);
        }
    }
}

void EffectManager::disableAmbientEffects() {
    Serial.println("EffectManager: Disabling ambient effects");
    for (auto& effect : effects) {
        if (effect->getPriority() == EffectPriority::AMBIENT) {
            effect->setEnabled(false);
        }
    }
}

void EffectManager::stopActiveEffects() {
    Serial.println("EffectManager: Stopping active effects");
    for (auto& effect : effects) {
        if (effect->getPriority() == EffectPriority::ACTIVE) {
            effect->stop();
        }
    }
}

void EffectManager::stopGlobalEffects() {
    Serial.println("EffectManager: Stopping global effects");
    for (auto& effect : effects) {
        if (effect->getPriority() == EffectPriority::GLOBAL) {
            effect->stop();
        }
    }
    currentGlobalEffect = nullptr;
    restorePreGlobalEffects();
}

void EffectManager::stopAllEffects() {
    Serial.println("EffectManager: Stopping all effects");
    for (auto& effect : effects) {
        effect->stop();
    }
    currentGlobalEffect = nullptr;
}

void EffectManager::printStatus() const {
    Serial.println("=== EffectManager Status ===");
    Serial.printf("Total effects: %d\n", effects.size());
    Serial.printf("Current global effect: %s\n", currentGlobalEffect ? currentGlobalEffect->getName().c_str() : "None");
    
    for (const auto& effect : effects) {
        String priorityStr = (effect->getPriority() == EffectPriority::AMBIENT) ? "AMBIENT" :
                            (effect->getPriority() == EffectPriority::ACTIVE) ? "ACTIVE" : "GLOBAL";
        Serial.printf("  '%s': %s (%s)\n", 
                     effect->getName().c_str(), 
                     effect->isEnabled() ? "ENABLED" : "DISABLED",
                     priorityStr.c_str());
    }
}

std::vector<String> EffectManager::getEffectNames() const {
    std::vector<String> names;
    for (const auto& effect : effects) {
        names.push_back(effect->getName());
    }
    return names;
}

// Private methods

BaseEffect* EffectManager::findEffect(const String& effectName) {
    for (auto& effect : effects) {
        if (effect->getName() == effectName) {
            return effect.get();
        }
    }
    return nullptr;
}

const BaseEffect* EffectManager::findEffect(const String& effectName) const {
    for (const auto& effect : effects) {
        if (effect->getName() == effectName) {
            return effect.get();
        }
    }
    return nullptr;
}

void EffectManager::handleGlobalEffectPriority() {
    // Check if any global effect is active
    BaseEffect* activeGlobal = nullptr;
    for (auto& effect : effects) {
        if (effect->getPriority() == EffectPriority::GLOBAL && effect->isEnabled()) {
            activeGlobal = effect.get();
            break;
        }
    }
    
    // Global effect started
    if (activeGlobal && currentGlobalEffect != activeGlobal) {
        Serial.printf("EffectManager: Global effect '%s' taking priority\n", activeGlobal->getName().c_str());
        
        // Store current effect states
        for (size_t i = 0; i < effects.size(); i++) {
            effectStates[i].wasEnabledBeforeGlobal = effects[i]->isEnabled();
            effectStates[i].globalStartTime = millis();
            
            // Disable non-global effects
            if (effects[i]->getPriority() != EffectPriority::GLOBAL) {
                effects[i]->setEnabled(false);
            }
        }
        
        currentGlobalEffect = activeGlobal;
    }
    
    // Global effect ended
    if (!activeGlobal && currentGlobalEffect) {
        Serial.printf("EffectManager: Global effect '%s' ended, restoring previous effects\n", 
                     currentGlobalEffect->getName().c_str());
        currentGlobalEffect = nullptr;
        restorePreGlobalEffects();
    }
}

void EffectManager::restorePreGlobalEffects() {
    // Restore effects that were enabled before global effect started
    for (size_t i = 0; i < effects.size(); i++) {
        if (effects[i]->getPriority() != EffectPriority::GLOBAL) {
            effects[i]->setEnabled(effectStates[i].wasEnabledBeforeGlobal);
        }
    }
}

void EffectManager::initializeDefaultEffects() {
    // Enable default ambient effects based on configured effect configs
    auto effectConfigs = config.getAllEffectConfigs();
    
    for (auto* effectConfig : effectConfigs) {
        if (effectConfig->type == EffectType::AMBIENT) {
            // Enable ambient effects by default
            enableEffect(effectConfig->name);
        }
    }
    
    // If no effect configs exist, enable candle flicker as default
    if (effectConfigs.empty()) {
        enableEffect("CandleFlicker");
    }
}

std::vector<Zone*> EffectManager::getZonesForGroups(const std::vector<String>& groupNames) {
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