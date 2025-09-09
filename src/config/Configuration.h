#pragma once

#include "ZoneConfig.h"
#include "SceneConfig.h"
#include <map>
#include <LittleFS.h>

namespace BattleAura {

// Audio track configuration structure
struct AudioTrack {
    uint16_t fileNumber;
    String description;
    bool isLoop;            // Ambient track that loops
    uint32_t duration;      // Duration in ms (0 = unknown/loop)
    
    AudioTrack() : fileNumber(0), isLoop(false), duration(0) {}
    AudioTrack(uint16_t num, const String& desc, bool loop = false, uint32_t dur = 0) 
        : fileNumber(num), description(desc), isLoop(loop), duration(dur) {}
};

struct DeviceConfig {
    String deviceName;
    String wifiSSID;
    String wifiPassword;
    String otaPassword;
    String apPassword;
    uint8_t audioVolume;
    bool audioEnabled;
    uint8_t globalBrightness;
    String firmwareVersion;
    
    DeviceConfig() : deviceName("BattleAura"), otaPassword("battlesync"), 
                    apPassword("battlesync"), audioVolume(20), audioEnabled(true),
                    globalBrightness(255), firmwareVersion("2.9.0-complete-vfx-library") {}
};

class Configuration {
public:
    Configuration();
    
    // Initialization
    bool begin();
    bool load();
    bool save();
    bool factoryReset();
    
    // Zone management
    bool addZone(const Zone& zone);
    bool removeZone(uint8_t zoneId);
    Zone* getZone(uint8_t zoneId);
    const Zone* getZone(uint8_t zoneId) const;
    std::vector<Zone*> getZonesByGroup(const String& groupName);
    std::vector<Zone*> getAllZones();
    const std::vector<Zone*> getAllZones() const;
    uint8_t getNextZoneId() const;
    
    // Group management  
    bool addGroup(const Group& group);
    bool removeGroup(const String& groupName);
    Group* getGroup(const String& groupName);
    const Group* getGroup(const String& groupName) const;
    std::vector<Group*> getAllGroups();
    const std::vector<Group*> getAllGroups() const;
    void updateGroupMembership(); // Rebuild group memberships from zones
    
    // Scene configuration management
    bool addSceneConfig(const SceneConfig& sceneConfig);
    bool removeSceneConfig(const String& sceneName);
    SceneConfig* getSceneConfig(const String& sceneName);
    const SceneConfig* getSceneConfig(const String& sceneName) const;
    std::vector<SceneConfig*> getSceneConfigsByGroup(const String& groupName);
    std::vector<SceneConfig*> getSceneConfigsByType(SceneType type);
    std::vector<SceneConfig*> getAllSceneConfigs();
    const std::vector<SceneConfig*> getAllSceneConfigs() const;
    
    // Audio track management
    bool addAudioTrack(const AudioTrack& track);
    bool removeAudioTrack(uint16_t fileNumber);
    AudioTrack* getAudioTrack(uint16_t fileNumber);
    const AudioTrack* getAudioTrack(uint16_t fileNumber) const;
    AudioTrack* getAudioTrack(const String& description);
    const AudioTrack* getAudioTrack(const String& description) const;
    std::vector<AudioTrack*> getAllAudioTracks();
    const std::vector<AudioTrack*> getAllAudioTracks() const;
    void clearAllAudioTracks();
    
    // Device configuration
    DeviceConfig& getDeviceConfig() { return deviceConfig; }
    const DeviceConfig& getDeviceConfig() const { return deviceConfig; }
    
    // Validation
    bool isValidGPIO(uint8_t gpio) const;
    bool isGPIOInUse(uint8_t gpio, uint8_t excludeZoneId = 0) const;
    std::vector<uint8_t> getAvailableGPIOs() const;
    
    // Utility
    void printStatus() const;
    size_t getConfigSize() const;

private:
    std::map<uint8_t, Zone> zones;              // zoneId -> Zone
    std::map<String, Group> groups;             // groupName -> Group  
    std::map<String, SceneConfig> sceneConfigs; // sceneName -> SceneConfig
    std::map<uint16_t, AudioTrack> audioTracks; // fileNumber -> AudioTrack
    DeviceConfig deviceConfig;
    
    bool loadFromLittleFS();
    bool saveToLittleFS();
    void createDefaultConfiguration();
    JsonDocument serializeZones() const;
    JsonDocument serializeGroups() const;
    JsonDocument serializeSceneConfigs() const;
    JsonDocument serializeAudioTracks() const;
    JsonDocument serializeDeviceConfig() const;
    bool deserializeZones(const JsonDocument& doc);
    bool deserializeGroups(const JsonDocument& doc);
    bool deserializeSceneConfigs(const JsonDocument& doc);
    bool deserializeAudioTracks(const JsonDocument& doc);
    bool deserializeDeviceConfig(const JsonDocument& doc);
};

// Global configuration instance
extern Configuration config;

} // namespace BattleAura