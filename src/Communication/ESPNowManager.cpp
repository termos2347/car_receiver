#include "ESPNowManager.h"
#include <Arduino.h>

// Инициализация статического указателя на экземпляр
ESPNowManager* ESPNowManager::instance = nullptr;

void ESPNowManager::begin() {
    // Сохраняем указатель на текущий экземпляр для callback
    instance = this;
    
    WiFi.mode(WIFI_STA);
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("❌ Ошибка инициализации ESP-NOW");
        lastError = ESP_ERR_ESPNOW_NOT_INIT;
        return;
    }
    
    esp_now_register_send_cb(onDataSentStatic);
    
    // Увеличиваем мощность передачи
    esp_now_set_pmk((uint8_t*)"rc_car_pmk_key_1234");
    
    Serial.println("✅ ESP-NOW инициализирован");
    lastError = ESP_OK;
}

bool ESPNowManager::sendData(const ControlData& data) {
    if (!paired) {
        Serial.println("⚠️  Приемник не спарен");
        lastError = ESP_ERR_ESPNOW_NOT_FOUND;
        return false;
    }
    
    // Ограничение частоты отправки (50 Hz)
    if (millis() - lastSendTime < 20) {
        return false;
    }
    
    esp_err_t result = esp_now_send(receiverMac, (uint8_t*)&data, sizeof(data));
    lastSendTime = millis();
    sentCount++;
    
    if (result != ESP_OK) {
        Serial.print("❌ Ошибка отправки: ");
        Serial.println(result);
        lastError = result;
        errorCount++;
        return false;
    }
    
    lastError = ESP_OK;
    return true;
}

// Статический callback, который вызывает метод экземпляра
void ESPNowManager::onDataSentStatic(const uint8_t* mac, esp_now_send_status_t status) {
    if (instance != nullptr) {
        instance->onDataSent(mac, status);
    }
}

// Нестатический метод, который имеет доступ к членам класса
void ESPNowManager::onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        // Быстрая индикация успешной отправки
        digitalWrite(2, HIGH);
        delay(5);
        digitalWrite(2, LOW);
        successCount++;
        lastError = ESP_OK;
    } else {
        Serial.print("❌ Ошибка доставки: ");
        Serial.println(status);
        lastError = status;
    }
}

bool ESPNowManager::addPeer(const uint8_t* mac) {
    if (paired) {
        removePeer();
    }
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    esp_err_t result = esp_now_add_peer(&peerInfo);
    if (result == ESP_OK) {
        memcpy(receiverMac, mac, 6);
        paired = true;
        Serial.println("✅ Пиар успешно добавлен");
        lastError = ESP_OK;
        return true;
    } else {
        Serial.print("❌ Ошибка добавления пиара: ");
        Serial.println(result);
        lastError = result;
        return false;
    }
}

bool ESPNowManager::removePeer() {
    if (!paired) return true;
    
    esp_err_t result = esp_now_del_peer(receiverMac);
    if (result == ESP_OK) {
        paired = false;
        memset(receiverMac, 0, 6);
        Serial.println("✅ Пиар удален");
        lastError = ESP_OK;
        return true;
    }
    lastError = result;
    return false;
}

bool ESPNowManager::isConnected() {
    return paired;
}

void ESPNowManager::pairWithReceiver() {
    Serial.println("🔍 Режим спаривания...");
    // Можно реализовать сканирование и автоматическое спаривание
}

void ESPNowManager::setReceiverMAC(const uint8_t* mac) {
    memcpy(receiverMac, mac, 6);
}

float ESPNowManager::getSuccessRate() const {
    if (sentCount == 0) return 0.0f;
    return (float(successCount) / float(sentCount)) * 100.0f;
} 