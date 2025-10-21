// ПУЛЬТ УПРАВЛЕНИЯ (передатчик) - С DEBUG РЕЖИМОМ
#include <esp_now.h>
#include <WiFi.h>
#include "Core/Types.h"
#include "Input/Joystick.h"

// === НАСТРОЙКИ DEBUG ===
#define DEBUG_MODE true  // true - отладка, false - полет (минимальный вывод)
// =======================

Joystick joystick;

// MAC адрес самолета (приемника)
const uint8_t receiverMac[] = {0xEC, 0xE3, 0x34, 0x1A, 0xB1, 0xA8};

// Глобальные переменные
static ControlData currentData;
static bool newDataReady = false;
static unsigned long lastDataTime = 0;

// Тайминги (в мс)
enum Timing {
  DATA_PROCESS_INTERVAL = 30,     // Обработка данных
  DATA_SEND_INTERVAL = 40,        // Отправка данных
  STATUS_PRINT_INTERVAL = 1000,   // Вывод статуса
  LED_INDICATION_TIME = 25        // Индикация LED
};

// Переменные для неблокирующих таймеров
static unsigned long lastDataProcess = 0;
static unsigned long lastDataSend = 0;
static unsigned long lastStatusPrint = 0;
static unsigned long ledOffTime = 0;
static bool ledState = false;
static bool connectionStatus = false;

// Прототипы функций
bool addPeer(const uint8_t* macAddress);
void printDeviceInfo();
void setupIndication();
void handleDataProcessing(unsigned long currentMillis);
void handleDataSending(unsigned long currentMillis);
void handleLEDIndication(unsigned long currentMillis);
void handleStatusOutput(unsigned long currentMillis);
void handleConnectionCheck(unsigned long currentMillis);

// Функция для добавления пира в ESP-NOW
bool addPeer(const uint8_t* macAddress) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, macAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    return esp_now_add_peer(&peerInfo) == ESP_OK;
}

// Вывод информации об устройстве
void printDeviceInfo() {
  #if DEBUG_MODE
    Serial.println("🎮 Пульт управления запущен");
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Free RAM: ");
    Serial.println(ESP.getFreeHeap());
  #endif
}

// Индикация готовности
void setupIndication() {
  pinMode(2, OUTPUT);
  for(int i = 0; i < 3; i++) {
    digitalWrite(2, HIGH);
    delay(80);
    digitalWrite(2, LOW);
    delay(80);
  }
}

void setup() {
  #if DEBUG_MODE
    Serial.begin(115200);
    delay(800);
    Serial.println("🎮 Запуск пульта...");
  #endif
  
  printDeviceInfo();
  
  // Инициализация компонентов
  joystick.begin();
  
  // Настройка WiFi и ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  
  if (esp_now_init() != ESP_OK) {
    #if DEBUG_MODE
      Serial.println("❌ Ошибка инициализации ESP-NOW");
    #endif
    return;
  }
  
  // Добавление пира
  if (addPeer(receiverMac)) {
    #if DEBUG_MODE
      Serial.println("✅ Самолет добавлен");
    #endif
    connectionStatus = true;
  } else {
    #if DEBUG_MODE
      Serial.println("❌ Ошибка добавления самолета");
    #endif
    connectionStatus = false;
  }
  
  // Индикация готовности
  setupIndication();
  
  // Инициализация начальных данных
  currentData = joystick.getData();
  currentData.crc = joystick.calculateCRC(currentData);
  
  #if DEBUG_MODE
    Serial.println("🚀 Пульт готов к работе");
  #endif
}

void loop() {
  const unsigned long currentMillis = millis();
  
  // Разделение задач по времени для равномерной нагрузки
  handleDataProcessing(currentMillis);
  handleDataSending(currentMillis);
  handleLEDIndication(currentMillis);
  handleStatusOutput(currentMillis);
  handleConnectionCheck(currentMillis);
}

// Обработка данных джойстиков
void handleDataProcessing(unsigned long currentMillis) {
  if (currentMillis - lastDataProcess >= DATA_PROCESS_INTERVAL) {
    joystick.update();
    ControlData newData = joystick.getData();
    
    // Проверка изменения данных с deadzone
    const int DEADZONE = 3;
    if (abs(newData.xAxis1 - currentData.xAxis1) > DEADZONE ||
        abs(newData.yAxis1 - currentData.yAxis1) > DEADZONE ||
        abs(newData.xAxis2 - currentData.xAxis2) > DEADZONE ||
        abs(newData.yAxis2 - currentData.yAxis2) > DEADZONE) {
      
      currentData = newData;
      currentData.crc = joystick.calculateCRC(currentData);
      newDataReady = true;
    }
    
    lastDataProcess = currentMillis;
  }
}

// Отправка данных
void handleDataSending(unsigned long currentMillis) {
  if (newDataReady && (currentMillis - lastDataSend >= DATA_SEND_INTERVAL)) {
    esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&currentData, sizeof(currentData));
    
    if (result == ESP_OK) {
      // Индикация отправки
      digitalWrite(2, HIGH);
      ledState = true;
      ledOffTime = currentMillis + LED_INDICATION_TIME;
      lastDataTime = currentMillis;
      
      // Вывод данных в одинаковом формате (только в debug режиме)
      #if DEBUG_MODE
        static unsigned long lastDataPrint = 0;
        if (currentMillis - lastDataPrint > 100) {
          Serial.printf("J1:%4d,%4d J2:%4d,%4d B1:%d B2:%d CRC:%4d\n", 
                       currentData.xAxis1, currentData.yAxis1, 
                       currentData.xAxis2, currentData.yAxis2,
                       currentData.button1, currentData.button2, currentData.crc);
          lastDataPrint = currentMillis;
        }
      #endif
    }
    
    newDataReady = false;
    lastDataSend = currentMillis;
  }
}

// Управление LED индикацией
void handleLEDIndication(unsigned long currentMillis) {
  if (ledState && currentMillis > ledOffTime) {
    digitalWrite(2, LOW);
    ledState = false;
  }
}

// Вывод статуса
void handleStatusOutput(unsigned long currentMillis) {
  #if DEBUG_MODE
    if (currentMillis - lastStatusPrint >= STATUS_PRINT_INTERVAL) {
      if (connectionStatus) {
        if (currentMillis - lastDataTime < 2000) {
          Serial.println("✅ Связь стабильная, данные отправляются");
        } else {
          Serial.println("⚠️  Самолет подключен, но данные не отправляются");
        }
      } else {
        Serial.println("❌ Самолет не зарегистрирован");
      }
      lastStatusPrint = currentMillis;
    }
  #endif
}

// Проверка соединения
void handleConnectionCheck(unsigned long currentMillis) {
  #if DEBUG_MODE
    static unsigned long lastConnectionCheck = 0;
    
    if (currentMillis - lastConnectionCheck > 10000) {
      bool currentStatus = esp_now_is_peer_exist(receiverMac);
      
      if (currentStatus != connectionStatus) {
        connectionStatus = currentStatus;
        if (connectionStatus) {
          Serial.println("🔗 Самолет подключен");
        } else {
          Serial.println("🔌 Самолет отключен");
        }
      }
      lastConnectionCheck = currentMillis;
    }
  #endif
}