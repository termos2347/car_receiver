#pragma once
#include "../Core/Config.h"

class LEDManager {
public:
    void begin();
    void update();
    
    // Состояния индикации
    void setConnectionStatus(bool connected);
    void setGear(uint8_t gear);
    void setTurbo(bool enabled);
    void setCalibration(bool calibrating);
    
    // Анимации
    void blinkError();
    void blinkSuccess();
    
private:
    HardwareConfig config;
    
    bool connected = false;
    uint8_t currentGear = 1;
    bool turboEnabled = false;
    bool calibrating = false;
    
    unsigned long lastBlinkTime = 0;
    bool blinkState = false;
    uint8_t blinkCount = 0;
    uint8_t targetBlinks = 0;
    
    void updateStatusLED();
    void updateGearLED();
    void updateTurboLED();
};