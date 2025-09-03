#include "Configuration.h"

namespace BattleAura {

Configuration::Configuration() {
    // Initialize with default values
}

bool Configuration::begin() {
    Serial.println("Configuration: Initializing LittleFS...");
    if (!LittleFS.begin(true)) {  // Format on fail, use default partition
        Serial.println("Configuration: Failed to initialize LittleFS");
        return false;
    }
    
    Serial.println("Configuration: Loading configuration...");
    if (!load()) {
        Serial.println("Configuration: Load failed, creating default configuration");
        createDefaultConfiguration();
        return save();
    }
    
    return true;
}

bool Configuration::load() {
    return loadFromLittleFS();
}

bool Configuration::save() {
    return saveToLittleFS();
}

bool Configuration::factoryReset() {
    Serial.println("Configuration: Performing factory reset");
    
    // Clear all data structures
    zones.clear();
    groups.clear();
    effectConfigs.clear();
    
    // Create default configuration
    createDefaultConfiguration();
    
    // Save to filesystem
    return save();
}

// Zone management
bool Configuration::addZone(const Zone& zone) {
    if (isGPIOInUse(zone.gpio, zone.id)) {
        Serial.printf("Configuration: GPIO %d already in use\n", zone.gpio);
        return false;
    }
    
    zones[zone.id] = zone;
    updateGroupMembership();
    return true;
}

bool Configuration::removeZone(uint8_t zoneId) {
    if (zones.find(zoneId) == zones.end()) {
        return false;
    }
    
    zones.erase(zoneId);
    updateGroupMembership();
    return true;
}

Zone* Configuration::getZone(uint8_t zoneId) {
    auto it = zones.find(zoneId);
    return (it != zones.end()) ? &it->second : nullptr;
}

const Zone* Configuration::getZone(uint8_t zoneId) const {
    auto it = zones.find(zoneId);
    return (it != zones.end()) ? &it->second : nullptr;
}

std::vector<Zone*> Configuration::getZonesByGroup(const String& groupName) {
    std::vector<Zone*> result;
    for (auto& pair : zones) {
        if (pair.second.groupName == groupName) {
            result.push_back(&pair.second);
        }
    }
    return result;
}

std::vector<Zone*> Configuration::getAllZones() {
    std::vector<Zone*> result;
    for (auto& pair : zones) {
        result.push_back(&pair.second);
    }
    return result;
}

const std::vector<Zone*> Configuration::getAllZones() const {
    std::vector<Zone*> result;
    for (auto& pair : const_cast<std::map<uint8_t, Zone>&>(zones)) {
        result.push_back(&pair.second);
    }
    return result;
}

uint8_t Configuration::getNextZoneId() const {
    uint8_t maxId = 0;
    for (const auto& pair : zones) {
        if (pair.first > maxId) {
            maxId = pair.first;
        }
    }
    return maxId + 1;
}

// Group management
bool Configuration::addGroup(const Group& group) {
    groups[group.name] = group;
    return true;
}

bool Configuration::removeGroup(const String& groupName) {
    return groups.erase(groupName) > 0;
}

Group* Configuration::getGroup(const String& groupName) {
    auto it = groups.find(groupName);
    return (it != groups.end()) ? &it->second : nullptr;
}

const Group* Configuration::getGroup(const String& groupName) const {
    auto it = groups.find(groupName);
    return (it != groups.end()) ? &it->second : nullptr;
}

std::vector<Group*> Configuration::getAllGroups() {
    std::vector<Group*> result;
    for (auto& pair : groups) {
        result.push_back(&pair.second);
    }
    return result;
}

const std::vector<Group*> Configuration::getAllGroups() const {
    std::vector<Group*> result;
    for (auto& pair : const_cast<std::map<String, Group>&>(groups)) {
        result.push_back(&pair.second);
    }
    return result;
}

void Configuration::updateGroupMembership() {
    // Clear all group memberships
    for (auto& pair : groups) {
        pair.second.zoneIds.clear();
    }
    
    // Rebuild from zones
    for (const auto& zonePair : zones) {
        const Zone& zone = zonePair.second;
        auto groupIt = groups.find(zone.groupName);
        if (groupIt != groups.end()) {
            groupIt->second.addZone(zone.id);
        } else {
            // Create group if it doesn't exist
            Group newGroup(zone.groupName);
            newGroup.addZone(zone.id);
            groups[zone.groupName] = newGroup;
        }
    }
}

// Effect configuration management
bool Configuration::addEffectConfig(const EffectConfig& effectConfig) {
    effectConfigs[effectConfig.name] = effectConfig;
    return true;
}

bool Configuration::removeEffectConfig(const String& effectName) {
    return effectConfigs.erase(effectName) > 0;
}

EffectConfig* Configuration::getEffectConfig(const String& effectName) {
    auto it = effectConfigs.find(effectName);
    return (it != effectConfigs.end()) ? &it->second : nullptr;
}

const EffectConfig* Configuration::getEffectConfig(const String& effectName) const {
    auto it = effectConfigs.find(effectName);
    return (it != effectConfigs.end()) ? &it->second : nullptr;
}

std::vector<EffectConfig*> Configuration::getEffectConfigsByGroup(const String& groupName) {
    std::vector<EffectConfig*> result;
    for (auto& pair : effectConfigs) {
        for (const String& targetGroup : pair.second.targetGroups) {
            if (targetGroup == groupName) {
                result.push_back(&pair.second);
                break;
            }
        }
    }
    return result;
}

std::vector<EffectConfig*> Configuration::getEffectConfigsByType(EffectType type) {
    std::vector<EffectConfig*> result;
    for (auto& pair : effectConfigs) {
        if (pair.second.type == type) {
            result.push_back(&pair.second);
        }
    }
    return result;
}

std::vector<EffectConfig*> Configuration::getAllEffectConfigs() {
    std::vector<EffectConfig*> result;
    for (auto& pair : effectConfigs) {
        result.push_back(&pair.second);
    }
    return result;
}

const std::vector<EffectConfig*> Configuration::getAllEffectConfigs() const {
    std::vector<EffectConfig*> result;
    for (auto& pair : const_cast<std::map<String, EffectConfig>&>(effectConfigs)) {
        result.push_back(&pair.second);
    }
    return result;
}

// Validation
bool Configuration::isValidGPIO(uint8_t gpio) const {
    // Valid GPIO pins for ESP32-C3: 2-10, 20-21 (if audio disabled)
    if (gpio >= 2 && gpio <= 10) return true;
    if (!deviceConfig.audioEnabled && (gpio == 20 || gpio == 21)) return true;
    return false;
}

bool Configuration::isGPIOInUse(uint8_t gpio, uint8_t excludeZoneId) const {
    for (const auto& pair : zones) {
        if (pair.second.gpio == gpio && pair.first != excludeZoneId) {
            return true;
        }
    }
    return false;
}

std::vector<uint8_t> Configuration::getAvailableGPIOs() const {
    std::vector<uint8_t> available;
    
    // Check GPIO 2-10
    for (uint8_t gpio = 2; gpio <= 10; gpio++) {
        if (!isGPIOInUse(gpio)) {
            available.push_back(gpio);
        }
    }
    
    // Check GPIO 20-21 if audio disabled
    if (!deviceConfig.audioEnabled) {
        if (!isGPIOInUse(20)) available.push_back(20);
        if (!isGPIOInUse(21)) available.push_back(21);
    }
    
    return available;
}

void Configuration::printStatus() const {
    Serial.println("=== BattleAura Configuration Status ===");
    Serial.printf("Device: %s\n", deviceConfig.deviceName.c_str());
    Serial.printf("Firmware: %s\n", deviceConfig.firmwareVersion.c_str());
    Serial.printf("Audio: %s (Volume: %d)\n", deviceConfig.audioEnabled ? "Enabled" : "Disabled", deviceConfig.audioVolume);
    
    Serial.printf("Zones: %d\n", zones.size());
    for (const auto& pair : zones) {
        const Zone& zone = pair.second;
        Serial.printf("  Zone %d: %s (GPIO %d, %s, Group: %s)\n", 
                     zone.id, zone.name.c_str(), zone.gpio, 
                     (zone.type == ZoneType::PWM) ? "PWM" : "WS2812B", 
                     zone.groupName.c_str());
    }
    
    Serial.printf("Groups: %d\n", groups.size());
    for (const auto& pair : groups) {
        const Group& group = pair.second;
        Serial.printf("  Group '%s': %d zones\n", group.name.c_str(), group.zoneIds.size());
    }
    
    Serial.printf("Effects: %d\n", effectConfigs.size());
    for (const auto& pair : effectConfigs) {
        const EffectConfig& effect = pair.second;
        String typeStr = (effect.type == EffectType::AMBIENT) ? "Ambient" :
                        (effect.type == EffectType::ACTIVE) ? "Active" : "Global";
        Serial.printf("  Effect '%s': %s (Audio: %d)\n", 
                     effect.name.c_str(), typeStr.c_str(), effect.audioFile);
    }
    
    std::vector<uint8_t> availableGPIOs = getAvailableGPIOs();
    Serial.printf("Available GPIOs: %d (", availableGPIOs.size());
    for (size_t i = 0; i < availableGPIOs.size(); i++) {
        Serial.printf("%d", availableGPIOs[i]);
        if (i < availableGPIOs.size() - 1) Serial.print(", ");
    }
    Serial.println(")");
}

size_t Configuration::getConfigSize() const {
    return zones.size() * sizeof(Zone) + 
           groups.size() * sizeof(Group) + 
           effectConfigs.size() * sizeof(EffectConfig);
}

// Private methods
bool Configuration::loadFromLittleFS() {
    // For Phase 1, just return false to trigger default creation
    return false;  // TODO: Implement actual loading
}

bool Configuration::saveToLittleFS() {
    // For Phase 1, just return true - we'll implement this later
    return true;   // TODO: Implement actual saving
}

void Configuration::createDefaultConfiguration() {
    Serial.println("Configuration: Creating default configuration");
    
    // Clear existing data
    zones.clear();
    groups.clear();
    effectConfigs.clear();
    
    // Set device defaults
    deviceConfig.deviceName = "BattleAura";
    deviceConfig.firmwareVersion = "2.2.1-audio-uart-fix";
    deviceConfig.audioEnabled = true;
    deviceConfig.audioVolume = 20;
    deviceConfig.otaPassword = "battlesync";
    deviceConfig.apPassword = "battlesync";
    
    // Start with no zones - user will configure via web interface
    // This allows testing with any hardware setup
    // Effects will be enabled automatically when zones are added
    
    Serial.println("Configuration: Default configuration created");
}

} // namespace BattleAura