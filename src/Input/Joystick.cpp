#include "Joystick.h"
#include <Arduino.h>

void Joystick::begin() {
    // Настройка пинов управления
    pinMode(hwConfig.THROTTLE_PIN, INPUT);
    pinMode(hwConfig.STEERING_PIN, INPUT);
    pinMode(hwConfig.JOYSTICK_BUTTON_PIN, INPUT_PULLUP);
    
    // Настройка дополнительных кнопок
    pinMode(hwConfig.GEAR_BUTTON_PIN, INPUT_PULLUP);
    pinMode(hwConfig.TURBO_BUTTON_PIN, INPUT_PULLUP);
    pinMode(hwConfig.MENU_BUTTON_PIN, INPUT_PULLUP);
    
    // Автокалибровка при запуске
    calibrate();
}

void Joystick::calibrate() {
    Serial.println("🔧 Калибровка управления...");
    calibrating = true;
    
    throttleCenter = steeringCenter = 0;
    throttleMin = throttleMax = steeringMin = steeringMax = 0;
    
    // Сбор калибровочных данных
    for(int i = 0; i < Config::CALIBRATION_SAMPLES; i++) {
        int throttle = analogRead(hwConfig.THROTTLE_PIN);
        int steering = analogRead(hwConfig.STEERING_PIN);
        
        throttleCenter += throttle;
        steeringCenter += steering;
        
        if (throttle < throttleMin || i == 0) throttleMin = throttle;
        if (throttle > throttleMax || i == 0) throttleMax = throttle;
        if (steering < steeringMin || i == 0) steeringMin = steering;
        if (steering > steeringMax || i == 0) steeringMax = steering;
        
        delay(10);
    }
    
    throttleCenter /= Config::CALIBRATION_SAMPLES;
    steeringCenter /= Config::CALIBRATION_SAMPLES;
    
    calibrated = true;
    calibrating = false;
    
    Serial.printf("✅ Калибровка завершена:\n");
    Serial.printf("   Газ: центр=%d, min=%d, max=%d\n", throttleCenter, throttleMin, throttleMax);
    Serial.printf("   Рулевое: центр=%d, min=%d, max=%d\n", steeringCenter, steeringMin, steeringMax);
}

void Joystick::update() {
    if (!calibrated || calibrating) return;
    
    // Чтение и фильтрация осей управления
    int rawThrottle = readFilteredAnalog(hwConfig.THROTTLE_PIN);
    int rawSteering = readFilteredAnalog(hwConfig.STEERING_PIN);
    
    // Преобразование в диапазон -512 до +512 относительно центра
    int throttle = map(rawThrottle, throttleMin, throttleMax, -512, 512);
    int steering = map(rawSteering, steeringMin, steeringMax, -512, 512);
    
    // Ограничение диапазона
    throttle = constrain(throttle, -512, 512);
    steering = constrain(steering, -512, 512);
    
    // Применение мертвой зоны и кривых
    applyDeadZone(throttle);
    applyDeadZone(steering);
    applyCurves(throttle, steering);
    
    currentData.throttle = throttle;
    currentData.steering = steering;
    
    // Чтение кнопок
    currentData.button1 = !digitalRead(hwConfig.JOYSTICK_BUTTON_PIN);
    
    // Обновление передачи и турбо-режима
    updateGear();
    updateTurbo();
    
    // Расчет CRC
    currentData.crc = calculateCRC(currentData);
}

void Joystick::updateGear() {
    bool currentButtonState = !digitalRead(hwConfig.GEAR_BUTTON_PIN);
    
    // Переключение передачи по нажатию кнопки
    if (currentButtonState && !lastGearButtonState) {
        currentGear = (currentGear == 1) ? 2 : 1;
        Serial.printf("🔄 Передача: %s\n", currentGear == 1 ? "ВПЕРЕД" : "НАЗАД");
    }
    lastGearButtonState = currentButtonState;
    
    currentData.gear = currentGear;
}

void Joystick::updateTurbo() {
    bool currentButtonState = !digitalRead(hwConfig.TURBO_BUTTON_PIN);
    
    // Включение/выключение турбо-режима
    if (currentButtonState && !lastTurboButtonState) {
        turboEnabled = !turboEnabled;
        currentData.turbo = turboEnabled ? 1 : 0;
        Serial.printf("💨 Турбо-режим: %s\n", turboEnabled ? "ВКЛ" : "ВЫКЛ");
    }
    lastTurboButtonState = currentButtonState;
    
    currentData.turbo = turboEnabled ? 1 : 0;
}

void Joystick::applyDeadZone(int& value) {
    if (abs(value) < controlConfig.deadZone) {
        value = 0;
    } else if (value > 0) {
        value = map(value, controlConfig.deadZone, 512, 0, controlConfig.maxThrottle);
    } else {
        value = map(value, -512, -controlConfig.deadZone, -controlConfig.maxThrottle, 0);
    }
}

void Joystick::applyCurves(int& throttle, int& steering) {
    // Применение кривой к газу
    throttle = exponentialTransform(throttle, controlConfig.throttleCurve);
    
    // Применение экспоненциальной кривой к рулевому
    if (controlConfig.exponentialSteering) {
        steering = exponentialTransform(steering, 1.5);
    }
}

int Joystick::exponentialTransform(int value, float curve) {
    if (value == 0) return 0;
    
    float normalized = float(value) / 512.0;
    float sign = normalized > 0 ? 1.0 : -1.0;
    float result = powf(abs(normalized), curve) * sign;
    
    return int(result * 512);
}

void Joystick::readButtons() {
    // Основная кнопка уже обработана в update()
    currentData.button2 = !digitalRead(hwConfig.MENU_BUTTON_PIN);
}

int Joystick::readFilteredAnalog(uint8_t pin) {
    // Медианный фильтр 5x для лучшего подавления шума
    int readings[5];
    for(int i = 0; i < 5; i++) {
        readings[i] = analogRead(pin);
        delay(1);
    }
    
    // Сортировка для медианы
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4 - i; j++) {
            if(readings[j] > readings[j+1]) {
                int temp = readings[j];
                readings[j] = readings[j+1];
                readings[j+1] = temp;
            }
        }
    }
    
    return readings[2]; // Медиана
}

ControlData Joystick::getData() {
    return currentData;
}

uint16_t Joystick::calculateCRC(const ControlData& data) {
    uint16_t crc = 0;
    const uint8_t* bytes = (const uint8_t*)&data;
    
    for(size_t i = 0; i < sizeof(ControlData) - sizeof(uint16_t); i++) {
        crc = (crc << 5) + crc + bytes[i]; // Простой полиномиальный CRC
    }
    return crc;
}

bool Joystick::isConnected() {
    return calibrated && 
           analogRead(hwConfig.THROTTLE_PIN) > 0 && 
           analogRead(hwConfig.STEERING_PIN) > 0;
}

void Joystick::setControlConfig(const ControlConfig& config) {
    controlConfig = config;
}

ControlConfig Joystick::getControlConfig() const {
    return controlConfig;
}

void Joystick::startCalibration() {
    calibrating = true;
    calibrated = false;
}

bool Joystick::isCalibrating() const {
    return calibrating;
}