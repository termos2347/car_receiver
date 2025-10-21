// ПУЛЬТ УПРАВЛЕНИЯ (передатчик)
#include <esp_now.h>
#include <WiFi.h>
#include "Core/Types.h"
#include "Input/Joystick.h"

Joystick joystick;

// MAC адрес самолета (приемника)
uint8_t receiverMac[] = {0xEC, 0xE3, 0x34, 0x1A, 0xB1, 0xA8};

// Переменные для неблокирующих таймеров
unsigned long lastJoystickRead = 0;
unsigned long lastDataSend = 0;
unsigned long lastSerialPrint = 0;
unsigned long ledOffTime = 0;
bool ledState = false;

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
  unsigned long currentMillis = millis();
  
  // Оптимизация обработки джойстиков - читаем каждые 50 мс вместо каждого цикла
  if (currentMillis - lastJoystickRead >= 50) {
    joystick.update();
    lastJoystickRead = currentMillis;
  }
  
  ControlData data = joystick.getData();
  
  // Проверка CRC перед отправкой
  static uint16_t lastCRC = 0;
  uint16_t currentCRC = joystick.calculateCRC(data);
  
  // Отправляем данные, если они изменились и прошло достаточно времени
  if (currentCRC == data.crc && currentCRC != lastCRC && 
      currentMillis - lastDataSend >= 80) {
    
    esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&data, sizeof(data));
    
    // Неблокирующая индикация LED
    if (result == ESP_OK) {
      digitalWrite(2, HIGH);
      ledState = true;
      ledOffTime = currentMillis + 50; // Короткая индикация 50 мс
    } else {
      Serial.printf("⚠️  Ошибка отправки: %d\n", result);
    }
    
    lastCRC = currentCRC;
    lastDataSend = currentMillis;
  }
  
  // Управление LED (неблокирующее)
  if (ledState && currentMillis > ledOffTime) {
    digitalWrite(2, LOW);
    ledState = false;
  }
  
  // Оптимизация серийного вывода - выводим каждые 30 секунд
  if (currentMillis - lastSerialPrint >= 30000) {
    Serial.printf("J1:%d,%d J2:%d,%d\n", 
                data.xAxis1, data.yAxis1, 
                data.xAxis2, data.yAxis2);
    lastSerialPrint = currentMillis;
  }
}