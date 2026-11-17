#include "barrier.h"

Barrier::Barrier(int pin) : servoPin(pin), isBarrierOpen(false) {}

void Barrier::init() {
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  servo.setPeriodHertz(50);
  servo.attach(servoPin, 500, 2400);
  close();
}

void Barrier::open() {
  servo.write(SERVO_OPEN_ANGLE);
  isBarrierOpen = true;
}

void Barrier::close() {
  servo.write(SERVO_CLOSE_ANGLE);
  isBarrierOpen = false;
}

bool Barrier::isOpen() const {
  return isBarrierOpen;
}
