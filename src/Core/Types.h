#pragma once
#include <cstdint>

// Структура данных для управления машинкой
struct ControlData {
    int16_t throttle;   // Газ/тормоз (-512 до +512)
    int16_t steering;   // Рулевое управление (-512 до +512)
    bool button1;       // Основная кнопка (ручной тормоз)
    bool button2;       // Дополнительная кнопка (функция)
    uint8_t gear;       // Передача (1-вперед, 2-назад)
    uint8_t turbo;      // Турбо-режим (0-выкл, 1-вкл)
    uint16_t crc;       // Контрольная сумма
};

// Конфигурация пинов для пульта
struct HardwareConfig {
    // Основной джойстик
    static const uint8_t THROTTLE_PIN = 32;    // Ось Y - газ/тормоз
    static const uint8_t STEERING_PIN = 33;    // Ось X - рулевое
    
    // Кнопки на джойстике
    static const uint8_t JOYSTICK_BUTTON_PIN = 4;
    
    // Дополнительные кнопки
    static const uint8_t GEAR_BUTTON_PIN = 18;     // Переключение передачи
    static const uint8_t TURBO_BUTTON_PIN = 19;    // Турбо-режим
    static const uint8_t MENU_BUTTON_PIN = 21;     // Меню/настройки
    
    // Светодиоды
    static const uint8_t STATUS_LED_PIN = 2;
    static const uint8_t GEAR_LED_PIN = 15;        // Индикатор передачи
    static const uint8_t TURBO_LED_PIN = 16;       // Индикатор турбо
    
    // Потенциометры настройки (опционально)
    static const uint8_t SENSITIVITY_PIN = 34;     // Чувствительность
};

// Настройки управления
struct ControlConfig {
    int deadZone = 50;          // Мертвая зона джойстика
    int maxThrottle = 512;      // Максимальный газ
    int maxSteering = 512;      // Максимальный поворот
    float throttleCurve = 1.0;  // Кривая газа
    bool exponentialSteering = true; // Экспоненциальное рулевое
};