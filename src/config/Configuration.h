#pragma once

#include "ZoneConfig.h"
#include "EffectConfig.h"
#include <map>
#include <LittleFS.h>

namespace BattleAura {

struct DeviceConfig {
    String deviceName;
    String wifiSSID;
    String wifiPassword;
    String otaPassword;
    String apPassword;
    uint8_t audioVolume;
    bool audioEnabled;
    String firmwareVersion;
    
    DeviceConfig() : deviceName("BattleAura"), otaPassword("battlesync"), 
                    apPassword("battlesync"), audioVolume(20), audioEnabled(true),
                    firmwareVersion("2.0.0") {}
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
    
    // Effect configuration management
    bool addEffectConfig(const EffectConfig& effectConfig);
    bool removeEffectConfig(const String& effectName);
    EffectConfig* getEffectConfig(const String& effectName);
    const EffectConfig* getEffectConfig(const String& effectName) const;
    std::vector<EffectConfig*> getEffectConfigsByGroup(const String& groupName);
    std::vector<EffectConfig*> getEffectConfigsByType(EffectType type);
    std::vector<EffectConfig*> getAllEffectConfigs();
    const std::vector<EffectConfig*> getAllEffectConfigs() const;
    
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
    std::map<String, EffectConfig> effectConfigs; // effectName -> EffectConfig
    DeviceConfig deviceConfig;
    
    bool loadFromLittleFS();
    bool saveToLittleFS();
    void createDefaultConfiguration();
    JsonDocument serializeZones() const;
    JsonDocument serializeGroups() const;
    JsonDocument serializeEffectConfigs() const;
    JsonDocument serializeDeviceConfig() const;
    bool deserializeZones(const JsonDocument& doc);
    bool deserializeGroups(const JsonDocument& doc);
    bool deserializeEffectConfigs(const JsonDocument& doc);
    bool deserializeDeviceConfig(const JsonDocument& doc);
};

// Global configuration instance
extern Configuration config;

} // namespace BattleAura