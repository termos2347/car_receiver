#pragma once
#include "../Core/Types.h"

class Joystick {
public:
    void begin();
    void update();
    ControlData getData();
    void calibrate();
    bool isConnected();
    
    // Сделать метод публичным для доступа из main.cpp
    uint16_t calculateCRC(const ControlData& data);
    
private:
    HardwareConfig config;
    ControlData currentData;
    
    // Центральные положения для двух джойстиков
    int x1Center = 2048;
    int y1Center = 2048;
    int x2Center = 2048;
    int y2Center = 2048;
    
    bool calibrated = false;
    
    int readFilteredAnalog(uint8_t pin);
    void readButtons();
};