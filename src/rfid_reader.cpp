#include "rfid_reader.h"

RFIDReader::RFIDReader(byte ssPin, byte rstPin) 
  : mfrc522(ssPin, rstPin), lastUID(""), lastReadTime(0) {}

void RFIDReader::init() {
  SPI.begin();
  mfrc522.PCD_Init();
}

bool RFIDReader::readCard(String &uidOut) {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();
  uidOut = uid;

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  return true;
}

bool RFIDReader::checkDebounce(const String &uid, unsigned long currentTime, unsigned long debounceMs) {
  if (uid == lastUID && (currentTime - lastReadTime < debounceMs)) {
    return false; // Bị trùng trong khoảng debounce
  }
  lastUID = uid;
  lastReadTime = currentTime;
  return true; // Hợp lệ
}
