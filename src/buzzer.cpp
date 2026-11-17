#include "buzzer.h"

Buzzer::Buzzer(int pin) : buzzerPin(pin) {}

void Buzzer::init() {
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
}

void Buzzer::beep(int count, int durationMs, int pauseMs) {
  for (int i = 0; i < count; i++) {
    long pulses = (long)1500 * durationMs / 1000;
    for (long j = 0; j < pulses; j++) {
      digitalWrite(buzzerPin, HIGH);
      delayMicroseconds(333);
      digitalWrite(buzzerPin, LOW);
      delayMicroseconds(333);
    }
    if (count > 1 && i < count - 1) {
      delay(pauseMs);
    }
  }
}

void Buzzer::playSuccessCheckIn() {
  beep(1, 150);
}

void Buzzer::playSuccessCheckOut() {
  beep(2, 100, 60);
}

void Buzzer::playErrorAlert() {
  beep(3, 150, 80);
}

void Buzzer::playWarning() {
  beep(2, 80, 50);
}
