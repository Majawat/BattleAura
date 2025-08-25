#include "gpio_manager.h"

GPIOManager& GPIOManager::getInstance() {
    static GPIOManager instance;
    return instance;
}

bool GPIOManager::configurePins(uint8_t pin, PinMode mode) {
    if (!isValidPin(pin) || isReservedPin(pin)) {
        return false;
    }
    
    // Initialize arrays on first use
    static bool initialized = false;
    if (!initialized) {
        initializePinArrays();
        initialized = true;
    }
    
    switch (mode) {
        case PinMode::OUTPUT_STANDARD:
        case PinMode::OUTPUT_PWM:
            pinMode(pin, OUTPUT);
            break;
        case PinMode::INPUT_DIGITAL:
            pinMode(pin, INPUT_PULLUP);
            break;
        case PinMode::INPUT_ANALOG:
            pinMode(pin, INPUT);
            break;
        case PinMode::PIN_DISABLED:
            // Don't call pinMode for disabled pins
            break;
        case PinMode::OUTPUT_WS2812B:
            // WS2812B will be handled by LED driver
            break;
    }
    
    pinModes[pin] = mode;
    pinConfigured[pin] = (mode != PinMode::PIN_DISABLED);
    return true;
}

bool GPIOManager::isPinConfigured(uint8_t pin) const {
    if (!isValidPin(pin)) return false;
    return pinConfigured[pin];
}

PinMode GPIOManager::getPinMode(uint8_t pin) const {
    if (!isValidPin(pin)) return PinMode::PIN_DISABLED;
    return pinModes[pin];
}

bool GPIOManager::digitalWrite(uint8_t pin, PinState state) {
    if (!isPinConfigured(pin)) return false;
    
    PinMode mode = getPinMode(pin);
    if (mode != PinMode::OUTPUT_STANDARD && mode != PinMode::OUTPUT_PWM) {
        return false;
    }
    
    ::digitalWrite(pin, static_cast<uint8_t>(state));
    return true;
}

PinState GPIOManager::digitalRead(uint8_t pin) {
    if (!isPinConfigured(pin)) return PinState::LOW;
    
    PinMode mode = getPinMode(pin);
    if (mode != PinMode::INPUT_DIGITAL) {
        return PinState::LOW;
    }
    
    return ::digitalRead(pin) ? PinState::HIGH : PinState::LOW;
}

bool GPIOManager::analogWrite(uint8_t pin, uint8_t value) {
    if (!isPinConfigured(pin)) return false;
    
    PinMode mode = getPinMode(pin);
    if (mode != PinMode::OUTPUT_PWM) {
        return false;
    }
    
    ::analogWrite(pin, value);
    return true;
}

uint16_t GPIOManager::analogRead(uint8_t pin) {
    if (!isPinConfigured(pin)) return 0;
    
    PinMode mode = getPinMode(pin);
    if (mode != PinMode::INPUT_ANALOG) {
        return 0;
    }
    
    return ::analogRead(pin);
}

bool GPIOManager::isValidPin(uint8_t pin) {
    // ESP32-C3 valid GPIO pins
    return pin < MAX_PINS && 
           (pin <= 10 || pin == 18 || pin == 19 || pin == 20 || pin == 21);
}

bool GPIOManager::isReservedPin(uint8_t pin) {
    // Reserved for boot/flash/USB on ESP32-C3
    return (pin == 11 || pin == 12 || pin == 13 || pin == 14 || pin == 15 || 
            pin == 16 || pin == 17);
}

void GPIOManager::initializePinArrays() {
    for (uint8_t i = 0; i < MAX_PINS; i++) {
        pinModes[i] = PinMode::PIN_DISABLED;
        pinConfigured[i] = false;
    }
}