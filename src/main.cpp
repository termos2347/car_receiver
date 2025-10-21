// ПУЛЬТ УПРАВЛЕНИЯ (передатчик)
#include <esp_now.h>
#include <WiFi.h>
#include "Core/Types.h"
#include "Input/Joystick.h"

Joystick joystick;

// MAC адрес самолета (приемника)
uint8_t receiverMac[] = {0xEC, 0xE3, 0x34, 0x1A, 0xB1, 0xA8};

// Функция для добавления пира в ESP-NOW
bool addPeer(const uint8_t* macAddress) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, macAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    esp_err_t result = esp_now_add_peer(&peerInfo);
    if (result == ESP_OK) {
        Serial.println("✅ Пир успешно добавлен");
        return true;
    } else {
        Serial.printf("❌ Ошибка добавления пира: %d\n", result);
        return false;
    }
}

void printDeviceInfo() {
  Serial.println("🎮 ===== ИНФОРМАЦИЯ ПУЛЬТА =====");
  Serial.print("MAC адрес: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Chip ID: 0x");
  Serial.println(ESP.getEfuseMac(), HEX);
  Serial.print("Частота CPU: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");
  Serial.print("Flash размер: ");
  Serial.print(ESP.getFlashChipSize() / (1024 * 1024));
  Serial.println(" MB");
  Serial.print("Свободная память: ");
  Serial.print(ESP.getFreeHeap() / 1024);
  Serial.println(" KB");
  Serial.println("================================");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("🎮 Запуск пульта управления...");
  
  // Вывод информации об устройстве
  printDeviceInfo();
  
  // Инициализация компонентов
  Serial.println("🔧 Инициализация компонентов...");
  joystick.begin();
  
  // Инициализация ESP-NOW в режиме передатчика
  Serial.println("📡 Инициализация ESP-NOW...");
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ Ошибка инициализации ESP-NOW");
    return;
  }
  
  // Добавляем самолет как пир
  Serial.println("⏳ Добавление самолета...");
  
  if (addPeer(receiverMac)) {
    Serial.print("✅ Самолет добавлен: ");
    for(int i = 0; i < 6; i++) {
      Serial.print(receiverMac[i], HEX);
      if(i < 5) Serial.print(":");
    }
    Serial.println();
  } else {
    Serial.println("❌ Не удалось добавить самолет");
    return;
  }
  
  // Индикация готовности
  pinMode(2, OUTPUT);
  for(int i = 0; i < 3; i++) {
    digitalWrite(2, HIGH);
    delay(100);
    digitalWrite(2, LOW);
    delay(100);
  }
  
  Serial.println("🚀 Пульт готов к работе");
  Serial.println("📡 Ожидание данных джойстиков...");
}

void loop() {
  // Обновляем данные джойстиков
  joystick.update();
  ControlData data = joystick.getData();
  
  // Проверка CRC перед отправкой
  static uint16_t lastCRC = 0;
  uint16_t currentCRC = joystick.calculateCRC(data);
  
  // Отправляем данные, если они изменились
  if (currentCRC == data.crc && currentCRC != lastCRC) {
    esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&data, sizeof(data));
    if (result == ESP_OK) {
      // Быстрая индикация успешной отправки
      digitalWrite(2, HIGH);
      delay(5);
      digitalWrite(2, LOW);
    } else {
      Serial.printf("⚠️  Ошибка отправки данных: %d\n", result);
    }
    lastCRC = currentCRC;
  }
  
  // Вывод отладочной информации каждые 500 мс
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.printf("🎮 Джойстик1: X=%-4d Y=%-4d %s\n", 
                data.xAxis1, data.yAxis1, 
                data.button1 ? "[BTN1]" : "      ");
    Serial.printf("🎮 Джойстик2: X=%-4d Y=%-4d %s\n", 
                data.xAxis2, data.yAxis2,
                data.button2 ? "[BTN2]" : "      ");
    Serial.printf("🔄 Доп.кнопки: 0x%02X CRC: %04X\n", 
                data.buttons, data.crc);
    
    // Статус соединения (проверяем наличие пира)
    if (esp_now_is_peer_exist(receiverMac)) {
      Serial.println("📡 Связь: ОК");
    } else {
      Serial.println("❌ Связь: НЕТ");
    }
    Serial.println("---");
    lastPrint = millis();
  }
  
  // Медленное мигание в режиме работы
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    digitalWrite(2, !digitalRead(2));
    lastBlink = millis();
  }
  
  delay(10);
}