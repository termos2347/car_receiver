// ПУЛЬТ УПРАВЛЕНИЯ (передатчик) - С ВЫВОДОМ MAC-АДРЕСОВ
#include <esp_now.h>
#include <WiFi.h>
#include "Core/Types.h"
#include "Input/Joystick.h"

#define DEBUG_MODE true  // false перед полетом

Joystick joystick;
const uint8_t receiverMac[] = {0xEC, 0xE3, 0x34, 0x1A, 0xB1, 0xA8};

static ControlData currentData;
static unsigned long lastDataTime = 0;
static unsigned long lastDataSend = 0;
static unsigned long ledOffTime = 0;
static bool ledState = false;

enum Timing {
  DATA_SEND_INTERVAL = 40,
  LED_INDICATION_TIME = 25
};

// Функция для форматированного вывода MAC-адреса
void printMacAddress(const uint8_t* mac, const char* label) {
  #if DEBUG_MODE
    Serial.printf("%s: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                 label, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  #endif
}

bool addPeer(const uint8_t* macAddress) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, macAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    return esp_now_add_peer(&peerInfo) == ESP_OK;
}

void setup() {
  #if DEBUG_MODE
    Serial.begin(115200);
    delay(500);
    Serial.println("🎮 ПУЛЬТ УПРАВЛЕНИЯ ЗАПУЩЕН");
    Serial.println("========================");
  #endif
  
  // Вывод MAC-адресов ДО инициализации компонентов
  #if DEBUG_MODE
    Serial.print("MAC пульта:    ");
    Serial.println(WiFi.macAddress());
    printMacAddress(receiverMac, "MAC самолета");
    Serial.println("------------------------");
  #endif
  
  joystick.begin();
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    #if DEBUG_MODE
      Serial.println("❌ Ошибка инициализации ESP-NOW");
    #endif
    return;
  }
  
  if (addPeer(receiverMac)) {
    #if DEBUG_MODE
      Serial.println("✅ Самолет добавлен в пиры");
    #endif
  } else {
    #if DEBUG_MODE
      Serial.println("❌ Ошибка добавления самолета");
    #endif
  }
  
  pinMode(2, OUTPUT);
  
  // Индикация готовности
  for(int i = 0; i < 2; i++) {
    digitalWrite(2, HIGH);
    delay(50);
    digitalWrite(2, LOW);
    delay(50);
  }
  
  #if DEBUG_MODE
    Serial.println("🚀 Пульт готов к работе");
    Serial.println("========================");
  #endif
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Отправка данных каждые 40мс
  if (currentMillis - lastDataSend >= DATA_SEND_INTERVAL) {
    joystick.update();
    currentData = joystick.getData();
    currentData.crc = joystick.calculateCRC(currentData);
    
    esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&currentData, sizeof(currentData));
    
    if (result == ESP_OK) {
      digitalWrite(2, HIGH);
      ledState = true;
      ledOffTime = currentMillis + LED_INDICATION_TIME;
      lastDataTime = currentMillis;
      
      #if DEBUG_MODE
        static unsigned long lastDataPrint = 0;
        if (currentMillis - lastDataPrint > 100) {
          Serial.printf("J1:%4d,%4d J2:%4d,%4d\n", 
                       currentData.xAxis1, currentData.yAxis1, 
                       currentData.xAxis2, currentData.yAxis2);
          lastDataPrint = currentMillis;
        }
      #endif
    }
    
    lastDataSend = currentMillis;
  }
  
  // Управление LED
  if (ledState && currentMillis > ledOffTime) {
    digitalWrite(2, LOW);
    ledState = false;
  }
}