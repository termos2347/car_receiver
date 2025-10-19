#pragma once
#include <esp_now.h>
#include <WiFi.h>
#include "../Core/Types.h"

class ESPNowManager {
public:
    void begin();
    bool sendData(const ControlData& data);
    bool isConnected();
    void pairWithReceiver();
    bool addPeer(const uint8_t* mac);
    
    // Управление соединением
    void setReceiverMAC(const uint8_t* mac);
    bool removePeer();
    int getLastError() const { return lastError; }
    
    // Статистика
    uint32_t getSentCount() const { return sentCount; }
    uint32_t getErrorCount() const { return errorCount; }
    uint32_t getSuccessCount() const { return successCount; }
    float getSuccessRate() const;
    
private:
    uint8_t receiverMac[6] = {0};
    bool paired = false;
    unsigned long lastSendTime = 0;
    int lastError = 0;
    
    // Статистика
    uint32_t sentCount = 0;
    uint32_t errorCount = 0;
    uint32_t successCount = 0;
    
    // Статический метод не может обращаться к нестатическим членам
    // Сделаем обертку для callback
    static ESPNowManager* instance;
    static void onDataSentStatic(const uint8_t* mac, esp_now_send_status_t status);
    void onDataSent(const uint8_t* mac, esp_now_send_status_t status);
};