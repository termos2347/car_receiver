// ПУЛЬТ УПРАВЛЕНИЯ (передатчик) - ОПТИМИЗИРОВАННАЯ ВЕРСИЯ
#include <esp_now.h>
#include <WiFi.h>
#include "Core/Types.h"
#include "Input/Joystick.h"

Joystick joystick;

// MAC адрес самолета (приемника)
const uint8_t receiverMac[] = {0xEC, 0xE3, 0x34, 0x1A, 0xB1, 0xA8};

// Глобальные переменные для оптимизации
static ControlData currentData;
static uint16_t lastCRC = 0;
static bool dataChanged = false;

// Тайминги (в мс)
enum Timing {
  JOYSTICK_READ_INTERVAL = 30,    // Чтение джойстиков
  DATA_SEND_INTERVAL = 40,        // Отправка данных
  SERIAL_PRINT_INTERVAL = 30000,  // Вывод в Serial
  LED_INDICATION_TIME = 25        // Индикация LED
};

// Переменные для неблокирующих таймеров
static unsigned long lastJoystickRead = 0;
static unsigned long lastDataSend = 0;
static unsigned long lastSerialPrint = 0;
static unsigned long ledOffTime = 0;
static bool ledState = false;

// Прототипы функций с правильными параметрами
bool addPeer(const uint8_t* macAddress);
void optimizedDeviceInfo();
void setupIndication();
void handleJoystickReading();
void handleDataSending();
void handleLEDIndication();
void handleSerialOutput();

// Функция для добавления пира в ESP-NOW
bool addPeer(const uint8_t* macAddress) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, macAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    return esp_now_add_peer(&peerInfo) == ESP_OK;
}

// Оптимизированный вывод информации об устройстве
void optimizedDeviceInfo() {
  Serial.println("🎮 Пульт управления запущен");
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Free RAM: ");
  Serial.println(ESP.getFreeHeap());
}

// Индикация готовности
void setupIndication() {
  pinMode(2, OUTPUT);
  for(int i = 0; i < 3; i++) {
    digitalWrite(2, HIGH);
    delay(80);  // Укороченные задержки
    digitalWrite(2, LOW);
    delay(80);
  }
}

void setup() {
  Serial.begin(115200);
  delay(800);  // Уменьшенная задержка
  
  Serial.println("🎮 Запуск пульта...");
  
  // Оптимизированная информация об устройстве
  optimizedDeviceInfo();
  
  // Инициализация компонентов
  joystick.begin();
  
  // Настройка WiFi и ESP-NOW
  WiFi.mode(WIFI_STA);
  
  // Оптимизация WiFi для снижения энергопотребления
  WiFi.setSleep(true);  // Разрешить сон WiFi
  WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Уменьшить мощность передачи
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ Ошибка ESP-NOW");
    return;
  }
  
  // Добавление пира с проверкой
  if (addPeer(receiverMac)) {
    Serial.println("✅ Самолет добавлен");
  } else {
    Serial.println("❌ Ошибка добавления");
    return;
  }
  
  // Индикация готовности
  setupIndication();
  
  // Инициализация начальных данных
  currentData = joystick.getData();
  lastCRC = joystick.calculateCRC(currentData);
  
  Serial.println("🚀 Пульт готов");
}

void loop() {
  const unsigned long currentMillis = millis();
  
  // Разделение задач по времени для равномерной нагрузки
  handleJoystickReading();
  handleDataSending();
  handleLEDIndication();
  handleSerialOutput();
}

// Обработка чтения джойстиков
void handleJoystickReading() {
  const unsigned long currentMillis = millis();
  if (currentMillis - lastJoystickRead >= JOYSTICK_READ_INTERVAL) {
    joystick.update();
    ControlData newData = joystick.getData();
    
    // Проверка изменения данных с deadzone для уменьшения шума
    const int DEADZONE = 3;
    if (abs(newData.xAxis1 - currentData.xAxis1) > DEADZONE ||
        abs(newData.yAxis1 - currentData.yAxis1) > DEADZONE ||
        abs(newData.xAxis2 - currentData.xAxis2) > DEADZONE ||
        abs(newData.yAxis2 - currentData.yAxis2) > DEADZONE) {
      
      currentData = newData;
      dataChanged = true;
    }
    
    lastJoystickRead = currentMillis;
  }
}

// Обработка отправки данных
void handleDataSending() {
  const unsigned long currentMillis = millis();
  if (dataChanged && (currentMillis - lastDataSend >= DATA_SEND_INTERVAL)) {
    // Проверка CRC только если данные изменились
    uint16_t currentCRC = joystick.calculateCRC(currentData);
    
    if (currentCRC == currentData.crc) {
      esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&currentData, sizeof(currentData));
      
      if (result == ESP_OK) {
        // Активация LED индикации
        digitalWrite(2, HIGH);
        ledState = true;
        ledOffTime = currentMillis + LED_INDICATION_TIME;
        lastCRC = currentCRC;
      }
      // Убрать вывод ошибок в продакшене для экономии ресурсов
    }
    
    dataChanged = false;
    lastDataSend = currentMillis;
  }
}

// Управление LED индикацией
void handleLEDIndication() {
  const unsigned long currentMillis = millis();
  if (ledState && currentMillis > ledOffTime) {
    digitalWrite(2, LOW);
    ledState = false;
  }
}

// Управление выводом в Serial
void handleSerialOutput() {
  const unsigned long currentMillis = millis();
  if (currentMillis - lastSerialPrint >= SERIAL_PRINT_INTERVAL) {
    // Минималистичный вывод
    Serial.printf("J1:%d,%d J2:%d,%d CRC:%u\n", 
                 currentData.xAxis1, currentData.yAxis1, 
                 currentData.xAxis2, currentData.yAxis2,
                 lastCRC);
    lastSerialPrint = currentMillis;
  }
}