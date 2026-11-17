#pragma once
#include <Arduino.h>
#include "types.h"
#include "config.h"

class CardManager {
public:
  CardManager();

  int findCardIndex(const String &uid) const;
  bool isCardValid(const String &uid) const;
  
  VehicleCard& getCard(int index);
  const VehicleCard& getCard(int index) const;
  int getTotalCards() const;
  int getParkedCount() const;

  ScanResult processCard(int cardIndex, unsigned long currentTime);
  void printDatabase() const;
  void resetAllCards();

  static String normalizeUID(const String &uid);

private:
  static const int MAX_CARDS = 8;
  VehicleCard database[MAX_CARDS];
};
