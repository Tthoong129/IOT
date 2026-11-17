#include "mqtt_manager.h"

static MqttManager* instance = nullptr;

static void globalMqttCallback(char* topic, byte* payload, unsigned int length) {
  if (instance) {
    instance->onMessageReceived(topic, payload, length);
  }
}

MqttManager::MqttManager()
  : mqttClient(espClient), lastReconnectAttempt(0), isReconnected(false) {
  instance = this;
}

void MqttManager::init(RemoteCommandCallback callback) {
  commandCallback = callback;
  
  // Tạo ClientID ngẫu nhiên để không bị xung đột trên Broker công cộng
  clientId = "ESP32_Parking_" + String(random(0xffff), HEX);

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(globalMqttCallback);
  mqttClient.setBufferSize(512); // Tăng buffer cho JSON

  connectWiFi();
}

void MqttManager::connectWiFi() {
  Serial.println("\n[WI-FI] Dang ket noi toi Wi-Fi: " + String(WIFI_SSID));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Chờ tối đa 10s cho lần kết nối đầu tiên
  unsigned long startWait = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startWait < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WI-FI] Ket noi Wi-Fi THANH CONG!");
    Serial.print("[WI-FI] IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WI-FI] Chua ket noi duoc Wi-Fi (se tu dong thu lai sau).");
  }
}

bool MqttManager::isConnected() {
  return WiFi.status() == WL_CONNECTED && mqttClient.connected();
}

bool MqttManager::isWiFiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool MqttManager::isMqttConnected() const {
  return const_cast<PubSubClient&>(mqttClient).connected();
}

String MqttManager::getClientId() const {
  return clientId;
}

String MqttManager::getIpAddress() const {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return "Chua ket noi";
}

void MqttManager::checkMqttConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    // Nếu mất Wi-Fi, kiểm tra lại kết nối Wi-Fi định kỳ
    if (millis() - lastReconnectAttempt > MQTT_RECONNECT_MS) {
      lastReconnectAttempt = millis();
      Serial.println("[WI-FI] Dang thu ket noi lai Wi-Fi...");
      WiFi.reconnect();
    }
    return;
  }

  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > MQTT_RECONNECT_MS) {
      lastReconnectAttempt = now;
      Serial.print("[MQTT] Dang ket noi toi Broker: ");
      Serial.print(MQTT_BROKER);
      Serial.print(" (ClientID: ");
      Serial.print(clientId);
      Serial.println(")...");

      if (mqttClient.connect(clientId.c_str())) {
        Serial.println("[MQTT] Ket noi Broker THANH CONG!");
        
        // Subscribe topic nhận lệnh điều khiển
        mqttClient.subscribe(MQTT_TOPIC_CMD);
        Serial.print("[MQTT] Da subscribe topic: ");
        Serial.println(MQTT_TOPIC_CMD);

        if (isReconnected) {
          publishStatus("online (reconnected)");
          Serial.println("[MQTT] Da gui goi tin xac nhan Reconnect len MQTTX!");
        } else {
          publishStatus("online");
        }
        isReconnected = true;
      } else {
        Serial.print("[MQTT] Ket noi that bai, rc=");
        Serial.print(mqttClient.state());
        Serial.println(". Se thu lai sau 5s...");
      }
    }
  }
}

void MqttManager::update() {
  checkMqttConnection();
  if (mqttClient.connected()) {
    mqttClient.loop();
  }
}

void MqttManager::onMessageReceived(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  Serial.println("\n---------------------------------------------");
  Serial.print("[MQTT RECEIVED] Topic: ");
  Serial.println(topic);
  Serial.print("[MQTT RECEIVED] Payload: ");
  Serial.println(message);

  if (String(topic) == MQTT_TOPIC_CMD) {
    String cmd = "";

    // Thử parse JSON dạng: {"cmd": "open"}
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (!error && doc["cmd"].is<const char*>()) {
      cmd = doc["cmd"].as<String>();
    } else {
      String msgLower = message;
      msgLower.toLowerCase();
      if (msgLower.indexOf("open_hold") >= 0) {
        cmd = "open_hold";
      } else if (msgLower.indexOf("close") >= 0) {
        cmd = "close";
      } else if (msgLower.indexOf("open") >= 0) {
        cmd = "open";
      } else {
        cmd = message;
      }
    }

    cmd.toLowerCase();
    Serial.print("[MQTT] Lenh duoc phan tich: [");
    Serial.print(cmd);
    Serial.println("]");

    if (commandCallback) {
      commandCallback(cmd);
    }
  }
}

bool MqttManager::publishScanEvent(const String &uid, 
                                   const String &action, 
                                   const String &licensePlate, 
                                   const String &ownerName, 
                                   unsigned long durationSec, 
                                   uint32_t fee, 
                                   int parkedCount, 
                                   int totalSlots) {
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Khong the publish: MQTT chua ket noi!");
    return false;
  }

  JsonDocument doc;
  doc["uid"]          = uid;
  doc["action"]       = action;         // "check_in", "check_out", "invalid"
  doc["licensePlate"] = licensePlate;
  doc["ownerName"]    = ownerName;
  doc["durationSec"]  = durationSec;
  doc["fee"]          = fee;
  doc["parkedCount"]  = parkedCount;
  doc["totalSlots"]   = totalSlots;

  char buffer[384];
  serializeJson(doc, buffer);

  bool success = mqttClient.publish(MQTT_TOPIC_SCAN, buffer);
  if (success) {
    Serial.print("[MQTT PUBLISH] Topic: ");
    Serial.println(MQTT_TOPIC_SCAN);
    Serial.print("[MQTT PUBLISH] Payload: ");
    Serial.println(buffer);
  } else {
    Serial.println("[MQTT PUBLISH] That bai!");
  }
  return success;
}

bool MqttManager::publishStatus(const String &statusMessage) {
  if (!mqttClient.connected()) return false;

  JsonDocument doc;
  doc["clientId"] = clientId;
  doc["status"]   = statusMessage;
  doc["ip"]       = WiFi.localIP().toString();

  char buffer[256];
  serializeJson(doc, buffer);
  return mqttClient.publish(MQTT_TOPIC_STATUS, buffer);
}

bool MqttManager::publishBarrierStatus(const String &state) {
  if (!mqttClient.connected()) return false;

  JsonDocument doc;
  doc["state"] = state; // "open" hoặc "closed"

  char buffer[128];
  serializeJson(doc, buffer);

  bool success = mqttClient.publish(MQTT_TOPIC_BARIE_STAT, buffer);
  if (success) {
    Serial.print("[MQTT PUBLISH] Topic: ");
    Serial.println(MQTT_TOPIC_BARIE_STAT);
    Serial.print("[MQTT PUBLISH] Payload: ");
    Serial.println(buffer);
  }
  return success;
}

bool MqttManager::publishAlert(const String &alertType, const String &uid) {
  if (!mqttClient.connected()) return false;

  JsonDocument doc;
  doc["type"] = alertType;
  if (uid.length() > 0) {
    doc["uid"] = uid;
  }

  char buffer[128];
  serializeJson(doc, buffer);

  bool success = mqttClient.publish(MQTT_TOPIC_ALERT, buffer);
  if (success) {
    Serial.print("[MQTT PUBLISH] Topic: ");
    Serial.println(MQTT_TOPIC_ALERT);
    Serial.print("[MQTT PUBLISH] Payload: ");
    Serial.println(buffer);
  }
  return success;
}

bool MqttManager::publishEnvironment(float temp, float humidity, const String &fireStatus) {
  if (!mqttClient.connected()) return false;
  JsonDocument doc;
  doc["temp"] = round(temp * 10) / 10.0;
  doc["humidity"] = round(humidity * 10) / 10.0;
  doc["fireRisk"] = fireStatus;

  char buffer[128];
  serializeJson(doc, buffer);
  return mqttClient.publish(MQTT_TOPIC_ENV, buffer);
}

bool MqttManager::publishFireAlarm(bool isActive) {
  if (!mqttClient.connected()) return false;
  JsonDocument doc;
  doc["fireAlarm"] = isActive;
  doc["msg"] = isActive ? "HOA HOAN! DANG SO TAN!" : "HE THONG AN TOAN";

  char buffer[128];
  serializeJson(doc, buffer);
  return mqttClient.publish(MQTT_TOPIC_FIRE, buffer);
}

void MqttManager::triggerReconnectTest() {
  Serial.println("\n[TEST RECONNECT] >>> YEU CAU TEST RECONNECT DUOC KICH HOAT <<<");
  Serial.println("[TEST RECONNECT] Dang ngat ket noi MQTT hien tai de kiem tra co che tu dong ket noi lai...");
  isReconnected = true;
  mqttClient.disconnect();
  clientId = "ESP32_Parking_" + String(random(0xffff), HEX);
  lastReconnectAttempt = 0;
}

