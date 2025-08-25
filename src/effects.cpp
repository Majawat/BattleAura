#include "effects.h"

EffectManager& EffectManager::getInstance() {
  static EffectManager instance;
  return instance;
}

void EffectManager::begin() {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  for (int i = 0; i < MAX_PINS; i++) {
    effects[i] = {0};
    neoPixels[i] = nullptr;
    
    if (config.pins[i].enabled) {
      initPin(i, config.pins[i]);
    }
  }
}

void EffectManager::initPin(uint8_t index, const PinConfig& pinConfig) {
  effects[index].brightness = pinConfig.brightness;
  effects[index].color = pinConfig.color;
  effects[index].active = false;
  effects[index].audioTriggered = false;
  
  if (pinConfig.type == PinType::STANDARD_LED) {
    pinMode(pinConfig.pin, OUTPUT);
    digitalWrite(pinConfig.pin, LOW);
  } else if (pinConfig.type == PinType::WS2812B) {
    if (neoPixels[index]) {
      delete neoPixels[index];
    }
    neoPixels[index] = new Adafruit_NeoPixel(1, pinConfig.pin, NEO_GRB + NEO_KHZ800);
    neoPixels[index]->begin();
    neoPixels[index]->setBrightness(pinConfig.brightness);
    neoPixels[index]->clear();
    neoPixels[index]->show();
  }
  
  if (pinConfig.effect != EffectType::NONE) {
    triggerEffect(pinConfig.pin, pinConfig.effect, 0);
  }
}

void EffectManager::update() {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  for (int i = 0; i < MAX_PINS; i++) {
    if (!config.pins[i].enabled || !effects[i].active) continue;
    
    uint32_t currentTime = millis();
    if (effects[i].duration > 0 && (currentTime - effects[i].startTime) > effects[i].duration) {
      stopEffect(config.pins[i].pin);
      continue;
    }
    
    switch (effects[i].type) {
      case EffectType::CANDLE_FLICKER:
        updateCandleFlicker(i);
        break;
      case EffectType::ENGINE_PULSE:
        updateEnginePulse(i);
        break;
      case EffectType::MACHINE_GUN:
        updateMachineGun(i);
        break;
      case EffectType::FLAMETHROWER:
        updateFlamethrower(i);
        break;
      case EffectType::ROCKET_LAUNCHER:
        updateRocketLauncher(i);
        break;
      case EffectType::TAKING_DAMAGE:
        updateTakingDamage(i);
        break;
      case EffectType::EXPLOSION:
        updateExplosion(i);
        break;
      case EffectType::CONSOLE_RGB:
        updateConsoleRGB(i);
        break;
      case EffectType::STATIC_ON:
        updateStaticOn(i);
        break;
      case EffectType::STATIC_OFF:
        updateStaticOff(i);
        break;
      default:
        break;
    }
  }
}

void EffectManager::triggerEffect(uint8_t pin, EffectType effect, uint32_t duration) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  for (int i = 0; i < MAX_PINS; i++) {
    if (config.pins[i].pin == pin && config.pins[i].enabled) {
      effects[i].active = true;
      effects[i].type = effect;
      effects[i].startTime = millis();
      effects[i].duration = duration;
      effects[i].lastUpdate = 0;
      effects[i].stepCount = 0;
      effects[i].audioTriggered = false;
      
      switch (effect) {
        case EffectType::CANDLE_FLICKER:
          effects[i].params.candle.intensity = 128;
          effects[i].params.candle.nextChange = millis() + random(50, 200);
          break;
        case EffectType::ENGINE_PULSE:
          effects[i].params.pulse.phase = 0;
          effects[i].params.pulse.direction = true;
          break;
        case EffectType::MACHINE_GUN:
          effects[i].params.weapon.burstCount = 0;
          effects[i].params.weapon.burstDelay = 0;
          effects[i].params.weapon.burstActive = false;
          break;
        case EffectType::CONSOLE_RGB:
          effects[i].params.rgb.red = (effects[i].color >> 16) & 0xFF;
          effects[i].params.rgb.green = (effects[i].color >> 8) & 0xFF;
          effects[i].params.rgb.blue = effects[i].color & 0xFF;
          effects[i].params.rgb.mode = 0;
          break;
        default:
          break;
      }
      break;
    }
  }
}

void EffectManager::stopEffect(uint8_t pin) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  for (int i = 0; i < MAX_PINS; i++) {
    if (config.pins[i].pin == pin) {
      effects[i].active = false;
      if (config.pins[i].type == PinType::STANDARD_LED) {
        setStandardLED(pin, 0);
      } else if (config.pins[i].type == PinType::WS2812B && neoPixels[i]) {
        neoPixels[i]->clear();
        neoPixels[i]->show();
      }
      break;
    }
  }
}

void EffectManager::stopAllEffects() {
  for (int i = 0; i < MAX_PINS; i++) {
    effects[i].active = false;
  }
}

void EffectManager::updateCandleFlicker(uint8_t index) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  uint32_t currentTime = millis();
  
  if (currentTime >= effects[index].params.candle.nextChange) {
    uint8_t baseIntensity = 180;
    uint8_t variation = random(50, 100);
    effects[index].params.candle.intensity = constrain(baseIntensity + variation, 50, 255);
    effects[index].params.candle.nextChange = currentTime + random(30, 150);
  }
  
  uint8_t scaledIntensity = map(effects[index].params.candle.intensity, 0, 255, 0, effects[index].brightness);
  
  if (config.pins[index].type == PinType::STANDARD_LED) {
    setStandardLED(config.pins[index].pin, scaledIntensity);
  } else if (config.pins[index].type == PinType::WS2812B) {
    uint32_t flickerColor = scaleColor(effects[index].color, scaledIntensity);
    setNeoPixel(index, flickerColor);
  }
}

void EffectManager::updateEnginePulse(uint8_t index) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  uint32_t currentTime = millis();
  if (currentTime - effects[index].lastUpdate < 20) return;
  effects[index].lastUpdate = currentTime;
  
  if (effects[index].params.pulse.direction) {
    effects[index].params.pulse.phase += 5;
    if (effects[index].params.pulse.phase >= 255) {
      effects[index].params.pulse.direction = false;
    }
  } else {
    effects[index].params.pulse.phase -= 5;
    if (effects[index].params.pulse.phase <= 50) {
      effects[index].params.pulse.direction = true;
    }
  }
  
  uint8_t scaledIntensity = map(effects[index].params.pulse.phase, 0, 255, 0, effects[index].brightness);
  
  if (config.pins[index].type == PinType::STANDARD_LED) {
    setStandardLED(config.pins[index].pin, scaledIntensity);
  } else if (config.pins[index].type == PinType::WS2812B) {
    uint32_t pulseColor = scaleColor(effects[index].color, scaledIntensity);
    setNeoPixel(index, pulseColor);
  }
}

void EffectManager::updateMachineGun(uint8_t index) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  uint32_t currentTime = millis();
  
  if (!effects[index].params.weapon.burstActive) {
    if (currentTime - effects[index].lastUpdate > 80) {
      effects[index].params.weapon.burstActive = true;
      effects[index].params.weapon.burstCount = random(3, 8);
      effects[index].params.weapon.burstDelay = currentTime + random(20, 40);
      effects[index].lastUpdate = currentTime;
    }
  } else {
    if (currentTime >= effects[index].params.weapon.burstDelay) {
      bool flash = (effects[index].stepCount % 2 == 0);
      uint8_t intensity = flash ? effects[index].brightness : 0;
      
      if (config.pins[index].type == PinType::STANDARD_LED) {
        setStandardLED(config.pins[index].pin, intensity);
      } else if (config.pins[index].type == PinType::WS2812B) {
        uint32_t flashColor = flash ? effects[index].color : 0x000000;
        setNeoPixel(index, scaleColor(flashColor, effects[index].brightness));
      }
      
      effects[index].stepCount++;
      effects[index].params.weapon.burstDelay = currentTime + 30;
      
      if (effects[index].stepCount >= effects[index].params.weapon.burstCount * 2) {
        effects[index].params.weapon.burstActive = false;
        effects[index].stepCount = 0;
        effects[index].lastUpdate = currentTime;
      }
    }
  }
}

void EffectManager::updateFlamethrower(uint8_t index) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  uint32_t currentTime = millis();
  if (currentTime - effects[index].lastUpdate < 25) return;
  effects[index].lastUpdate = currentTime;
  
  uint8_t flameIntensity = random(effects[index].brightness * 0.7, effects[index].brightness);
  uint32_t flameColor = 0xFF0000 | (random(0, 100) << 8);
  
  if (config.pins[index].type == PinType::STANDARD_LED) {
    setStandardLED(config.pins[index].pin, flameIntensity);
  } else if (config.pins[index].type == PinType::WS2812B) {
    setNeoPixel(index, scaleColor(flameColor, flameIntensity));
  }
}

void EffectManager::updateRocketLauncher(uint8_t index) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  uint32_t currentTime = millis();
  uint32_t elapsed = currentTime - effects[index].startTime;
  
  if (elapsed < 500) {
    uint8_t intensity = map(elapsed, 0, 500, 0, effects[index].brightness);
    if (config.pins[index].type == PinType::STANDARD_LED) {
      setStandardLED(config.pins[index].pin, intensity);
    } else if (config.pins[index].type == PinType::WS2812B) {
      setNeoPixel(index, scaleColor(0xFFFF00, intensity));
    }
  } else if (elapsed < 1000) {
    if (config.pins[index].type == PinType::STANDARD_LED) {
      setStandardLED(config.pins[index].pin, effects[index].brightness);
    } else if (config.pins[index].type == PinType::WS2812B) {
      setNeoPixel(index, scaleColor(0xFFFFFF, effects[index].brightness));
    }
  } else {
    uint8_t intensity = map(elapsed, 1000, 1500, effects[index].brightness, 0);
    if (config.pins[index].type == PinType::STANDARD_LED) {
      setStandardLED(config.pins[index].pin, intensity);
    } else if (config.pins[index].type == PinType::WS2812B) {
      setNeoPixel(index, scaleColor(0xFF6600, intensity));
    }
  }
}

void EffectManager::updateTakingDamage(uint8_t index) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  uint32_t currentTime = millis();
  bool flash = ((currentTime - effects[index].startTime) / 100) % 2 == 0;
  
  if (flash) {
    if (config.pins[index].type == PinType::STANDARD_LED) {
      setStandardLED(config.pins[index].pin, effects[index].brightness);
    } else if (config.pins[index].type == PinType::WS2812B) {
      setNeoPixel(index, scaleColor(0xFF0000, effects[index].brightness));
    }
  } else {
    if (config.pins[index].type == PinType::STANDARD_LED) {
      setStandardLED(config.pins[index].pin, 0);
    } else if (config.pins[index].type == PinType::WS2812B) {
      setNeoPixel(index, 0);
    }
  }
}

void EffectManager::updateExplosion(uint8_t index) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  uint32_t currentTime = millis();
  uint32_t elapsed = currentTime - effects[index].startTime;
  
  if (elapsed < 200) {
    if (config.pins[index].type == PinType::STANDARD_LED) {
      setStandardLED(config.pins[index].pin, effects[index].brightness);
    } else if (config.pins[index].type == PinType::WS2812B) {
      setNeoPixel(index, scaleColor(0xFFFFFF, effects[index].brightness));
    }
  } else if (elapsed < 800) {
    uint8_t intensity = map(elapsed, 200, 800, effects[index].brightness, effects[index].brightness * 0.3);
    uint32_t explosionColor = blend(0xFFFFFF, 0xFF6600, map(elapsed, 200, 800, 0, 255));
    
    if (config.pins[index].type == PinType::STANDARD_LED) {
      setStandardLED(config.pins[index].pin, intensity);
    } else if (config.pins[index].type == PinType::WS2812B) {
      setNeoPixel(index, scaleColor(explosionColor, intensity));
    }
  } else {
    uint8_t intensity = map(elapsed, 800, 2000, effects[index].brightness * 0.3, 0);
    if (config.pins[index].type == PinType::STANDARD_LED) {
      setStandardLED(config.pins[index].pin, intensity);
    } else if (config.pins[index].type == PinType::WS2812B) {
      setNeoPixel(index, scaleColor(0x660000, intensity));
    }
  }
}

void EffectManager::updateConsoleRGB(uint8_t index) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  if (config.pins[index].type == PinType::WS2812B) {
    setNeoPixel(index, scaleColor(effects[index].color, effects[index].brightness));
  }
}

void EffectManager::updateStaticOn(uint8_t index) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  if (config.pins[index].type == PinType::STANDARD_LED) {
    setStandardLED(config.pins[index].pin, effects[index].brightness);
  } else if (config.pins[index].type == PinType::WS2812B) {
    setNeoPixel(index, scaleColor(effects[index].color, effects[index].brightness));
  }
}

void EffectManager::updateStaticOff(uint8_t index) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  if (config.pins[index].type == PinType::STANDARD_LED) {
    setStandardLED(config.pins[index].pin, 0);
  } else if (config.pins[index].type == PinType::WS2812B) {
    setNeoPixel(index, 0);
  }
}

void EffectManager::setStandardLED(uint8_t pin, uint8_t value) {
  analogWrite(pin, value);
}

void EffectManager::setNeoPixel(uint8_t index, uint32_t color) {
  if (neoPixels[index]) {
    neoPixels[index]->setPixelColor(0, color);
    neoPixels[index]->show();
  }
}

uint32_t EffectManager::scaleColor(uint32_t color, uint8_t brightness) {
  uint8_t r = ((color >> 16) & 0xFF) * brightness / 255;
  uint8_t g = ((color >> 8) & 0xFF) * brightness / 255;
  uint8_t b = (color & 0xFF) * brightness / 255;
  return (r << 16) | (g << 8) | b;
}

uint32_t EffectManager::blend(uint32_t color1, uint32_t color2, uint8_t amount) {
  uint8_t r1 = (color1 >> 16) & 0xFF;
  uint8_t g1 = (color1 >> 8) & 0xFF;
  uint8_t b1 = color1 & 0xFF;
  
  uint8_t r2 = (color2 >> 16) & 0xFF;
  uint8_t g2 = (color2 >> 8) & 0xFF;
  uint8_t b2 = color2 & 0xFF;
  
  uint8_t r = r1 + ((r2 - r1) * amount / 255);
  uint8_t g = g1 + ((g2 - g1) * amount / 255);
  uint8_t b = b1 + ((b2 - b1) * amount / 255);
  
  return (r << 16) | (g << 8) | b;
}

void EffectManager::setBrightness(uint8_t pin, uint8_t brightness) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  for (int i = 0; i < MAX_PINS; i++) {
    if (config.pins[i].pin == pin) {
      effects[i].brightness = brightness;
      if (neoPixels[i]) {
        neoPixels[i]->setBrightness(brightness);
      }
      break;
    }
  }
}

void EffectManager::setColor(uint8_t pin, uint32_t color) {
  ConfigManager& configMgr = ConfigManager::getInstance();
  SystemConfig& config = configMgr.getConfig();
  
  for (int i = 0; i < MAX_PINS; i++) {
    if (config.pins[i].pin == pin) {
      effects[i].color = color;
      break;
    }
  }
}