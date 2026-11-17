#pragma once
#include <Arduino.h>
#include "config.h"

class Buzzer {
public:
  Buzzer(int pin = PIN_BUZZER);

  void init();
  void beep(int count = 1, int durationMs = 100, int pauseMs = 80);
  void playSuccessCheckIn();
  void playSuccessCheckOut();
  void playErrorAlert();
  void playWarning();

private:
  int buzzerPin;
};
