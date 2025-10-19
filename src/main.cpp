#include "Core/Types.h"
#include "Core/Config.h"
#include "Input/Joystick.h"
#include "Communication/ESPNowManager.h"
#include "UI/LEDManager.h"

Joystick joystick;
ESPNowManager espNow;
LEDManager ledManager;

// MAC адрес приемника (машинки) - используем конфигурацию
const uint8_t* receiverMac = Config::DEFAULT_RECEIVER_MAC;

void printDeviceInfo() {
  Serial.println("🏎️ ===== RC ПУЛЬТ УПРАВЛЕНИЯ =====");
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
  Serial.print("SDK версия: ");
  Serial.println(ESP.getSdkVersion());
  Serial.println("=================================");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("🏎️ Инициализация RC пульта управления...");
  
  // Вывод информации об устройстве
  printDeviceInfo();
  
  // Инициализация компонентов
  joystick.begin();
  espNow.begin();
  ledManager.begin();
  
  // Добавляем приемник как пиар
  Serial.println("⏳ Поиск машинки...");
  
  if (espNow.addPeer(receiverMac)) {
    Serial.print("✅ Машинка найдена: ");
    for(int i = 0; i < 6; i++) {
      Serial.print(receiverMac[i], HEX);
      if(i < 5) Serial.print(":");
    }
    Serial.println();
    ledManager.setConnectionStatus(true);
    ledManager.blinkSuccess();
  } else {
    Serial.println("❌ Не удалось найти машинку");
    Serial.println("⚠️  Проверьте MAC-адрес и перезагрузите пульт");
    ledManager.blinkError();
  }
  
  Serial.println("🚀 Пульт готов к управлению");
  if (espNow.isConnected()) {
    Serial.println("📡 Связь с машинкой установлена");
  } else {
    Serial.println("⚠️  Связь с машинкой НЕ установлена");
  }
  Serial.println("🎮 Ожидание команд управления...");
  Serial.println("📋 Управление:");
  Serial.println("   Джойстик - газ и рулевое");
  Serial.println("   Кнопка A - ручной тормоз");
  Serial.println("   Кнопка B - переключение передачи");
  Serial.println("   Кнопка C - турбо-режим");
}

void loop() {
  // Обновление состояния джойстика
  joystick.update();
  ControlData data = joystick.getData();
  
  // Обновление индикации
  ledManager.setGear(data.gear);
  ledManager.setTurbo(data.turbo);
  ledManager.update();
  
  // Проверка CRC перед отправкой
  static uint16_t lastCRC = 0;
  uint16_t currentCRC = joystick.calculateCRC(data);
  
  if (currentCRC == data.crc && currentCRC != lastCRC) {
    if (espNow.sendData(data)) {
      // Успешная отправка
      ledManager.setConnectionStatus(true);
    } else {
      ledManager.setConnectionStatus(false);
    }
    lastCRC = currentCRC;
  }
  
  // Вывод отладочной информации каждые 500 мс
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    const char* gearText = data.gear == 1 ? "ВПЕРЕД" : "НАЗАД";
    const char* turboText = data.turbo ? "ТУРБО" : "НОРМА";
    
    Serial.printf("🎮 Газ: %-4d Рулевое: %-4d\n", data.throttle, data.steering);
    Serial.printf("🔄 %s | %s | %s %s\n", gearText, turboText,
                data.button1 ? "[ТОРМОЗ]" : "        ",
                data.button2 ? "[ФУНКЦИЯ]" : "         ");
    Serial.printf("📊 Пакеты: %d Успех: %.1f%%\n", 
                 espNow.getSentCount(), espNow.getSuccessRate());
    Serial.println("---");
    lastPrint = millis();
  }
  
  // Проверка соединения
  static unsigned long lastConnectionCheck = 0;
  if (millis() - lastConnectionCheck > 2000) {
    if (!espNow.isConnected()) {
      Serial.println("⚠️  Потеряно соединение с машинкой");
      ledManager.setConnectionStatus(false);
    }
    lastConnectionCheck = millis();
  }
  
  delay(10); // Основная задержка цикла
}