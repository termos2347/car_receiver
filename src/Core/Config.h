#pragma once
#include "Types.h"

// Конфигурация приложения
namespace Config {
    // Базовые настройки
    constexpr const char* DEVICE_NAME = "RC_Transmitter";
    constexpr uint32_t TRANSMIT_RATE = 50;      // Hz
    constexpr uint32_t DISPLAY_UPDATE_RATE = 10; // Hz
    
    // Настройки связи
    constexpr uint32_t CONNECTION_TIMEOUT = 2000; // мс
    constexpr uint8_t MAX_RETRIES = 3;
    
    // Настройки калибровки
    constexpr uint16_t CALIBRATION_SAMPLES = 100;
    constexpr uint16_t ADC_MAX = 4095;
    
    // MAC адрес приемника по умолчанию
    extern const uint8_t DEFAULT_RECEIVER_MAC[6];
    
    inline HardwareConfig getHardwareConfig() {
        return HardwareConfig();
    }
    
    inline ControlConfig getDefaultControlConfig() {
        return ControlConfig();
    }
}