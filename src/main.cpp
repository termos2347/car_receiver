// ПУЛЬТ УПРАВЛЕНИЯ (передатчик) - ДАННЫЕ ОТПРАВЛЯЮТСЯ ВСЕГДА
#include <esp_now.h>
#include <WiFi.h>
#include "Core/Types.h"
#include "Input/Joystick.h"

// === НАСТРОЙКИ DEBUG ===
#define DEBUG_MODE true  // Меняй на false перед полетом
// =======================

Joystick joystick;

// MAC адрес самолета (приемника)
const uint8_t receiverMac[] = {0xEC, 0xE3, 0x34, 0x1A, 0xB1, 0xA8};

// Глобальные переменные
static ControlData currentData;
static unsigned long lastDataTime = 0;

// Тайминги (в мс)
enum Timing {
  DATA_SEND_INTERVAL = 40,        // Отправка данных
  STATUS_PRINT_INTERVAL = 5000,   // Вывод статуса
  LED_INDICATION_TIME = 25        // Индикация LED
};

// Переменные для неблокирующих таймеров
static unsigned long lastDataSend = 0;
static unsigned long lastStatusPrint = 0;
static unsigned long ledOffTime = 0;
static bool ledState = false;
static bool connectionStatus = false;

// Прототипы функций
bool addPeer(const uint8_t* macAddress);
void printDeviceInfo();
void setupIndication();
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
  handleDataSending(currentMillis);
  handleLEDIndication(currentMillis);
  handleStatusOutput(currentMillis);
  handleConnectionCheck(currentMillis);
}

// Отправка данных - ВСЕГДА, без проверки изменений
void handleDataSending(unsigned long currentMillis) {
  if (currentMillis - lastDataSend >= DATA_SEND_INTERVAL) {
    // ВСЕГДА обновляем данные джойстиков
    joystick.update();
    currentData = joystick.getData();
    currentData.crc = joystick.calculateCRC(currentData);
    
    // ВСЕГДА отправляем данные
    esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&currentData, sizeof(currentData));
    
    if (result == ESP_OK) {
      // Индикация отправки
      digitalWrite(2, HIGH);
      ledState = true;
      ledOffTime = currentMillis + LED_INDICATION_TIME;
      lastDataTime = currentMillis;
      
      // Вывод данных (только в debug режиме)
      #if DEBUG_MODE
        static unsigned long lastDataPrint = 0;
        if (currentMillis - lastDataPrint > 100) { // Ограничиваем частоту вывода
          Serial.printf("J1:%4d,%4d J2:%4d,%4d B1:%d B2:%d\n", 
                       currentData.xAxis1, currentData.yAxis1, 
                       currentData.xAxis2, currentData.yAxis2,
                       currentData.button1, currentData.button2);
          lastDataPrint = currentMillis;
        }
      #endif
    } else {
      #if DEBUG_MODE
        static unsigned long lastErrorPrint = 0;
        if (currentMillis - lastErrorPrint > 1000) {
          Serial.println("❌ Ошибка отправки");
          lastErrorPrint = currentMillis;
        }
      #endif
    }
    
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
      // УБРАН ВЫВОД СТАТИСТИКИ - оставляем только критически важное
      if (connectionStatus) {
        if (currentMillis - lastDataTime < 2000) {
          // Краткий статус вместо подробного
          Serial.println("✅ Связь OK");
        } else {
          Serial.println("⚠️  Нет связи с самолетом");
        }
      } else {
        Serial.println("❌ Самолет не найден");
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