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
    
    // Track management (delegated to Configuration)
    bool addTrack(const AudioTrack& track);
    bool removeTrack(uint16_t fileNumber);
    AudioTrack* getTrack(uint16_t fileNumber);
    AudioTrack* getTrack(const String& trackName);
    std::vector<AudioTrack*> getAllTracks();
    
    // Testing and diagnostics
    bool testTrack(uint16_t fileNumber);
    void printStatus() const;
    uint16_t getAvailableTrackCount() const;
    
    // Reconnection
    bool retryInitialization();
    void enablePeriodicRetries(bool enable = true);
    
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
    uint32_t lastRetryAttempt;
    bool enableRetries;
    
    // Hardware management
    bool initializeHardware();
    void checkPlayerStatus();
    bool waitForReady(uint32_t timeout = 1000);
    
    // Utilities
    void updateCurrentStatus();
    
    // Constants
    static const uint8_t AUDIO_RX_PIN = 44;  // ESP32-S3 D7/GPIO44 RX <- DFPlayer TX
    static const uint8_t AUDIO_TX_PIN = 43;  // ESP32-S3 D6/GPIO43 TX -> DFPlayer RX
    static const uint32_t AUDIO_BAUD = 9600;
    static const uint32_t STATUS_CHECK_INTERVAL = 500; // Check every 500ms
};

} // namespace BattleAura