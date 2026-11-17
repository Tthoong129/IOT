#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "config.h"

class Barrier {
public:
  Barrier(int pin = PIN_SERVO);

  void init();
  void open();
  void close();
  bool isOpen() const;

private:
  int servoPin;
  Servo servo;
  bool isBarrierOpen;
};
