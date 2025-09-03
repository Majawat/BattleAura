#include "AudioController.h"

namespace BattleAura {

AudioController::AudioController(Configuration& config) 
    : config(config), audioSerial(nullptr), currentStatus(AudioStatus::STOPPED),
      currentTrack(0), currentVolume(15), playStartTime(0), audioAvailable(false),
      lastStatusCheck(0) {
}

bool AudioController::begin() {
    Serial.println("AudioController: Initializing...");
    
    const auto& deviceConfig = config.getDeviceConfig();
    
    // Skip initialization if audio is disabled
    if (!deviceConfig.audioEnabled) {
        Serial.println("AudioController: Audio disabled in configuration");
        audioAvailable = false;
        return true; // Return success but mark as unavailable
    }
    
    // Initialize Hardware Serial for DFPlayer
    audioSerial = &Serial1;  // Use Hardware Serial 1
    Serial.printf("AudioController: Initializing UART on RX=%d, TX=%d at %d baud\n", 
                 AUDIO_RX_PIN, AUDIO_TX_PIN, AUDIO_BAUD);
    audioSerial->begin(AUDIO_BAUD, SERIAL_8N1, AUDIO_RX_PIN, AUDIO_TX_PIN);
    
    // Give DFPlayer time to initialize
    Serial.println("AudioController: Waiting 1000ms for DFPlayer startup...");
    delay(1000);
    
    // Initialize DFPlayer
    Serial.println("AudioController: Calling dfPlayer.begin()...");
    if (!dfPlayer.begin(*audioSerial)) {
        Serial.println("AudioController: dfPlayer.begin() failed");
        Serial.println("AudioController: Check DFPlayer connections and SD card");
        audioAvailable = false;
        return false;
    }
    Serial.println("AudioController: dfPlayer.begin() succeeded");
    
    // Give DFPlayer additional time to stabilize
    Serial.println("AudioController: Waiting additional 500ms for DFPlayer stabilization...");
    delay(500);
    
    audioAvailable = true;
    
    // Set initial volume from config
    currentVolume = deviceConfig.audioVolume;
    if (currentVolume > 30) currentVolume = 30;
    dfPlayer.volume(currentVolume);
    
    // Initialize default tracks
    initializeDefaultTracks();
    
    Serial.printf("AudioController: Ready (Volume: %d, Tracks: %d)\n", 
                 currentVolume, tracks.size());
    return true;
}

bool AudioController::play(uint16_t fileNumber, bool loop) {
    if (!audioAvailable) {
        Serial.println("AudioController: Audio not available");
        return false;
    }
    
    if (fileNumber == 0) {
        Serial.println("AudioController: Invalid file number 0");
        return false;
    }
    
    Serial.printf("AudioController: Playing file %d %s\n", fileNumber, loop ? "(loop)" : "");
    
    if (loop) {
        dfPlayer.loop(fileNumber);
    } else {
        dfPlayer.play(fileNumber);
    }
    
    currentTrack = fileNumber;
    currentStatus = AudioStatus::PLAYING;
    playStartTime = millis();
    
    return true;
}

bool AudioController::playTrack(const String& trackName) {
    AudioTrack* track = findTrack(trackName);
    if (!track) {
        Serial.printf("AudioController: Track '%s' not found\n", trackName.c_str());
        return false;
    }
    
    return play(track->fileNumber, track->isLoop);
}

bool AudioController::stop() {
    if (!audioAvailable) return false;
    
    dfPlayer.stop();
    currentStatus = AudioStatus::STOPPED;
    currentTrack = 0;
    playStartTime = 0;
    
    Serial.println("AudioController: Stopped");
    return true;
}

bool AudioController::pause() {
    if (!audioAvailable) return false;
    
    dfPlayer.pause();
    currentStatus = AudioStatus::PAUSED;
    
    Serial.println("AudioController: Paused");
    return true;
}

bool AudioController::resume() {
    if (!audioAvailable) return false;
    
    dfPlayer.start();
    currentStatus = AudioStatus::PLAYING;
    
    Serial.println("AudioController: Resumed");
    return true;
}

bool AudioController::next() {
    if (!audioAvailable) return false;
    
    dfPlayer.next();
    updateCurrentStatus();
    
    Serial.println("AudioController: Next track");
    return true;
}

bool AudioController::previous() {
    if (!audioAvailable) return false;
    
    dfPlayer.previous();
    updateCurrentStatus();
    
    Serial.println("AudioController: Previous track");
    return true;
}

bool AudioController::setVolume(uint8_t volume) {
    if (!audioAvailable) return false;
    
    if (volume > 30) volume = 30;
    
    dfPlayer.volume(volume);
    currentVolume = volume;
    
    Serial.printf("AudioController: Volume set to %d\n", volume);
    return true;
}

uint8_t AudioController::getVolume() const {
    return currentVolume;
}

bool AudioController::volumeUp() {
    if (currentVolume < 30) {
        return setVolume(currentVolume + 1);
    }
    return true;
}

bool AudioController::volumeDown() {
    if (currentVolume > 0) {
        return setVolume(currentVolume - 1);
    }
    return true;
}

AudioStatus AudioController::getStatus() const {
    return currentStatus;
}

uint16_t AudioController::getCurrentTrack() const {
    return currentTrack;
}

bool AudioController::isPlaying() const {
    return currentStatus == AudioStatus::PLAYING;
}

bool AudioController::isAvailable() const {
    return audioAvailable;
}

bool AudioController::addTrack(const AudioTrack& track) {
    // Check if track already exists
    AudioTrack* existing = findTrack(track.fileNumber);
    if (existing) {
        // Update existing track
        *existing = track;
        Serial.printf("AudioController: Updated track %d: '%s'\n", 
                     track.fileNumber, track.description.c_str());
    } else {
        // Add new track
        tracks.push_back(track);
        Serial.printf("AudioController: Added track %d: '%s'\n", 
                     track.fileNumber, track.description.c_str());
    }
    
    return true;
}

bool AudioController::removeTrack(uint16_t fileNumber) {
    auto it = std::remove_if(tracks.begin(), tracks.end(),
        [fileNumber](const AudioTrack& track) { 
            return track.fileNumber == fileNumber; 
        });
    
    if (it != tracks.end()) {
        tracks.erase(it, tracks.end());
        Serial.printf("AudioController: Removed track %d\n", fileNumber);
        return true;
    }
    
    return false;
}

AudioTrack* AudioController::getTrack(uint16_t fileNumber) {
    return findTrack(fileNumber);
}

AudioTrack* AudioController::getTrack(const String& trackName) {
    return findTrack(trackName);
}

std::vector<AudioTrack*> AudioController::getAllTracks() {
    std::vector<AudioTrack*> result;
    for (AudioTrack& track : tracks) {
        result.push_back(&track);
    }
    return result;
}

bool AudioController::testTrack(uint16_t fileNumber) {
    if (!audioAvailable) return false;
    
    Serial.printf("AudioController: Testing track %d\n", fileNumber);
    
    // Stop any current playback
    stop();
    delay(100);
    
    // Play track for 3 seconds
    if (play(fileNumber, false)) {
        delay(3000);
        stop();
        return true;
    }
    
    return false;
}

void AudioController::printStatus() const {
    Serial.println("=== AudioController Status ===");
    Serial.printf("Available: %s\n", audioAvailable ? "Yes" : "No");
    Serial.printf("Status: ");
    
    switch (currentStatus) {
        case AudioStatus::STOPPED: Serial.println("Stopped"); break;
        case AudioStatus::PLAYING: Serial.println("Playing"); break;
        case AudioStatus::PAUSED: Serial.println("Paused"); break;
        case AudioStatus::ERROR: Serial.println("Error"); break;
    }
    
    Serial.printf("Current Track: %d\n", currentTrack);
    Serial.printf("Volume: %d/30\n", currentVolume);
    Serial.printf("Available Tracks: %d\n", tracks.size());
    
    if (!tracks.empty()) {
        Serial.println("Track List:");
        for (const AudioTrack& track : tracks) {
            Serial.printf("  %d: %s %s\n", track.fileNumber, track.description.c_str(),
                         track.isLoop ? "(loop)" : "");
        }
    }
}

uint16_t AudioController::getAvailableTrackCount() const {
    return tracks.size();
}

void AudioController::update() {
    if (!audioAvailable) return;
    
    uint32_t currentTime = millis();
    
    // Periodically check DFPlayer status
    if (currentTime - lastStatusCheck >= STATUS_CHECK_INTERVAL) {
        checkPlayerStatus();
        lastStatusCheck = currentTime;
    }
}

// Private methods

bool AudioController::initializeHardware() {
    // Hardware initialization is done in begin()
    return audioAvailable;
}

void AudioController::checkPlayerStatus() {
    if (!audioAvailable) return;
    
    // Query DFPlayer status
    int playerState = dfPlayer.readState();
    
    switch (playerState) {
        case 513: // Playing
            if (currentStatus != AudioStatus::PLAYING) {
                currentStatus = AudioStatus::PLAYING;
            }
            break;
            
        case 512: // Stopped
            if (currentStatus == AudioStatus::PLAYING) {
                currentStatus = AudioStatus::STOPPED;
                currentTrack = 0;
            }
            break;
            
        case 514: // Paused
            currentStatus = AudioStatus::PAUSED;
            break;
            
        default:
            // Handle error or unknown states
            if (playerState == -1) {
                currentStatus = AudioStatus::ERROR;
            }
            break;
    }
}

bool AudioController::waitForReady(uint32_t timeout) {
    uint32_t startTime = millis();
    
    while (millis() - startTime < timeout) {
        if (dfPlayer.available()) {
            // Check for ready message or successful response
            uint8_t type = dfPlayer.readType();
            if (type == DFPlayerCardInserted || type == DFPlayerCardOnline) {
                return true;
            }
        }
        delay(100);
    }
    
    return false;
}

AudioTrack* AudioController::findTrack(uint16_t fileNumber) {
    for (AudioTrack& track : tracks) {
        if (track.fileNumber == fileNumber) {
            return &track;
        }
    }
    return nullptr;
}

AudioTrack* AudioController::findTrack(const String& trackName) {
    for (AudioTrack& track : tracks) {
        if (track.description == trackName) {
            return &track;
        }
    }
    return nullptr;
}

void AudioController::updateCurrentStatus() {
    checkPlayerStatus();
}

void AudioController::initializeDefaultTracks() {
    // Add default tracks based on project documentation
    addTrack(AudioTrack(1, "Tank Idle", true, 0));
    addTrack(AudioTrack(2, "Tank Idle 2", true, 0));
    addTrack(AudioTrack(3, "Machine Gun", false, 2000));
    addTrack(AudioTrack(4, "Flamethrower", false, 3000));
    addTrack(AudioTrack(5, "Taking Hits", false, 1500));
    addTrack(AudioTrack(6, "Engine Revving", false, 4000));
    addTrack(AudioTrack(7, "Explosion", false, 2500));
    addTrack(AudioTrack(8, "Rocket Launcher", false, 3000));
    addTrack(AudioTrack(9, "Kill Confirmed", false, 1000));
}

} // namespace BattleAura