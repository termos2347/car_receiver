#include "LEDManager.h"
#include <Arduino.h>

void LEDManager::begin() {
    pinMode(config.STATUS_LED_PIN, OUTPUT);
    pinMode(config.GEAR_LED_PIN, OUTPUT);
    pinMode(config.TURBO_LED_PIN, OUTPUT);
    
    // Начальное состояние
    digitalWrite(config.STATUS_LED_PIN, LOW);
    digitalWrite(config.GEAR_LED_PIN, LOW);
    digitalWrite(config.TURBO_LED_PIN, LOW);
}

void LEDManager::update() {
    updateStatusLED();
    updateGearLED();
    updateTurboLED();
}

void LEDManager::setConnectionStatus(bool connected) {
    this->connected = connected;
}

void LEDManager::setGear(uint8_t gear) {
    this->currentGear = gear;
}

void LEDManager::setTurbo(bool enabled) {
    this->turboEnabled = enabled;
}

void LEDManager::setCalibration(bool calibrating) {
    this->calibrating = calibrating;
}

void LEDManager::updateStatusLED() {
    unsigned long currentTime = millis();
    
    if (calibrating) {
        // Быстрое мигание при калибровке
        if (currentTime - lastBlinkTime > 100) {
            digitalWrite(config.STATUS_LED_PIN, blinkState);
            blinkState = !blinkState;
            lastBlinkTime = currentTime;
        }
    } else if (connected) {
        // Медленное мигание при соединении
        if (currentTime - lastBlinkTime > 1000) {
            digitalWrite(config.STATUS_LED_PIN, blinkState);
            blinkState = !blinkState;
            lastBlinkTime = currentTime;
        }
    } else {
        // Короткие вспышки при отсутствии соединения
        if (currentTime - lastBlinkTime > 2000) {
            digitalWrite(config.STATUS_LED_PIN, HIGH);
            delay(50);
            digitalWrite(config.STATUS_LED_PIN, LOW);
            lastBlinkTime = currentTime;
        }
    }
}

void LEDManager::updateGearLED() {
    // Зеленый для вперед, красный для назад
    if (currentGear == 1) {
        digitalWrite(config.GEAR_LED_PIN, HIGH); // Вперед - горит
    } else {
        // Мигание для задней передачи
        unsigned long currentTime = millis();
        if (currentTime - lastBlinkTime > 500) {
            digitalWrite(config.GEAR_LED_PIN, blinkState);
            blinkState = !blinkState;
            lastBlinkTime = currentTime;
        }
    }
}

void LEDManager::updateTurboLED() {
    digitalWrite(config.TURBO_LED_PIN, turboEnabled ? HIGH : LOW);
}

void LEDManager::blinkError() {
    for(int i = 0; i < 3; i++) {
        digitalWrite(config.STATUS_LED_PIN, HIGH);
        delay(100);
        digitalWrite(config.STATUS_LED_PIN, LOW);
        delay(100);
    }
}

void LEDManager::blinkSuccess() {
    for(int i = 0; i < 2; i++) {
        digitalWrite(config.STATUS_LED_PIN, HIGH);
        delay(300);
        digitalWrite(config.STATUS_LED_PIN, LOW);
        delay(300);
    }
}