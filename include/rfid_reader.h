#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "config.h"

class RFIDReader {
public:
  RFIDReader(byte ssPin = PIN_SS, byte rstPin = PIN_RST);
  
  void init();
  bool readCard(String &uidOut);
  bool checkDebounce(const String &uid, unsigned long currentTime, unsigned long debounceMs = SCAN_DEBOUNCE_MS);

private:
  MFRC522 mfrc522;
  String lastUID;
  unsigned long lastReadTime;
};
