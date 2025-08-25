#pragma once

#include <Arduino.h>
#include <cstdint>

enum class PinMode {
    PIN_DISABLED,
    OUTPUT_STANDARD,
    OUTPUT_PWM,
    OUTPUT_WS2812B,
    INPUT_DIGITAL,
    INPUT_ANALOG
};

enum class PinState {
    LOW = 0,
    HIGH = 1
};

class GPIOManager {
public:
    static GPIOManager& getInstance();
    
    // Pin configuration
    bool configurePins(uint8_t pin, PinMode mode);
    bool isPinConfigured(uint8_t pin) const;
    PinMode getPinMode(uint8_t pin) const;
    
    // Digital operations
    bool digitalWrite(uint8_t pin, PinState state);
    PinState digitalRead(uint8_t pin);
    
    // PWM operations  
    bool analogWrite(uint8_t pin, uint8_t value);
    
    // Analog input
    uint16_t analogRead(uint8_t pin);
    
    // Validation
    static bool isValidPin(uint8_t pin);
    static bool isReservedPin(uint8_t pin);
    
private:
    GPIOManager() = default;
    ~GPIOManager() = default;
    GPIOManager(const GPIOManager&) = delete;
    GPIOManager& operator=(const GPIOManager&) = delete;
    
    static constexpr uint8_t MAX_PINS = 22;
    PinMode pinModes[MAX_PINS];
    bool pinConfigured[MAX_PINS];
    
    void initializePinArrays();
};