#include "display.h"

Display::Display(uint8_t addr, uint8_t cols, uint8_t rows) : lcd(addr, cols, rows) {}

void Display::init() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  lcd.init();
  lcd.backlight();
}

void Display::showHomeScreen(int parkedCount, int totalSlots) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BAI XE: ");
  lcd.print(parkedCount);
  lcd.print("/");
  lcd.print(totalSlots);
  lcd.print(" XE");

  lcd.setCursor(0, 1);
  lcd.print("Moi quet the...");
}

void Display::showCheckIn(const VehicleCard &card) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("VAO: ");
  lcd.print(card.licensePlate);

  lcd.setCursor(0, 1);
  lcd.print("Moi vao! -> MO");
}

void Display::showCheckOut(const VehicleCard &card, unsigned long durationSec, uint32_t fee) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RA: ");
  lcd.print(card.licensePlate);

  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print(durationSec);
  lcd.print("s ");
  lcd.print(fee);
  lcd.print("d");
}

void Display::showWarning(const String &line1, const String &line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void Display::showClosingStatus() {
  lcd.setCursor(0, 1);
  lcd.print("Dang dong cong..");
}
