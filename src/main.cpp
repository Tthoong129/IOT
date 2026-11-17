#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include "config.h"

// =========================================================================
// KHỞI TẠO CÁC ĐỐI TƯỢNG PHẦN CỨNG
// =========================================================================
WiFiClient espClient;
PubSubClient mqttClient(espClient);
MFRC522 rfid(PIN_SS, PIN_RST);
Servo barieServo;
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
DHT dht(PIN_DHT, DHT22);

// =========================================================================
// CƠ SỞ DỮ LIỆU THẺ XE TRONG BÃI
// =========================================================================
struct TheXe {
  String uid;
  String bienSo;
  String chuXe;
  bool dangTrongBai;
  unsigned long thoiGianVao; // tính bằng millis()
};

TheXe danhSachThe[] = {
  {"04 3B 2A 1C", "29A-123.45", "Nguyen Van A", false, 0},
  {"14 8C 3D 2E", "51F-999.88", "Tran Thi B",   false, 0},
  {"24 D5 4E 3F", "43C-678.90", "Le Van C",     false, 0},
  {"34 E6 5F 4A", "92B-456.78", "Pham Thi D",   false, 0}
};
const int TONG_SO_THE = sizeof(danhSachThe) / sizeof(danhSachThe[0]);
const int SUC_CHUA_MAX = 2; // Bãi xe mô phỏng tối đa 2 chỗ đỗ (để dễ test kịch bản đầy bãi)

// =========================================================================
// BIẾN TRẠNG THÁI HỆ THỐNG
// =========================================================================
bool barieDangMo = false;
bool giuMoThuCong = false;
bool coBaoChay = false;
bool ledChopState = false;

unsigned long timerDongBarie = 0;
unsigned long timerGuiMoiTruong = 0;
unsigned long timerMqttReconnect = 0;
unsigned long timerChopLed = 0;
unsigned long thoiGianQuetCuoi = 0;
String uidVuaQuet = "";

float khoangCachHienTai = 100.0;
float nhietDoHienTai = 28.0;
float doAmHienTai = 65.0;

String clientIdMqtt = "";

// =========================================================================
// HÀM TIỆN ÍCH PHẦN CỨNG (CÒI, ĐÈN, BARIE, LCD)
// =========================================================================
void keuCoi(int soLan, int thoiGianMs = 80, int nghiMs = 50) {
  for (int i = 0; i < soLan; i++) {
    long soXung = (long)1500 * thoiGianMs / 1000;
    for (long j = 0; j < soXung; j++) {
      digitalWrite(PIN_BUZZER, HIGH);
      delayMicroseconds(333);
      digitalWrite(PIN_BUZZER, LOW);
      delayMicroseconds(333);
    }
    if (soLan > 1 && i < soLan - 1) {
      delay(nghiMs);
    }
  }
}

void batDen(bool batDo, bool batXanh) {
  digitalWrite(PIN_LED_RED, batDo ? HIGH : LOW);
  digitalWrite(PIN_LED_GREEN, batXanh ? HIGH : LOW);
}

int demSoXeTrongBai() {
  int dem = 0;
  for (int i = 0; i < TONG_SO_THE; i++) {
    if (danhSachThe[i].dangTrongBai) dem++;
  }
  return dem;
}

void capNhatManHinhChinh() {
  if (coBaoChay) return;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BAI XE: ");
  lcd.print(demSoXeTrongBai());
  lcd.print("/");
  lcd.print(SUC_CHUA_MAX);
  lcd.print(" XE");

  lcd.setCursor(0, 1);
  lcd.print("Moi quet the...");
}

void moBarie(bool giuMo = false) {
  barieServo.write(SERVO_OPEN_ANGLE);
  barieDangMo = true;
  giuMoThuCong = giuMo;
  timerDongBarie = millis();
  batDen(false, true); // Đèn XANH sáng
  mqttClient.publish(MQTT_TOPIC_BARIE_STAT, "{\"state\":\"open\"}");
}

void dongBarie() {
  barieServo.write(SERVO_CLOSE_ANGLE);
  barieDangMo = false;
  giuMoThuCong = false;
  batDen(true, false); // Đèn ĐỎ sáng
  mqttClient.publish(MQTT_TOPIC_BARIE_STAT, "{\"state\":\"closed\"}");
  capNhatManHinhChinh();
}

float docKhoangCachCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long thoiGian = pulseIn(PIN_ECHO, HIGH, 25000); // timeout 25ms
  if (thoiGian == 0) return 999.0;
  return thoiGian * 0.0343 / 2.0;
}

int timThe(String uid) {
  for (int i = 0; i < TONG_SO_THE; i++) {
    if (uid.equalsIgnoreCase(danhSachThe[i].uid)) {
      return i;
    }
  }
  return -1;
}

// =========================================================================
// TRUYỀN THÔNG MQTT (UPLINK & DOWNLINK)
// =========================================================================
void guiDuLieuQuetThe(TheXe &the, String hanhDong, unsigned long thoiGianDo, uint32_t tien) {
  JsonDocument doc;
  doc["uid"]          = the.uid;
  doc["action"]       = hanhDong; // "check_in" hoặc "check_out"
  doc["licensePlate"] = the.bienSo;
  doc["ownerName"]    = the.chuXe;
  doc["durationSec"]  = thoiGianDo;
  doc["fee"]          = tien;
  doc["parkedCount"]  = demSoXeTrongBai();
  doc["totalSlots"]   = SUC_CHUA_MAX;

  char buf[300];
  serializeJson(doc, buf);
  mqttClient.publish(MQTT_TOPIC_SCAN, buf);
  Serial.printf("[MQTT] Publish topic %s: %s\n", MQTT_TOPIC_SCAN, buf);
}

void guiCanhBaoTheLa(String uid) {
  JsonDocument doc;
  doc["type"] = "INVALID_CARD";
  doc["uid"]  = uid;

  char buf[128];
  serializeJson(doc, buf);
  mqttClient.publish(MQTT_TOPIC_ALERT, buf);

  // Gửi cả vào topic scan để dashboard hiển thị log lỗi
  doc.clear();
  doc["uid"]          = uid;
  doc["action"]       = "invalid";
  doc["licensePlate"] = "N/A";
  doc["ownerName"]    = "The la / Khong hop le";
  doc["durationSec"]  = 0;
  doc["fee"]          = 0;
  doc["parkedCount"]  = demSoXeTrongBai();
  doc["totalSlots"]   = SUC_CHUA_MAX;
  char buf2[256];
  serializeJson(doc, buf2);
  mqttClient.publish(MQTT_TOPIC_SCAN, buf2);
}

void guiDuLieuMoiTruong() {
  JsonDocument doc;
  doc["temp"] = round(nhietDoHienTai * 10) / 10.0;
  doc["humidity"] = round(doAmHienTai * 10) / 10.0;
  doc["fireRisk"] = coBaoChay ? "DANGER_FIRE" : (nhietDoHienTai >= 38.0 ? "WARNING_HIGH_TEMP" : "NORMAL");

  char buf[128];
  serializeJson(doc, buf);
  mqttClient.publish(MQTT_TOPIC_ENV, buf);
}

void kichHoatBaoChay(bool bat) {
  coBaoChay = bat;
  if (bat) {
    moBarie(true);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CHAY! THOAT HIEM");
    lcd.setCursor(0, 1);
    lcd.print("MO TOANG BARIE!");
    keuCoi(5, 150, 80);

    JsonDocument doc;
    doc["fireAlarm"] = true;
    doc["msg"] = "HOA HOAN! DANG SO TAN!";
    char buf[128];
    serializeJson(doc, buf);
    mqttClient.publish(MQTT_TOPIC_FIRE, buf);

    Serial.println("\n[ALERT] [FIRE] Phat hien nhiet do cao! Mo toang barie de so tan!\n");
  } else {
    coBaoChay = false;
    dongBarie();
    mqttClient.publish(MQTT_TOPIC_FIRE, "{\"fireAlarm\":false,\"msg\":\"HE THONG AN TOAN\"}");
    Serial.println("[SYSTEM] Da tat bao chay, he thong ve binh thuong.");
  }
}

// Xử lý lệnh nhận được từ Dashboard qua MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  Serial.printf("[MQTT RECV] Topic: %s | Payload: %s\n", topic, message.c_str());

  if (String(topic) == MQTT_TOPIC_CMD) {
    String cmd = message;
    // Nếu là JSON dạng {"cmd":"open"}
    JsonDocument doc;
    if (!deserializeJson(doc, message) && doc["cmd"].is<const char*>()) {
      cmd = doc["cmd"].as<String>();
    }
    cmd.toLowerCase();

    if (cmd == "open") {
      Serial.println("[CMD] Nhan lenh MO barie");
      keuCoi(1, 80);
      moBarie(false);
    } else if (cmd == "open_hold") {
      Serial.println("[CMD] Nhan lenh MO GIU barie (mo phong ket)");
      keuCoi(2, 80);
      moBarie(true);
    } else if (cmd == "close") {
      Serial.println("[CMD] Nhan lenh DONG barie");
      keuCoi(1, 80);
      dongBarie();
    } else if (cmd == "fire" || cmd == "fire_alarm") {
      kichHoatBaoChay(true);
    } else if (cmd == "nofire" || cmd == "fire_clear") {
      kichHoatBaoChay(false);
    }
  }
}

void ketNoiWiFi() {
  Serial.printf("[WIFI] Dang ket noi vao mạng %s...", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
    delay(400);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WIFI] Ket noi timeout, se thu lai trong loop.");
  }
}

void ketNoiMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqttClient.connected()) return;

  unsigned long now = millis();
  if (now - timerMqttReconnect < 5000) return;
  timerMqttReconnect = now;

  if (clientIdMqtt == "") {
    clientIdMqtt = "ESP32_Parking_" + String(random(0xffff), HEX);
  }

  Serial.printf("[MQTT] Dang ket noi Broker %s:%d...\n", MQTT_BROKER, MQTT_PORT);
  if (mqttClient.connect(clientIdMqtt.c_str())) {
    Serial.println("[MQTT] Connected thanh cong!");
    mqttClient.subscribe(MQTT_TOPIC_CMD);
    mqttClient.publish(MQTT_TOPIC_STATUS, "{\"status\":\"online\"}");
  } else {
    Serial.printf("[MQTT] Ket noi that bai, ma loi rc=%d\n", mqttClient.state());
  }
}

// =========================================================================
// NGHIỆP VỤ QUÉT THẺ (CHECK-IN / CHECK-OUT)
// =========================================================================
void xuLyQuetThe(String uid) {
  if (coBaoChay) {
    Serial.println("[WARNING] He thong dang trong che do bao chay, khong nhan the.");
    return;
  }
  if (barieDangMo) {
    Serial.println("[WARNING] Barie dang mo cho xe truoc, vui long doi giay lat.");
    return;
  }

  // Chống dội thẻ (debounce 3s)
  unsigned long now = millis();
  if (uid == uidVuaQuet && (now - thoiGianQuetCuoi < 3000)) {
    Serial.println("[DEBOUNCE] The vua duoc quet, vui long khong quet lien tuc.");
    return;
  }
  uidVuaQuet = uid;
  thoiGianQuetCuoi = now;

  int idx = timThe(uid);
  if (idx == -1) {
    // Thẻ không có trong cơ sở dữ liệu
    Serial.printf("\n[ALERT] The la khong hop le! UID: %s -> Tu choi truy cap!\n", uid.c_str());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("THE KHONG HOP LE");
    lcd.setCursor(0, 1);
    lcd.print("Tu choi truy cap");

    batDen(true, false); // Đèn Đỏ
    keuCoi(3, 120, 80);
    guiCanhBaoTheLa(uid);
    delay(1500);
    capNhatManHinhChinh();
    return;
  }

  TheXe &the = danhSachThe[idx];

  // 1. Trường hợp xe VÀO bãi (Check-in)
  if (!the.dangTrongBai) {
    int soXe = demSoXeTrongBai();
    if (soXe >= SUC_CHUA_MAX) {
      Serial.printf("[FULL] Bai xe da day (%d/%d xe)! Khong the tiep nhan.\n", soXe, SUC_CHUA_MAX);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BAI XE DA DAY!");
      lcd.setCursor(0, 1);
      lcd.print("Vui long quay lai");
      keuCoi(2, 200, 100);
      delay(1500);
      capNhatManHinhChinh();
      return;
    }

    the.dangTrongBai = true;
    the.thoiGianVao = now;

    Serial.printf("\n[CHECK-IN] Xe vao: %s | Chu xe: %s | Bien so: %s\n", the.uid.c_str(), the.chuXe.c_str(), the.bienSo.c_str());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("VAO: ");
    lcd.print(the.bienSo);
    lcd.setCursor(0, 1);
    lcd.print("Moi vao! -> MO");

    keuCoi(1, 150);
    moBarie(false);
    guiDuLieuQuetThe(the, "check_in", 0, 0);
  }
  // 2. Trường hợp xe RA bãi (Check-out & Tính tiền)
  else {
    the.dangTrongBai = false;
    unsigned long thoiGianDoSec = (now - the.thoiGianVao) / 1000;
    if (thoiGianDoSec == 0) thoiGianDoSec = 1;

    // Tính cước: dưới 10s: 3.000đ, 10-30s: 5.000đ, trên 30s: 9.000đ
    uint32_t tien = 3000;
    if (thoiGianDoSec > 30) {
      tien = 9000;
    } else if (thoiGianDoSec >= 10) {
      tien = 5000;
    }

    Serial.printf("\n[CHECK-OUT] Xe ra: %s | Bien so: %s | Do: %lus | Phi: %u VND\n", 
                  the.uid.c_str(), the.bienSo.c_str(), thoiGianDoSec, tien);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RA: ");
    lcd.print(the.bienSo);
    lcd.setCursor(0, 1);
    lcd.printf("T:%lus %ud", thoiGianDoSec, tien);

    keuCoi(2, 100, 60);
    moBarie(false);
    guiDuLieuQuetThe(the, "check_out", thoiGianDoSec, tien);
  }
}

// =========================================================================
// LỆNH ĐIỀU KHIỂN DEBUG TRÊN SERIAL MONITOR
// =========================================================================
void xuLyLenhSerial(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  // Quẹt nhanh thẻ bằng số 1, 2, 3, 4
  if (cmd.length() == 1 && cmd[0] >= '1' && cmd[0] <= ('0' + TONG_SO_THE)) {
    int idx = cmd[0] - '1';
    Serial.printf("\n[SERIAL KEY] Quet the so %d (UID: %s)\n", idx + 1, danhSachThe[idx].uid.c_str());
    xuLyQuetThe(danhSachThe[idx].uid);
    return;
  }

  cmd.toLowerCase();
  if (cmd == "open") {
    moBarie(false);
    Serial.println("[MANUAL] Da mo barie thu cong.");
  } else if (cmd == "close") {
    dongBarie();
    Serial.println("[MANUAL] Da dong barie thu cong.");
  } else if (cmd == "fire") {
    kichHoatBaoChay(true);
  } else if (cmd == "nofire") {
    kichHoatBaoChay(false);
  } else if (cmd == "sonar") {
    Serial.printf("[SONAR] Khoang cach: %.1f cm\n", khoangCachHienTai);
  } else if (cmd == "env") {
    Serial.printf("[DHT22] Nhiet do: %.1f C | Do am: %.1f %%\n", nhietDoHienTai, doAmHienTai);
  } else if (cmd.startsWith("scan ")) {
    String uid = cmd.substring(5);
    uid.trim();
    uid.toUpperCase();
    xuLyQuetThe(uid);
  } else if (cmd == "help" || cmd == "?") {
    Serial.println("\n--- HUONG DAN TEST SERIAL ---");
    Serial.println("  1..4       : Quet nhanh the 1 den 4 (Vao/Ra)");
    Serial.println("  scan <UID> : Quet the bat ky (VD: scan 99 99 99 99)");
    Serial.println("  open/close : Mo hoac dong barie thu cong");
    Serial.println("  fire/nofire: Bat hoac tat bao chay khan cap");
    Serial.println("  sonar/env  : Xem khoang cach sieu am & nhiet am DHT22");
    Serial.println("-----------------------------\n");
  }
}

// =========================================================================
// SETUP & LOOP CHÍNH CỦA CHƯƠNG TRÌNH
// =========================================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[SYSTEM] Khoi dong He thong Bai giu xe Thong minh ESP32...");

  // 1. Cấu hình chân I/O
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  batDen(true, false); // Khởi động: Đèn Đỏ sáng (cổng đóng)

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  // 2. Màn hình LCD 1602 I2C
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  lcd.init();
  lcd.backlight();
  capNhatManHinhChinh();

  // 3. Servo Barie
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  barieServo.setPeriodHertz(50);
  barieServo.attach(PIN_SERVO, 500, 2400);
  barieServo.write(SERVO_CLOSE_ANGLE);

  // 4. Cảm biến nhiệt ẩm DHT22
  dht.begin();

  // 5. Cảm biến thẻ RFID RC522 (SPI)
  SPI.begin();
  rfid.PCD_Init();

  // 6. Kết nối Wi-Fi & MQTT
  ketNoiWiFi();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);
  ketNoiMQTT();

  // Kêu còi bíp ngắn báo hiệu đã sẵn sàng
  keuCoi(2, 60, 40);
  Serial.println("[SYSTEM] He thong da san sang! Nhap 'help' hoac bam phim 1-4 de test.");
}

void loop() {
  unsigned long now = millis();

  // 1. Duy trì kết nối Wi-Fi và MQTT
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      ketNoiMQTT();
    } else {
      mqttClient.loop();
    }
  }

  // 2. Xử lý lệnh gõ từ bàn phím Serial Monitor
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    xuLyLenhSerial(cmd);
  }

  // 3. Đọc cảm biến nhiệt độ & siêu âm định kỳ mỗi 1 giây
  static unsigned long timerDocSensors = 0;
  if (now - timerDocSensors >= 1000) {
    timerDocSensors = now;

    khoangCachHienTai = docKhoangCachCm();

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      nhietDoHienTai = t;
      doAmHienTai = h;
    }

    // Tự động kích hoạt báo cháy nếu nhiệt độ hầm xe >= 50°C
    if (nhietDoHienTai >= 50.0 && !coBaoChay) {
      Serial.printf("[FIRE SENSOR] Nhiet do ham xe dat %.1f C! Kich hoat bao chay!\n", nhietDoHienTai);
      kichHoatBaoChay(true);
    }
  }

  // 4. Gửi dữ liệu môi trường lên MQTT mỗi 5 giây
  if (now - timerGuiMoiTruong >= 5000) {
    timerGuiMoiTruong = now;
    if (mqttClient.connected()) {
      guiDuLieuMoiTruong();
    }
  }

  // 5. Quản lý cánh tay Barie & Thuật toán Chống Kẹp Xe (Anti-Pinch)
  if (barieDangMo && !giuMoThuCong && !coBaoChay) {
    if (now - timerDongBarie >= BARRIER_HOLD_MS) {
      // Khi chuẩn bị đóng barie, kiểm tra cảm biến siêu âm:
      // Nếu phát hiện xe vẫn đang đi qua ngay dưới cần (< 15cm) -> HOÃN ĐÓNG
      if (khoangCachHienTai > 0 && khoangCachHienTai < 15.0) {
        timerDongBarie = now; // Hoãn thêm thời gian, giữ mở tiếp
        Serial.printf("[ANTI-PINCH] Phat hien xe dang qua barie (dist=%.1fcm)! Hoan dong barie.\n", khoangCachHienTai);
        lcd.setCursor(0, 1);
        lcd.print("XE QUA: Giu mo..");
      } else {
        // Không có vật cản -> Đóng barie an toàn
        Serial.println("[BARIE] Xe da qua an toan, bat dau ha can barie.");
        lcd.setCursor(0, 1);
        lcd.print("Dang dong barie.");
        dongBarie();
      }
    }
  }

  // 6. Nhấp nháy đèn cảnh báo khi có sự cố báo cháy
  if (coBaoChay) {
    if (now - timerChopLed >= 200) {
      timerChopLed = now;
      ledChopState = !ledChopState;
      batDen(ledChopState, !ledChopState);
    }
  }

  // 7. Quét thẻ RFID thực tế (từ cảm biến MFRC522)
  if (!coBaoChay && !barieDangMo) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String uid = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) uid += "0";
        uid += String(rfid.uid.uidByte[i], HEX);
        if (i < rfid.uid.size - 1) uid += " ";
      }
      uid.toUpperCase();

      Serial.printf("\n[RFID HARDWARE] Quet duoc the UID: %s\n", uid.c_str());
      xuLyQuetThe(uid);

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }
}
