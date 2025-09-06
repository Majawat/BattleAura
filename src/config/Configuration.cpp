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

// Scene configuration management
bool Configuration::addSceneConfig(const SceneConfig& effectConfig) {
    effectConfigs[effectConfig.name] = effectConfig;
    return true;
}

bool Configuration::removeSceneConfig(const String& effectName) {
    return effectConfigs.erase(effectName) > 0;
}

SceneConfig* Configuration::getSceneConfig(const String& effectName) {
    auto it = effectConfigs.find(effectName);
    return (it != effectConfigs.end()) ? &it->second : nullptr;
}

const SceneConfig* Configuration::getSceneConfig(const String& effectName) const {
    auto it = effectConfigs.find(effectName);
    return (it != effectConfigs.end()) ? &it->second : nullptr;
}

std::vector<SceneConfig*> Configuration::getSceneConfigsByGroup(const String& groupName) {
    std::vector<SceneConfig*> result;
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

std::vector<SceneConfig*> Configuration::getSceneConfigsByType(SceneType type) {
    std::vector<SceneConfig*> result;
    for (auto& pair : effectConfigs) {
        if (pair.second.type == type) {
            result.push_back(&pair.second);
        }
    }
    return result;
}

std::vector<SceneConfig*> Configuration::getAllSceneConfigs() {
    std::vector<SceneConfig*> result;
    for (auto& pair : effectConfigs) {
        result.push_back(&pair.second);
    }
    return result;
}

const std::vector<SceneConfig*> Configuration::getAllSceneConfigs() const {
    std::vector<SceneConfig*> result;
    for (auto& pair : const_cast<std::map<String, SceneConfig>&>(effectConfigs)) {
        result.push_back(&pair.second);
    }
    return result;
}

// Audio track management
bool Configuration::addAudioTrack(const AudioTrack& track) {
    audioTracks[track.fileNumber] = track;
    return true;
}

bool Configuration::removeAudioTrack(uint16_t fileNumber) {
    return audioTracks.erase(fileNumber) > 0;
}

AudioTrack* Configuration::getAudioTrack(uint16_t fileNumber) {
    auto it = audioTracks.find(fileNumber);
    return (it != audioTracks.end()) ? &it->second : nullptr;
}

const AudioTrack* Configuration::getAudioTrack(uint16_t fileNumber) const {
    auto it = audioTracks.find(fileNumber);
    return (it != audioTracks.end()) ? &it->second : nullptr;
}

AudioTrack* Configuration::getAudioTrack(const String& description) {
    for (auto& pair : audioTracks) {
        if (pair.second.description == description) {
            return &pair.second;
        }
    }
    return nullptr;
}

const AudioTrack* Configuration::getAudioTrack(const String& description) const {
    for (const auto& pair : audioTracks) {
        if (pair.second.description == description) {
            return &pair.second;
        }
    }
    return nullptr;
}

std::vector<AudioTrack*> Configuration::getAllAudioTracks() {
    std::vector<AudioTrack*> result;
    for (auto& pair : audioTracks) {
        result.push_back(&pair.second);
    }
    return result;
}

const std::vector<AudioTrack*> Configuration::getAllAudioTracks() const {
    std::vector<AudioTrack*> result;
    for (const auto& pair : audioTracks) {
        result.push_back(const_cast<AudioTrack*>(&pair.second));
    }
    return result;
}

void Configuration::clearAllAudioTracks() {
    audioTracks.clear();
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
    
    Serial.printf("Scenes: %d\n", effectConfigs.size());
    for (const auto& pair : effectConfigs) {
        const SceneConfig& scene = pair.second;
        String typeStr = (scene.type == SceneType::AMBIENT) ? "Ambient" :
                        (scene.type == SceneType::ACTIVE) ? "Active" : "Global";
        Serial.printf("  Scene '%s': %s (Audio: %d)\n", 
                     scene.name.c_str(), typeStr.c_str(), scene.audioFile);
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
           effectConfigs.size() * sizeof(SceneConfig);
}

// Private methods
bool Configuration::loadFromLittleFS() {
    Serial.println("Configuration: Attempting to load from LittleFS...");
    
    if (!LittleFS.exists("/config.json")) {
        Serial.println("Configuration: config.json does not exist");
        return false;
    }
    
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("Configuration: Failed to open config.json for reading");
        return false;
    }
    
    size_t fileSize = file.size();
    if (fileSize == 0) {
        Serial.println("Configuration: config.json is empty");
        file.close();
        return false;
    }
    
    // Read file content
    String jsonString = file.readString();
    file.close();
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        Serial.printf("Configuration: JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    // Load device config
    if (doc["device"]) {
        JsonObject deviceObj = doc["device"];
        deviceConfig.deviceName = deviceObj["name"] | "BattleAura";
        deviceConfig.wifiSSID = deviceObj["wifiSSID"] | "";
        deviceConfig.wifiPassword = deviceObj["wifiPassword"] | "";
        deviceConfig.audioEnabled = deviceObj["audioEnabled"] | true;
        deviceConfig.audioVolume = deviceObj["audioVolume"] | 20;
        deviceConfig.globalBrightness = deviceObj["globalBrightness"] | 255;
        deviceConfig.otaPassword = deviceObj["otaPassword"] | "battlesync";
        deviceConfig.apPassword = deviceObj["apPassword"] | "battlesync";
    }
    
    // Load zones
    if (doc["zones"]) {
        zones.clear();
        for (JsonPair zonePair : doc["zones"].as<JsonObject>()) {
            uint8_t zoneId = atoi(zonePair.key().c_str());
            JsonObject zoneObj = zonePair.value();
            
            Zone zone;
            zone.id = zoneId;
            zone.gpio = zoneObj["gpio"];
            zone.type = static_cast<ZoneType>(zoneObj["type"].as<int>());
            zone.groupName = zoneObj["group"] | "Default";
            zone.name = zoneObj["name"] | "";
            zone.brightness = zoneObj["brightness"] | 255;
            zone.ledCount = zoneObj["ledCount"] | 1;
            zone.enabled = zoneObj["enabled"] | true;
            
            zones[zoneId] = zone;
        }
    }
    
    // Load audio tracks
    if (doc["audioTracks"]) {
        audioTracks.clear();
        for (JsonPair trackPair : doc["audioTracks"].as<JsonObject>()) {
            uint16_t fileNumber = atoi(trackPair.key().c_str());
            JsonObject trackObj = trackPair.value();
            
            AudioTrack track;
            track.fileNumber = fileNumber;
            track.description = trackObj["description"] | "";
            track.isLoop = trackObj["isLoop"] | false;
            track.duration = trackObj["duration"] | 0;
            
            audioTracks[fileNumber] = track;
        }
    }
    
    // Load scene configs
    if (doc["effectConfigs"]) {
        effectConfigs.clear();
        for (JsonPair configPair : doc["effectConfigs"].as<JsonObject>()) {
            String effectName = configPair.key().c_str();
            JsonObject configObj = configPair.value();
            
            SceneConfig effectConfig;
            effectConfig.name = effectName;
            effectConfig.audioFile = configObj["audioFile"] | 0;
            effectConfig.duration = configObj["duration"] | 0;
            effectConfig.enabled = configObj["enabled"] | true;
            
            // Load type
            String typeStr = configObj["type"] | "AMBIENT";
            if (typeStr == "AMBIENT") {
                effectConfig.type = SceneType::AMBIENT;
            } else if (typeStr == "ACTIVE") {
                effectConfig.type = SceneType::ACTIVE;
            } else if (typeStr == "GLOBAL") {
                effectConfig.type = SceneType::GLOBAL;
            } else {
                effectConfig.type = SceneType::AMBIENT; // Default
            }
            
            // Load target groups
            if (configObj["targetGroups"]) {
                for (JsonVariant group : configObj["targetGroups"].as<JsonArray>()) {
                    String groupName = group.as<String>();
                    if (!groupName.isEmpty()) {
                        effectConfig.addTargetGroup(groupName);
                    }
                }
            }
            
            effectConfigs[effectName] = effectConfig;
        }
    }
    
    updateGroupMembership();
    
    Serial.printf("Configuration: Loaded %d zones, %d audio tracks, %d scene configs from LittleFS\n", 
                 zones.size(), audioTracks.size(), effectConfigs.size());
    return true;
}

bool Configuration::saveToLittleFS() {
    Serial.println("Configuration: Saving to LittleFS...");
    
    // Create JSON document
    JsonDocument doc;
    
    // Save device config
    JsonObject deviceObj = doc["device"].to<JsonObject>();
    deviceObj["name"] = deviceConfig.deviceName;
    deviceObj["wifiSSID"] = deviceConfig.wifiSSID;
    deviceObj["wifiPassword"] = deviceConfig.wifiPassword;
    deviceObj["audioEnabled"] = deviceConfig.audioEnabled;
    deviceObj["audioVolume"] = deviceConfig.audioVolume;
    deviceObj["globalBrightness"] = deviceConfig.globalBrightness;
    deviceObj["otaPassword"] = deviceConfig.otaPassword;
    deviceObj["apPassword"] = deviceConfig.apPassword;
    
    // Save zones
    JsonObject zonesObj = doc["zones"].to<JsonObject>();
    for (const auto& pair : zones) {
        const Zone& zone = pair.second;
        JsonObject zoneObj = zonesObj[String(zone.id)].to<JsonObject>();
        
        zoneObj["gpio"] = zone.gpio;
        zoneObj["type"] = static_cast<int>(zone.type);
        zoneObj["group"] = zone.groupName;
        zoneObj["name"] = zone.name;
        zoneObj["brightness"] = zone.brightness;
        zoneObj["ledCount"] = zone.ledCount;
        zoneObj["enabled"] = zone.enabled;
    }
    
    // Save audio tracks
    JsonObject audioTracksObj = doc["audioTracks"].to<JsonObject>();
    for (const auto& pair : audioTracks) {
        const AudioTrack& track = pair.second;
        JsonObject trackObj = audioTracksObj[String(track.fileNumber)].to<JsonObject>();
        
        trackObj["description"] = track.description;
        trackObj["isLoop"] = track.isLoop;
        trackObj["duration"] = track.duration;
    }
    
    // Save scene configs
    JsonObject effectConfigsObj = doc["effectConfigs"].to<JsonObject>();
    for (const auto& pair : effectConfigs) {
        const SceneConfig& effectConfig = pair.second;
        JsonObject configObj = effectConfigsObj[effectConfig.name].to<JsonObject>();
        
        configObj["audioFile"] = effectConfig.audioFile;
        configObj["duration"] = effectConfig.duration;
        configObj["enabled"] = effectConfig.enabled;
        
        // Save type
        configObj["type"] = (effectConfig.type == SceneType::AMBIENT) ? "AMBIENT" :
                           (effectConfig.type == SceneType::ACTIVE) ? "ACTIVE" : "GLOBAL";
        
        // Save target groups
        JsonArray groupsArray = configObj["targetGroups"].to<JsonArray>();
        for (const String& group : effectConfig.targetGroups) {
            groupsArray.add(group);
        }
    }
    
    // Open file for writing
    File file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println("Configuration: Failed to open config.json for writing");
        return false;
    }
    
    // Write JSON to file
    size_t bytesWritten = serializeJson(doc, file);
    file.close();
    
    if (bytesWritten == 0) {
        Serial.println("Configuration: Failed to write JSON to file");
        return false;
    }
    
    Serial.printf("Configuration: Saved %d bytes to LittleFS\n", bytesWritten);
    return true;
}

void Configuration::createDefaultConfiguration() {
    Serial.println("Configuration: Creating default configuration");
    
    // Clear existing data
    zones.clear();
    groups.clear();
    effectConfigs.clear();
    audioTracks.clear();
    
    // Set device defaults
    deviceConfig.deviceName = "BattleAura";
    deviceConfig.firmwareVersion = "2.5.1-user-workflow";
    deviceConfig.audioEnabled = true;
    deviceConfig.audioVolume = 20;
    deviceConfig.otaPassword = "battlesync";
    deviceConfig.apPassword = "battlesync";
    
    // Start with no zones or audio tracks - user will configure via web interface
    // This allows testing with any hardware setup
    // VFX will be enabled automatically when zones are added
    
    Serial.println("Configuration: Default configuration created");
}

} // namespace BattleAura