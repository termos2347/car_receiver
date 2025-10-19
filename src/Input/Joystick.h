#pragma once
#include "../Core/Types.h"
#include "../Core/Config.h"

class Joystick {
public:
    void begin();
    void update();
    ControlData getData();
    void calibrate();
    bool isConnected();
    
    // Управление настройками
    void setControlConfig(const ControlConfig& config);
    ControlConfig getControlConfig() const;
    
    // Калибровка
    void startCalibration();
    bool isCalibrating() const;
    
    // CRC расчет
    uint16_t calculateCRC(const ControlData& data);
    
private:
    HardwareConfig hwConfig;
    ControlConfig controlConfig;
    ControlData currentData;
    
    // Калибровочные значения
    int throttleCenter = 2048;
    int steeringCenter = 2048;
    int throttleMin = 0, throttleMax = 4095;
    int steeringMin = 0, steeringMax = 4095;
    
    bool calibrated = false;
    bool calibrating = false;
    uint8_t currentGear = 1; // 1-вперед, 2-назад
    bool turboEnabled = false;
    
    // Состояния кнопок для обработки нажатий
    bool lastGearButtonState = false;
    bool lastTurboButtonState = false;
    
    int readFilteredAnalog(uint8_t pin);
    void readButtons();
    void updateGear();
    void updateTurbo();
    void applyCurves(int& throttle, int& steering);
    void applyDeadZone(int& value);
    int exponentialTransform(int value, float curve);
};