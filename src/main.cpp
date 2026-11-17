#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Config.h"
#include "GateController.h"
#include "EnvironmentService.h"
#include "ParkingSystem.h"
#include "MqttService.h"

// Subsystem instances
GateController gate(PIN_SERVO, PIN_TRIG, PIN_ECHO, PIN_LED_RED, PIN_LED_GREEN);
EnvironmentService env(PIN_DHT);
ParkingSystem parking(LCD_I2C_ADDR, PIN_BUZZER);
MqttService mqtt(WIFI_SSID, WIFI_PASS, MQTT_BROKER, MQTT_PORT);
MFRC522 rfid(PIN_SS, PIN_RST);

// State tracking
unsigned long lastEnvPublish = 0;
unsigned long lastDebounceTime = 0;
String lastScannedUid = "";
bool fireMode = false;

void handleCardScan(const String& uid) {
    if (fireMode || gate.isOpen()) return;

    unsigned long now = millis();
    if (uid == lastScannedUid && (now - lastDebounceTime < 3000)) return;
    lastScannedUid = uid;
    lastDebounceTime = now;

    CardScanEvent event = parking.processCard(uid);

    switch (event.result) {
        case CARD_CHECK_IN:
            Serial.printf("[IN] Plate: %s | Owner: %s\n", event.card.licensePlate.c_str(), event.card.ownerName.c_str());
            parking.showCheckIn(event.card.licensePlate);
            parking.beep(1, 150);
            gate.open(false);
            mqtt.publishGateState(true);
            mqtt.publishScan(event.card.uid, "check_in", event.card.licensePlate, event.card.ownerName,
                             0, 0, parking.getOccupiedCount(), parking.getMaxCapacity());
            break;

        case CARD_CHECK_OUT:
            Serial.printf("[OUT] Plate: %s | Duration: %lus | Fee: %u VND\n",
                          event.card.licensePlate.c_str(), event.durationSec, event.fee);
            parking.showCheckOut(event.card.licensePlate, event.durationSec, event.fee);
            parking.beep(2, 100);
            gate.open(false);
            mqtt.publishGateState(true);
            mqtt.publishScan(event.card.uid, "check_out", event.card.licensePlate, event.card.ownerName,
                             event.durationSec, event.fee, parking.getOccupiedCount(), parking.getMaxCapacity());
            break;

        case CARD_PARKING_FULL:
            Serial.println("[WARN] Parking lot is full.");
            parking.showLotFull();
            parking.beep(2, 200);
            delay(1500);
            parking.showHome();
            break;

        case CARD_NOT_FOUND:
            Serial.printf("[SECURITY] Invalid card scanned: %s\n", uid.c_str());
            parking.showDenied();
            parking.beep(3, 120);
            mqtt.publishAlert("INVALID_CARD", uid);
            mqtt.publishScan(uid, "invalid", "N/A", "Unknown", 0, 0,
                             parking.getOccupiedCount(), parking.getMaxCapacity());
            delay(1500);
            parking.showHome();
            break;
    }
}

void triggerFireEvacuation(bool enable) {
    fireMode = enable;
    gate.setEmergency(enable);
    mqtt.publishFire(enable);
    mqtt.publishGateState(enable);

    if (enable) {
        Serial.println("[FIRE] Emergency evacuation active. Gate held open.");
        parking.showFireAlarm();
        parking.beep(5, 150);
    } else {
        Serial.println("[FIRE] Fire alarm cleared. System returning to normal.");
        parking.showHome();
    }
}

void onCommandReceived(const String& cmd) {
    Serial.printf("[CMD] Received: %s\n", cmd.c_str());

    if (cmd == "open") {
        gate.open(false);
        mqtt.publishGateState(true);
    } else if (cmd == "open_hold") {
        gate.open(true);
        mqtt.publishGateState(true);
    } else if (cmd == "close") {
        gate.close();
        mqtt.publishGateState(false);
        parking.showHome();
    } else if (cmd == "fire" || cmd == "fire_alarm") {
        triggerFireEvacuation(true);
    } else if (cmd == "nofire" || cmd == "fire_clear") {
        triggerFireEvacuation(false);
    }
}

void handleSerialInput() {
    if (!Serial.available()) return;
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.isEmpty()) return;

    if (line == "1") handleCardScan("04 3B 2A 1C");
    else if (line == "2") handleCardScan("14 8C 3D 2E");
    else if (line == "3") handleCardScan("24 D5 4E 3F");
    else if (line == "4") handleCardScan("34 E6 5F 4A");
    else if (line.startsWith("scan ")) handleCardScan(line.substring(5));
    else if (line == "open") onCommandReceived("open");
    else if (line == "close") onCommandReceived("close");
    else if (line == "fire") triggerFireEvacuation(true);
    else if (line == "nofire") triggerFireEvacuation(false);
    else if (line == "sonar") Serial.printf("[SONAR] Distance: %.1f cm\n", gate.readObstacleDistanceCm());
    else if (line == "env") Serial.printf("[ENV] Temp: %.1f C | Hum: %.1f %%\n", env.getTemperature(), env.getHumidity());
    else if (line == "help" || line == "?") {
        Serial.println("Commands: 1..4, scan <uid>, open, close, fire, nofire, sonar, env");
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("[BOOT] Initializing Smart Parking Subsystems...");

    gate.begin();
    env.begin();
    parking.begin();

    SPI.begin();
    rfid.PCD_Init();

    mqtt.begin(onCommandReceived);

    parking.beep(2, 60);
    Serial.println("[BOOT] System ready.");
}

void loop() {
    unsigned long now = millis();

    // 1. Service updates
    mqtt.update();
    env.update();
    gate.update();

    // 2. Automated fire detection from DHT22
    if (!fireMode && env.isFireThresholdExceeded()) {
        Serial.printf("[ALERT] High temperature detected: %.1f C\n", env.getTemperature());
        triggerFireEvacuation(true);
    }

    // 3. Periodic telemetry publish every 5 seconds
    if (now - lastEnvPublish >= 5000) {
        lastEnvPublish = now;
        String risk = fireMode ? "DANGER_FIRE" : (env.getTemperature() >= 38.0 ? "WARNING_HIGH_TEMP" : "NORMAL");
        mqtt.publishEnvironment(env.getTemperature(), env.getHumidity(), risk);
    }

    // 4. Physical RFID scanning
    if (!fireMode && !gate.isOpen()) {
        if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
            String uid = "";
            for (byte i = 0; i < rfid.uid.size; i++) {
                if (rfid.uid.uidByte[i] < 0x10) uid += "0";
                uid += String(rfid.uid.uidByte[i], HEX);
                if (i < rfid.uid.size - 1) uid += " ";
            }
            uid.toUpperCase();
            handleCardScan(uid);
            rfid.PICC_HaltA();
            rfid.PCD_StopCrypto1();
        }
    }

    // 5. Console debug commands
    handleSerialInput();
}
