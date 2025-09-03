#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "../config/Configuration.h"

namespace BattleAura {

enum class AudioStatus {
    STOPPED,
    PLAYING,
    PAUSED,
    ERROR
};

struct AudioTrack {
    uint16_t fileNumber;
    String description;
    bool isLoop;            // Ambient track that loops
    uint32_t duration;      // Duration in ms (0 = unknown/loop)
    
    AudioTrack() : fileNumber(0), isLoop(false), duration(0) {}
    AudioTrack(uint16_t num, const String& desc, bool loop = false, uint32_t dur = 0) 
        : fileNumber(num), description(desc), isLoop(loop), duration(dur) {}
};

class AudioController {
public:
    AudioController(Configuration& config);
    ~AudioController() = default;
    
    // Initialization
    bool begin();
    
    // Playback control
    bool play(uint16_t fileNumber, bool loop = false);
    bool playTrack(const String& trackName);
    bool stop();
    bool pause();
    bool resume();
    bool next();
    bool previous();
    
    // Volume control
    bool setVolume(uint8_t volume);  // 0-30
    uint8_t getVolume() const;
    bool volumeUp();
    bool volumeDown();
    
    // Status and info
    AudioStatus getStatus() const;
    uint16_t getCurrentTrack() const;
    bool isPlaying() const;
    bool isAvailable() const;
    
    // Track management
    bool addTrack(const AudioTrack& track);
    bool removeTrack(uint16_t fileNumber);
    AudioTrack* getTrack(uint16_t fileNumber);
    AudioTrack* getTrack(const String& trackName);
    std::vector<AudioTrack*> getAllTracks();
    
    // Testing and diagnostics
    bool testTrack(uint16_t fileNumber);
    void printStatus() const;
    uint16_t getAvailableTrackCount() const;
    
    // Update loop (check status, handle timeouts)
    void update();
    
private:
    Configuration& config;
    HardwareSerial* audioSerial;
    DFRobotDFPlayerMini dfPlayer;
    
    // Audio state
    AudioStatus currentStatus;
    uint16_t currentTrack;
    uint8_t currentVolume;
    uint32_t playStartTime;
    bool audioAvailable;
    uint32_t lastStatusCheck;
    
    // Track database
    std::vector<AudioTrack> tracks;
    
    // Hardware management
    bool initializeHardware();
    void checkPlayerStatus();
    bool waitForReady(uint32_t timeout = 1000);
    void initializeDefaultTracks();
    
    // Utilities
    AudioTrack* findTrack(uint16_t fileNumber);
    AudioTrack* findTrack(const String& trackName);
    void updateCurrentStatus();
    
    // Constants
    static const uint8_t AUDIO_RX_PIN = 21;  // ESP32 TX -> DFPlayer RX
    static const uint8_t AUDIO_TX_PIN = 20;  // ESP32 RX -> DFPlayer TX  
    static const uint32_t AUDIO_BAUD = 9600;
    static const uint32_t STATUS_CHECK_INTERVAL = 500; // Check every 500ms
};

} // namespace BattleAura