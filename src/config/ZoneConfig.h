#pragma once

#include <Arduino.h>
#include <vector>

namespace BattleAura {

enum class ZoneType {
    PWM,      // Single color PWM LED
    WS2812B   // RGB addressable LED strip
};

struct Zone {
    uint8_t id;              // Unique zone ID
    String name;             // "Engine LEDs Left" 
    uint8_t gpio;           // GPIO pin (2-10, 20-21 if audio disabled)
    ZoneType type;          // PWM or WS2812B
    uint8_t ledCount;       // Number of LEDs (1 for PWM, >1 for WS2812B)
    String groupName;       // "Engines", "Weapons", "Candles", etc.
    uint8_t brightness;     // 0-255 max brightness for this zone
    bool enabled;           // Zone enabled/disabled
    
    Zone() : id(0), gpio(0), type(ZoneType::PWM), ledCount(1), 
             brightness(255), enabled(false) {}
             
    Zone(uint8_t _id, const String& _name, uint8_t _gpio, ZoneType _type, 
         uint8_t _ledCount, const String& _groupName, uint8_t _brightness = 255) 
        : id(_id), name(_name), gpio(_gpio), type(_type), ledCount(_ledCount), 
          groupName(_groupName), brightness(_brightness), enabled(true) {}
};

struct Group {
    String name;                    // "Engines", "Weapons", etc.
    std::vector<uint8_t> zoneIds;   // Zone IDs belonging to this group
    bool enabled;                   // Group enabled/disabled
    
    Group() : enabled(true) {}
    
    Group(const String& _name) : name(_name), enabled(true) {}
    
    void addZone(uint8_t zoneId) {
        // Avoid duplicates
        for (uint8_t id : zoneIds) {
            if (id == zoneId) return;
        }
        zoneIds.push_back(zoneId);
    }
    
    void removeZone(uint8_t zoneId) {
        zoneIds.erase(
            std::remove(zoneIds.begin(), zoneIds.end(), zoneId), 
            zoneIds.end()
        );
    }
};

} // namespace BattleAura