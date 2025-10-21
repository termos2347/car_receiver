#pragma once
#include <cstdint>

// Структура данных для управления с двумя джойстиками
struct ControlData {
    int16_t xAxis1;     // Ось X первого джойстика (-512 до +512)
    int16_t yAxis1;     // Ось Y первого джойстика (-512 до +512)
    int16_t xAxis2;     // Ось X второго джойстика (-512 до +512)
    int16_t yAxis2;     // Ось Y второго джойстика (-512 до +512)
    bool button1;       // Кнопка первого джойстика
    bool button2;       // Кнопка второго джойстика
    uint8_t buttons;    // Дополнительные кнопки (битовая маска)
    uint16_t crc;       // Контрольная сумма
};

// Конфигурация пинов
struct HardwareConfig {
    // Первый джойстик
    static const uint8_t JOYSTICK1_X_PIN = 32;
    static const uint8_t JOYSTICK1_Y_PIN = 33;
    static const uint8_t JOYSTICK1_BUTTON_PIN = 4;
    
    // Второй джойстик
    static const uint8_t JOYSTICK2_X_PIN = 34;
    static const uint8_t JOYSTICK2_Y_PIN = 35;
    static const uint8_t JOYSTICK2_BUTTON_PIN = 5;
    
    // Дополнительные кнопки
    static const uint8_t BUTTON3_PIN = 18;
    static const uint8_t BUTTON4_PIN = 19;
    
    static const uint8_t LED_PIN = 2;
};