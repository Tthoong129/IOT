#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

#define PIN_SS     5
#define PIN_RST    4
#define PIN_SERVO  13
#define PIN_BUZZER 14

MFRC522 mfrc522(PIN_SS, PIN_RST);
Servo barie;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Danh sach the xe hop le
const String THE_HOP_LE[] = {
  "01 02 03 04",
  "DE AD BE EF"
};
const int SO_LUONG_THE = sizeof(THE_HOP_LE) / sizeof(THE_HOP_LE[0]);

void keuCoi(int soLan, int thoiGianMs) {
  for (int i = 0; i < soLan; i++) {
    long soXung = (long)1500 * thoiGianMs / 1000;
    for (long j = 0; j < soXung; j++) {
      digitalWrite(PIN_BUZZER, HIGH);
      delayMicroseconds(333);
      digitalWrite(PIN_BUZZER, LOW);
      delayMicroseconds(333);
    }
    if (soLan > 1)
      delay(100);
  }
}

void moBarie() {
  barie.write(90);
  keuCoi(1, 150);
}

void dongBarie() {
  barie.write(0);
}

void manHinhChinh() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BAI GIU XE ESP32");
  lcd.setCursor(0, 1);
  lcd.print("Moi quet the...");
}

bool kiemTraThe(String uid) {
  for (int i = 0; i < SO_LUONG_THE; i++) {
    if (uid.equalsIgnoreCase(THE_HOP_LE[i])) {
      return true;
    }
  }
  return false;
}

String layMaUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();
  return uid;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  manHinhChinh();

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  barie.setPeriodHertz(50);
  barie.attach(PIN_SERVO, 500, 2400);
  dongBarie();

  SPI.begin();
  mfrc522.PCD_Init();
}

void loop() {
  // Kiem tra co the quet vao khong
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  String uid = layMaUID();
  Serial.print("Quet the: ");
  Serial.println(uid);

  if (kiemTraThe(uid)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("THE HOP LE");
    lcd.setCursor(0, 1);
    lcd.print("Moi xe vao ->");

    moBarie();
    delay(3000); // Cho xe chay qua

    lcd.setCursor(0, 1);
    lcd.print("Dang dong cong..");
    dongBarie();
    delay(500);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("THE KHONG DUNG");
    lcd.setCursor(0, 1);
    lcd.print("Khong the vao!");

    keuCoi(3, 120);
    delay(1500);
  }

  manHinhChinh();

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
