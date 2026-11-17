#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"
#include "types.h"

class Display {
public:
  Display(uint8_t addr = LCD_I2C_ADDR, uint8_t cols = LCD_COLS, uint8_t rows = LCD_ROWS);

  void init();
  void showHomeScreen(int parkedCount, int totalSlots);
  void showCheckIn(const VehicleCard &card);
  void showCheckOut(const VehicleCard &card, unsigned long durationSec, uint32_t fee);
  void showWarning(const String &line1, const String &line2);
  void showClosingStatus();

private:
  LiquidCrystal_I2C lcd;
};
