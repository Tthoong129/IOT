#include <Arduino.h>
#include "parking_system.h"

ParkingSystem systemApp;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=============================================================");
  Serial.println(">>> HE THONG BAI GIU XE ESP32 - WOKWI SERIAL CONNECTED <<<");
  Serial.println("=============================================================");
  Serial.flush();
  
  systemApp.init();
}

void loop() {
  systemApp.update();
}

