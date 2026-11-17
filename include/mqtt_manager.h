#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <functional>
#include "config.h"

typedef std::function<void(const String& cmd)> RemoteCommandCallback;

class MqttManager {
public:
  MqttManager();

  void init(RemoteCommandCallback callback = nullptr);
  void update();

  bool isConnected();
  bool isWiFiConnected() const;
  bool isMqttConnected() const;
  String getClientId() const;
  String getIpAddress() const;
  
  // Publish sự kiện quẹt thẻ
  bool publishScanEvent(const String &uid, 
                        const String &action, 
                        const String &licensePlate, 
                        const String &ownerName, 
                        unsigned long durationSec, 
                        uint32_t fee, 
                        int parkedCount, 
                        int totalSlots);

  // Publish trạng thái hệ thống
  bool publishStatus(const String &statusMessage);

  // Publish trạng thái barie (open / closed)
  bool publishBarrierStatus(const String &state);

  // Publish cảnh báo (INVALID_CARD, v.v.)
  bool publishAlert(const String &alertType, const String &uid = "");

  // Publish dữ liệu môi trường nhiệt độ & độ ẩm hầm đỗ xe
  bool publishEnvironment(float temp, float humidity, const String &fireStatus);

  // Publish trạng thái báo cháy khẩn cấp & sơ tán
  bool publishFireAlarm(bool isActive);

  void triggerReconnectTest();

  void onMessageReceived(char* topic, byte* payload, unsigned int length);

private:
  void connectWiFi();
  void checkMqttConnection();

  WiFiClient espClient;
  PubSubClient mqttClient;
  RemoteCommandCallback commandCallback;
  
  unsigned long lastReconnectAttempt;
  String clientId;
  bool isReconnected;
};
