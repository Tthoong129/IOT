#include "parking_system.h"

ParkingSystem::ParkingSystem()
  : dht(PIN_DHT, DHT22), currentState(STATE_IDLE), stateTimer(0), holdOpen(false),
    currentTemp(28.0), currentHumidity(65.0), currentDistance(100.0),
    vehicleApproaching(false), fireEmergency(false),
    lastSensorPoll(0), lastEnvMqttPoll(0), lastLedBlink(0), ledBlinkState(false) {}

void ParkingSystem::init() {
  Serial.println("\n=============================================================");
  Serial.println("   HE THONG QUAN LY BAI GIU XE THONG MINH AIOT - ESP32 V4    ");
  Serial.println("=============================================================");

  buzzer.init();
  display.init();
  barrier.init();
  rfidReader.init();
  initSensorsAndLeds();

  buzzer.beep(1, 100);
  display.showHomeScreen(cardMgr.getParkedCount(), cardMgr.getTotalCards());
  
  // In danh sách thẻ đã đăng ký trong hệ thống
  cardMgr.printDatabase();

  // Khởi tạo kết nối WiFi và MQTT
  mqttMgr.init([this](const String &cmd) {
    this->handleRemoteCommand(cmd);
  });

  // In bảng hướng dẫn lệnh debug Terminal
  printHelpMenu();

  Serial.println("\n[SYSTEM] >>> He thong da san sang! <<<");
  Serial.println("[SYSTEM] Quet the RFID Wokwi hoac nhap lenh tren Terminal (nhap '1'-'8', 'fire', 'help')\n");
}

void ParkingSystem::initSensorsAndLeds() {
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  setTrafficLeds(true, false); // Khởi động: Đèn Đỏ sáng (Cổng đóng)

  dht.begin();
  Serial.println("[HARDWARE] Da khoi tao: Cam bien khoang cach sieu am HC-SR04");
  Serial.println("[HARDWARE] Da khoi tao: Cam bien nhiet do & do am ham xe DHT22");
  Serial.println("[HARDWARE] Da khoi tao: He thong den tin hieu giao thong LED Do/Xanh");
}

void ParkingSystem::update() {
  static String serialBuffer = "";
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      serialBuffer.trim();
      if (serialBuffer.length() > 0) {
        handleSerialCommand(serialBuffer);
        serialBuffer = "";
      } else {
        Serial.println("\n[TERMINAL] San sang! Nhap '1'-'8', 'help', 'fire', 'open', 'status':");
        Serial.flush();
      }
    } else {
      Serial.print(c);
      serialBuffer += c;
    }
  }

  // In Heartbeat định kỳ mỗi 5 giây
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat >= 5000) {
    lastHeartbeat = millis();
    Serial.printf("[HEARTBEAT %lus] Bai xe: %d/%d | Barie: %s | Nhiet do: %.1f°C | Khoang cach: %.1fcm | MQTT: %s\n",
                  millis() / 1000,
                  cardMgr.getParkedCount(),
                  cardMgr.getTotalCards(),
                  barrier.isOpen() ? "MO" : "DONG",
                  currentTemp,
                  currentDistance,
                  mqttMgr.isMqttConnected() ? "ONLINE" : "OFFLINE");
    Serial.flush();
  }

  mqttMgr.update();
  updateSensorsAndLeds();
  handleRFIDScan();
  updateStateMachine();
}

void ParkingSystem::updateSensorsAndLeds() {
  unsigned long now = millis();

  // 1. Đọc cảm biến định kỳ mỗi 500ms
  if (now - lastSensorPoll >= 500) {
    lastSensorPoll = now;

    // Đọc khoảng cách siêu âm
    currentDistance = readDistanceCm();

    // Phát hiện xe đang tiến tới cổng (< 25cm)
    if (currentDistance > 0 && currentDistance < 25.0) {
      if (!vehicleApproaching && currentState == STATE_IDLE) {
        vehicleApproaching = true;
        Serial.printf("[SONAR] Phat hien phuong tien dang tien vao cong! Khoang cach: %.1f cm\n", currentDistance);
        display.showWarning("CO XE DANG TOI", "Xin moi quet the");
      }
    } else {
      if (vehicleApproaching && currentState == STATE_IDLE) {
        vehicleApproaching = false;
        display.showHomeScreen(cardMgr.getParkedCount(), cardMgr.getTotalCards());
      }
    }

    // Đọc nhiệt độ và độ ẩm DHT22
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      currentTemp = t;
      currentHumidity = h;
    }

    // Tự động kích hoạt báo cháy khẩn cấp nếu nhiệt độ vượt quá 50°C
    if (currentTemp >= 50.0 && !fireEmergency) {
      Serial.println("\n[ALERT] [FIRE_EMERGENCY] Basement temperature exceeded 50.0 C threshold! Triggering emergency evacuation protocol.");
      triggerFireEmergency(true);
    }
  }

  // 2. Publish Telemetry Môi trường định kỳ mỗi 5 giây
  if (now - lastEnvMqttPoll >= 5000) {
    lastEnvMqttPoll = now;
    String risk = "NORMAL";
    if (currentTemp >= 50.0 || fireEmergency) risk = "DANGER_FIRE";
    else if (currentTemp >= 38.0) risk = "WARNING_HIGH_TEMP";
    
    mqttMgr.publishEnvironment(currentTemp, currentHumidity, risk);
  }

  // 3. Điều khiển đèn LED theo trạng thái
  if (fireEmergency) {
    // Nhấp nháy đèn báo động liên tục khi có cháy
    if (now - lastLedBlink >= 200) {
      lastLedBlink = now;
      ledBlinkState = !ledBlinkState;
      setTrafficLeds(ledBlinkState, !ledBlinkState);
    }
  } else if (barrier.isOpen()) {
    setTrafficLeds(false, true); // Đèn Xanh sáng khi Barie MỞ
  } else {
    setTrafficLeds(true, false); // Đèn Đỏ sáng khi Barie ĐÓNG
  }
}

float ParkingSystem::readDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duration = pulseIn(PIN_ECHO, HIGH, 25000); // Timeout 25ms (~4m)
  if (duration <= 0) return 100.0;
  float dist = (duration * 0.0343) / 2.0;
  return dist;
}

void ParkingSystem::setTrafficLeds(bool redOn, bool greenOn) {
  digitalWrite(PIN_LED_RED, redOn ? HIGH : LOW);
  digitalWrite(PIN_LED_GREEN, greenOn ? HIGH : LOW);
}

void ParkingSystem::triggerFireEmergency(bool enable) {
  fireEmergency = enable;
  if (enable) {
    currentState = STATE_FIRE_EVACUATE;
    holdOpen = true;
    barrier.open();
    buzzer.beep(5, 200, 100);
    display.showWarning("CHAY! THOAT HIEM", "MO TOANG BARIE!");
    mqttMgr.publishFireAlarm(true);
    mqttMgr.publishBarrierStatus("open");
    Serial.println("\n=============================================================");
    Serial.println(">>> [CRITICAL] EMERGENCY EVACUATION PROTOCOL ACTIVATED <<<");
    Serial.println(">>> Barrier gate forced open (90 deg) for immediate exit  <<<");
    Serial.println("=============================================================\n");
  } else {
    currentState = STATE_IDLE;
    holdOpen = false;
    barrier.close();
    display.showHomeScreen(cardMgr.getParkedCount(), cardMgr.getTotalCards());
    mqttMgr.publishFireAlarm(false);
    mqttMgr.publishBarrierStatus("closed");
    Serial.println("[SYSTEM] Da tat bao chay, he thong tro ve trang thai binh thuong.");
  }
}

void ParkingSystem::handleSerialCommand(String input) {
  input.trim();
  if (input.length() == 0) return;

  Serial.println();
  Serial.print("[TERMINAL CMD] >> ");
  Serial.println(input);

  String lower = input;
  lower.toLowerCase();

  if (lower == "help" || lower == "?" || lower == "menu") {
    printHelpMenu();
  } else if (lower == "status" || lower == "info") {
    printSystemStatus();
  } else if (lower == "list" || lower == "db" || lower == "cards") {
    cardMgr.printDatabase();
  } else if (lower == "open") {
    handleRemoteCommand("open");
  } else if (lower == "open_hold" || lower == "hold") {
    handleRemoteCommand("open_hold");
  } else if (lower == "close") {
    handleRemoteCommand("close");
  } else if (lower == "fire" || lower == "chay") {
    triggerFireEmergency(true);
  } else if (lower == "nofire" || lower == "tatchay") {
    triggerFireEmergency(false);
  } else if (lower == "sonar" || lower == "dist") {
    Serial.printf("[SONAR] Khoang cach hien tai: %.1f cm (Trang thai: %s)\n", currentDistance, vehicleApproaching ? "CO XE TIEP CAN" : "TRONG");
  } else if (lower == "env" || lower == "dht") {
    Serial.printf("[DHT22] Nhiet do ham xe: %.1f °C | Do am: %.1f %%\n", currentTemp, currentHumidity);
  } else if (lower == "reconnect" || lower == "test") {
    mqttMgr.triggerReconnectTest();
  } else if (lower == "clear" || lower == "reset") {
    cardMgr.resetAllCards();
    display.showHomeScreen(cardMgr.getParkedCount(), cardMgr.getTotalCards());
    Serial.println("[SYSTEM] Da reset toan bo xe trong bai ve trang thai TRONG (0 xe).");
  } else if (lower.startsWith("scan ")) {
    String uid = input.substring(5);
    uid.trim();
    processCardScan(uid, true);
  } else if (lower.startsWith("card ")) {
    int cardNum = input.substring(5).toInt();
    if (cardNum >= 1 && cardNum <= cardMgr.getTotalCards()) {
      processCardScan(cardMgr.getCard(cardNum - 1).uid, true);
    } else {
      Serial.printf("[TERMINAL] So the khong hop le! Vui long nhap tu 1 den %d\n", cardMgr.getTotalCards());
    }
  } else if (input.length() == 1 && input[0] >= '1' && input[0] <= ('0' + cardMgr.getTotalCards())) {
    int cardNum = input[0] - '0';
    processCardScan(cardMgr.getCard(cardNum - 1).uid, true);
  } else if (lower == "topic" || lower == "topics") {
    printMqttTopics();
  } else {
    String clean = CardManager::normalizeUID(input);
    if (clean.length() >= 4) {
      processCardScan(input, true);
    } else {
      Serial.print("[TERMINAL] Lenh khong hop le: '");
      Serial.print(input);
      Serial.println("'. Go 'help' hoac '?' de xem danh sach lenh debug.");
    }
  }
}

void ParkingSystem::printHelpMenu() const {
  Serial.println("+---------------------------------------------------------------+");
  Serial.println("|               BANG HUONG DAN LENH DEBUG TERMINAL              |");
  Serial.println("+----------------+----------------------------------------------+");
  Serial.println("| LENH           | CHUC NANG                                    |");
  Serial.println("+----------------+----------------------------------------------+");
  Serial.println("| 1 .. 8         | Quet nhanh the 1 -> 8 (Vao/Ra & Tinh tien)   |");
  Serial.println("| scan <UID>     | Gia lap quet the bat ky (VD: scan 01 02 03 04)|");
  Serial.println("| open           | Dieu khien MO barie thu cong                 |");
  Serial.println("| open_hold/hold | Dieu khien MO GIU barie (mo phong ket)       |");
  Serial.println("| close          | Dieu khien DONG barie thu cong               |");
  Serial.println("| fire           | Kich hoat CHE DO THOAT HIEM BAO CHAY (Mo barie)|");
  Serial.println("| nofire         | Tat bao chay, dua he thong ve binh thuong    |");
  Serial.println("| sonar          | Kiem tra khoang cach cam bien sieu am        |");
  Serial.println("| env            | Xem nhiet do va do am ham xe DHT22           |");
  Serial.println("| status         | Xem toan bo trang thai (WiFi, MQTT, Sensors) |");
  Serial.println("| list / db      | Xem danh sach the & trang thai trong bai     |");
  Serial.println("| reconnect/test | Kiem tra co che tu dong ket noi lai MQTT     |");
  Serial.println("| clear / reset  | Xoa tat ca xe dang gui ve 0                  |");
  Serial.println("| help / ?       | Hien thi lai menu tro giup nay               |");
  Serial.println("+----------------+----------------------------------------------+");
}

void ParkingSystem::printSystemStatus() const {
  Serial.println("\n+---------------------------------------------------------------+");
  Serial.println("|                     THONG TIN HE THONG REALTIME               |");
  Serial.println("+---------------------------------------------------------------+");
  
  const char* stateStr = "UNKNOWN";
  switch (currentState) {
    case STATE_IDLE:          stateStr = "IDLE (Cho quet the)"; break;
    case STATE_OPENING:       stateStr = "OPENING (Dang mo Barie)"; break;
    case STATE_OPEN:          stateStr = "OPEN (Dang giu mo Barie)"; break;
    case STATE_CLOSING:       stateStr = "CLOSING (Dang dong Barie)"; break;
    case STATE_ALERT_HOLD:    stateStr = "ALERT (Canh bao / The khong hop le)"; break;
    case STATE_FIRE_EVACUATE: stateStr = "FIRE_EVACUATE (Emergency Alarm Active)"; break;
  }
  Serial.print("  * Trang thai he thong : "); Serial.println(stateStr);
  Serial.print("  * Trang thai Barie    : "); 
  Serial.println(barrier.isOpen() ? (holdOpen ? "DANG MO GIU (HOLD OPEN)" : "DANG MO") : "DANG DONG");
  Serial.print("  * Den tin hieu Traffic: "); 
  Serial.println(barrier.isOpen() ? "XANH (GO)" : "DO (STOP)");
  Serial.print("  * Khoang cach Sieu am : "); Serial.print(currentDistance); Serial.println(" cm");
  Serial.print("  * Nhiet do ham xe     : "); Serial.print(currentTemp); Serial.println(" °C");
  Serial.print("  * Do am ham xe        : "); Serial.print(currentHumidity); Serial.println(" %");
  Serial.print("  * Che do PCCC         : "); Serial.println(fireEmergency ? "DANGER_FIRE_ALARM (Active)" : "NORMAL (Secure)");
  Serial.print("  * So xe trong bai     : "); Serial.print(cardMgr.getParkedCount());
  Serial.print(" / "); Serial.print(cardMgr.getTotalCards()); Serial.println(" xe");
  Serial.print("  * Wi-Fi               : "); 
  Serial.println(mqttMgr.isWiFiConnected() ? ("DA KET NOI (IP: " + mqttMgr.getIpAddress() + ")") : "MAT KET NOI");
  Serial.print("  * MQTT Broker         : ");
  Serial.println(mqttMgr.isMqttConnected() ? ("DA KET NOI (" + String(MQTT_BROKER) + ":" + String(MQTT_PORT) + ")") : "CHUA KET NOI");
  Serial.print("  * Bo nho Free Heap    : "); Serial.print(ESP.getFreeHeap()); Serial.println(" bytes");
  Serial.println("+---------------------------------------------------------------+\n");
}

void ParkingSystem::printMqttTopics() const {
  Serial.println("\n+---------------------------------------------------------------+");
  Serial.println("|                   CAU HINH MQTT DE TEST MQTTX                 |");
  Serial.println("+---------------------------------------------------------------+");
  Serial.print("  * Broker        : "); Serial.println(MQTT_BROKER);
  Serial.print("  * Port          : "); Serial.println(MQTT_PORT);
  Serial.print("  * Scan Topic    : "); Serial.println(MQTT_TOPIC_SCAN);
  Serial.print("  * Status Topic  : "); Serial.println(MQTT_TOPIC_BARIE_STAT);
  Serial.print("  * Cmd Topic     : "); Serial.println(MQTT_TOPIC_CMD);
  Serial.print("  * Alert Topic   : "); Serial.println(MQTT_TOPIC_ALERT);
  Serial.print("  * Env Topic     : "); Serial.println(MQTT_TOPIC_ENV);
  Serial.print("  * Fire Topic    : "); Serial.println(MQTT_TOPIC_FIRE);
  Serial.println("+---------------------------------------------------------------+\n");
}

void ParkingSystem::handleRemoteCommand(const String &cmd) {
  unsigned long now = millis();
  if (cmd == "fire_alarm" || cmd == "fire") {
    Serial.println("[COMMAND] Nhan lenh KICH HOAT BAO CHAY KHAN CAP tu Dashboard!");
    triggerFireEmergency(true);
  } else if (cmd == "fire_clear" || cmd == "nofire") {
    Serial.println("[COMMAND] Nhan lenh TAT BAO CHAY tu Dashboard!");
    triggerFireEmergency(false);
  } else if (cmd == "open_hold") {
    Serial.println("[COMMAND] Nhan lenh MO GIU BARIE (HOLD OPEN) de mo phong ket!");
    holdOpen = true;
    buzzer.beep(2, 80);
    display.showWarning("MO GIU BARIE", "MO PHONG KET");
    barrier.open();
    mqttMgr.publishBarrierStatus("open");
    currentState = STATE_OPENING;
    stateTimer = now;
  } else if (cmd == "open") {
    Serial.println("[COMMAND] Nhan lenh MO BARIE tu Dashboard / MQTT!");
    holdOpen = false;
    buzzer.beep(2, 80);
    display.showWarning("LENH DIEU KHIEN", "DANG MO BARIE");
    barrier.open();
    mqttMgr.publishBarrierStatus("open");
    currentState = STATE_OPENING;
    stateTimer = now;
  } else if (cmd == "close") {
    Serial.println("[COMMAND] Nhan lenh DONG BARIE tu Dashboard / MQTT!");
    holdOpen = false;
    buzzer.beep(1, 80);
    display.showClosingStatus();
    barrier.close();
    mqttMgr.publishBarrierStatus("closed");
    currentState = STATE_CLOSING;
    stateTimer = now;
  } else if (cmd == "reconnect" || cmd == "test") {
    Serial.println("[COMMAND] Nhan lenh TEST RECONNECT!");
    buzzer.beep(3, 50);
    display.showWarning("TEST RECONNECT", "DANG KET NOI LAI");
    mqttMgr.triggerReconnectTest();
    currentState = STATE_ALERT_HOLD;
    stateTimer = now;
  } else {
    Serial.print("[COMMAND] Lenh khong xac dinh: ");
    Serial.println(cmd);
  }
}

void ParkingSystem::handleRFIDScan() {
  if (currentState != STATE_IDLE && currentState != STATE_FIRE_EVACUATE) {
    return;
  }
  if (fireEmergency) {
    return; // Đang báo cháy khẩn cấp, Barie đã mở toang
  }

  String uid = "";
  if (!rfidReader.readCard(uid)) {
    return;
  }

  processCardScan(uid, false);
}

void ParkingSystem::processCardScan(const String &rawUid, bool bypassDebounce) {
  if (fireEmergency) {
    Serial.println("[SYSTEM] He thong dang trong che do BAO CHAY KHAN CAP!");
    return;
  }

  if (currentState != STATE_IDLE) {
    Serial.println("[SYSTEM] Barie/He thong dang ban xu ly, vui long doi...");
    return;
  }

  String uid = rawUid;
  uid.trim();
  uid.toUpperCase();

  unsigned long now = millis();

  // 1. Debounce check
  if (!bypassDebounce && !rfidReader.checkDebounce(uid, now, SCAN_DEBOUNCE_MS)) {
    Serial.println("[DEBOUNCE] The vua duoc quet! Vui long khong quet lien tuc.");
    return;
  }

  // 2. Tìm thẻ trong database
  int cardIdx = cardMgr.findCardIndex(uid);
  if (cardIdx == -1) {
    Serial.println("\n-------------------------------------------------------------");
    Serial.print("[CANH BAO] THE KHONG HOP LE! UID: [");
    Serial.print(uid);
    Serial.println("]");
    Serial.println("  -> Tu choi truy cap & Phat coi bao dong!");

    display.showWarning("THE KHONG HOP LE", "Tu choi truy cap");
    buzzer.playErrorAlert();

    // Nhấp nháy đèn Đỏ cảnh báo
    setTrafficLeds(true, false);

    // Publish sự kiện cảnh báo và thẻ không hợp lệ lên MQTT
    mqttMgr.publishAlert("INVALID_CARD", uid);
    mqttMgr.publishScanEvent(uid, "invalid", "N/A", "N/A", 0, 0, cardMgr.getParkedCount(), cardMgr.getTotalCards());

    currentState = STATE_ALERT_HOLD;
    stateTimer = now;
    return;
  }

  // Lấy thẻ chuẩn từ cơ sở dữ liệu
  const VehicleCard &matchedCard = cardMgr.getCard(cardIdx);
  String canonicalUid = matchedCard.uid;

  // 3. Xử lý nghiệp vụ Vào / Ra
  ScanResult res = cardMgr.processCard(cardIdx, now);
  const VehicleCard &card = cardMgr.getCard(cardIdx);
  logScanEvent(canonicalUid, res);

  if (res.type == SCAN_RESULT_CHECK_IN) {
    display.showCheckIn(card);
    buzzer.playSuccessCheckIn();
    mqttMgr.publishScanEvent(canonicalUid, "check_in", card.licensePlate, card.ownerName, 0, 0, cardMgr.getParkedCount(), cardMgr.getTotalCards());
  } else if (res.type == SCAN_RESULT_CHECK_OUT) {
    display.showCheckOut(card, res.durationSec, res.fee);
    buzzer.playSuccessCheckOut();
    mqttMgr.publishScanEvent(canonicalUid, "check_out", card.licensePlate, card.ownerName, res.durationSec, res.fee, cardMgr.getParkedCount(), cardMgr.getTotalCards());
  }

  // 4. Mở Barie và bật đèn Xanh
  Serial.println("[BARIE] Bat dau mo barie cho xe di qua...");
  holdOpen = false;
  barrier.open();
  setTrafficLeds(false, true); // Đèn XANH sáng
  mqttMgr.publishBarrierStatus("open");
  currentState = STATE_OPENING;
  stateTimer = now;
}

void ParkingSystem::updateStateMachine() {
  unsigned long now = millis();

  switch (currentState) {
    case STATE_IDLE:
      break;

    case STATE_OPENING:
      if (now - stateTimer >= BARRIER_MOVE_MS) {
        currentState = STATE_OPEN;
        stateTimer = now;
        Serial.println("[STATE] Barie da mo hoan toan. Dang giu mo...");
      }
      break;

    case STATE_OPEN:
      if (!holdOpen && !fireEmergency && now - stateTimer >= BARRIER_HOLD_MS) {
        // TÍNH NĂNG THÔNG MINH CHỐNG KẸP XE (ANTI-PINCH / ANTI-COLLISION):
        // Nếu cảm biến siêu âm phát hiện xe vẫn đang đi qua ngay dưới cần chắn (< 15cm):
        if (currentDistance > 0 && currentDistance < 15.0) {
          stateTimer = now; // Hoãn đóng barie, gia hạn thời gian mở an toàn
          Serial.printf("[ANTI-PINCH] Phat hien xe dang qua barie (dist=%.1fcm)! Hoan dong barie de chong kep xe!\n", currentDistance);
          display.showWarning("XE DANG DI QUA", "Giu mo barie...");
        } else {
          Serial.println("[STATE] Xe da qua an toan. Bat dau dong barie...");
          display.showClosingStatus();
          barrier.close();
          setTrafficLeds(true, false); // Đèn ĐỎ sáng
          mqttMgr.publishBarrierStatus("closed");
          currentState = STATE_CLOSING;
          stateTimer = now;
        }
      }
      break;

    case STATE_CLOSING:
      if (now - stateTimer >= BARRIER_MOVE_MS) {
        Serial.println("[STATE] Barie da dong xong. He thong tro ve IDLE.");
        currentState = STATE_IDLE;
        display.showHomeScreen(cardMgr.getParkedCount(), cardMgr.getTotalCards());
      }
      break;

    case STATE_ALERT_HOLD:
      if (now - stateTimer >= MSG_DISPLAY_MS) {
        currentState = STATE_IDLE;
        display.showHomeScreen(cardMgr.getParkedCount(), cardMgr.getTotalCards());
      }
      break;

    case STATE_FIRE_EVACUATE:
      // Giữ mở toang barie vĩnh viễn cho đến khi có lệnh nofire
      break;
  }
}

void ParkingSystem::logScanEvent(const String &uid, const ScanResult &result) {
  const VehicleCard &card = cardMgr.getCard(result.cardIndex);

  Serial.println("\n=============================================================");
  if (result.type == SCAN_RESULT_CHECK_IN) {
    Serial.println("              >>> XAC NHAN: XE VAO BAI <<<                   ");
    Serial.println("=============================================================");
    Serial.print("  - UID The    : "); Serial.println(uid);
    Serial.print("  - Chu xe     : "); Serial.println(card.ownerName);
    Serial.print("  - Bien so    : "); Serial.println(card.licensePlate);
    Serial.print("  - Thoi diem  : "); Serial.print(card.entryTime / 1000); Serial.println("s");
    Serial.print("  - Tong xe gui: "); Serial.print(cardMgr.getParkedCount()); 
    Serial.print(" / "); Serial.print(cardMgr.getTotalCards()); Serial.println(" xe");
  } else if (result.type == SCAN_RESULT_CHECK_OUT) {
    Serial.println("              <<< XAC NHAN: XE RA BAI <<<                   ");
    Serial.println("=============================================================");
    Serial.print("  - UID The    : "); Serial.println(uid);
    Serial.print("  - Chu xe     : "); Serial.println(card.ownerName);
    Serial.print("  - Bien so    : "); Serial.println(card.licensePlate);
    Serial.print("  - Thoi gian  : "); Serial.print(result.durationSec); Serial.println(" giay");
    Serial.print("  - Phi gui xe : "); Serial.print(result.fee); Serial.println(" VND");
    Serial.print("  - Tong xe gui: "); Serial.print(cardMgr.getParkedCount()); 
    Serial.print(" / "); Serial.print(cardMgr.getTotalCards()); Serial.println(" xe");
  }
  Serial.println("=============================================================");
}
