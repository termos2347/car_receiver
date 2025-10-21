#include "Joystick.h"
#include <Arduino.h>

void Joystick::begin() {
    // Настройка пинов первого джойстика
    pinMode(config.JOYSTICK1_X_PIN, INPUT);
    pinMode(config.JOYSTICK1_Y_PIN, INPUT);
    pinMode(config.JOYSTICK1_BUTTON_PIN, INPUT_PULLUP);
    
    // Настройка пинов второго джойстика
    pinMode(config.JOYSTICK2_X_PIN, INPUT);
    pinMode(config.JOYSTICK2_Y_PIN, INPUT);
    pinMode(config.JOYSTICK2_BUTTON_PIN, INPUT_PULLUP);
    
    // Настройка дополнительных кнопок
    pinMode(config.BUTTON3_PIN, INPUT_PULLUP);
    pinMode(config.BUTTON4_PIN, INPUT_PULLUP);
    
    // Автокалибровка при запуске
    calibrate();
}

void Joystick::calibrate() {
    Serial.println("🔧 Калибровка джойстиков...");
    delay(100);
    
    x1Center = y1Center = x2Center = y2Center = 0;
    
    // Усредняем несколько измерений для обоих джойстиков
    for(int i = 0; i < 10; i++) {
        x1Center += analogRead(config.JOYSTICK1_X_PIN);
        y1Center += analogRead(config.JOYSTICK1_Y_PIN);
        x2Center += analogRead(config.JOYSTICK2_X_PIN);
        y2Center += analogRead(config.JOYSTICK2_Y_PIN);
        delay(10);
    }
    
    x1Center /= 10;
    y1Center /= 10;
    x2Center /= 10;
    y2Center /= 10;
    
    calibrated = true;
    Serial.printf("✅ Центр Джойстик1: X=%d, Y=%d\n", x1Center, y1Center);
    Serial.printf("✅ Центр Джойстик2: X=%d, Y=%d\n", x2Center, y2Center);
}

void Joystick::update() {
    if (!calibrated) return;
    
    // Чтение и фильтрация осей первого джойстика
    int rawX1 = readFilteredAnalog(config.JOYSTICK1_X_PIN);
    int rawY1 = readFilteredAnalog(config.JOYSTICK1_Y_PIN);
    
    // Чтение и фильтрация осей второго джойстика
    int rawX2 = readFilteredAnalog(config.JOYSTICK2_X_PIN);
    int rawY2 = readFilteredAnalog(config.JOYSTICK2_Y_PIN);
    
    // Преобразование в диапазон -512 до +512
    currentData.xAxis1 = constrain(map(rawX1, 0, 4095, -512, 512), -512, 512);
    currentData.yAxis1 = constrain(map(rawY1, 0, 4095, -512, 512), -512, 512);
    currentData.xAxis2 = constrain(map(rawX2, 0, 4095, -512, 512), -512, 512);
    currentData.yAxis2 = constrain(map(rawY2, 0, 4095, -512, 512), -512, 512);
    
    // Чтение кнопок
    currentData.button1 = !digitalRead(config.JOYSTICK1_BUTTON_PIN);
    currentData.button2 = !digitalRead(config.JOYSTICK2_BUTTON_PIN);
    
    // Чтение дополнительных кнопок
    readButtons();
    
    // Расчет CRC
    currentData.crc = calculateCRC(currentData);
}

void Joystick::readButtons() {
    currentData.buttons = 0;
    
    // Кнопка 3 (бит 0)
    if (!digitalRead(config.BUTTON3_PIN)) {
        currentData.buttons |= 0x01;
    }
    
    // Кнопка 4 (бит 1)
    if (!digitalRead(config.BUTTON4_PIN)) {
        currentData.buttons |= 0x02;
    }
    
    // Можно добавить больше кнопок здесь
    // currentData.buttons |= 0x04; // Кнопка 5 (бит 2)
    // currentData.buttons |= 0x08; // Кнопка 6 (бит 3)
}

int Joystick::readFilteredAnalog(uint8_t pin) {
    // Простой медианный фильтр 3x
    int readings[3];
    for(int i = 0; i < 3; i++) {
        readings[i] = analogRead(pin);
        delay(2);
    }
    
    // Сортировка пузырьком для медианы
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 2 - i; j++) {
            if(readings[j] > readings[j+1]) {
                int temp = readings[j];
                readings[j] = readings[j+1];
                readings[j+1] = temp;
            }
        }
    }
    
    return readings[1]; // Медиана
}

ControlData Joystick::getData() {
    return currentData;
}

uint16_t Joystick::calculateCRC(const ControlData& data) {
    // Простая контрольная сумма
    uint16_t crc = 0;
    const uint8_t* bytes = (const uint8_t*)&data;
    
    for(size_t i = 0; i < sizeof(ControlData) - sizeof(uint16_t); i++) {
        crc += bytes[i];
    }
    return crc;
}

bool Joystick::isConnected() {
    return calibrated && 
           analogRead(config.JOYSTICK1_X_PIN) > 0 && 
           analogRead(config.JOYSTICK1_Y_PIN) > 0 &&
           analogRead(config.JOYSTICK2_X_PIN) > 0 && 
           analogRead(config.JOYSTICK2_Y_PIN) > 0;
}