#include "card_manager.h"

CardManager::CardManager() {
  database[0] = {"01 02 03 04", "29A-123.45", "Nguyen Van A", false, 0};
  database[1] = {"A3 5C 12 F0", "51F-888.88", "Tran Thi B",   false, 0};
  database[2] = {"B4 22 9A 01", "30E-999.99", "Le Van C",     false, 0};
  database[3] = {"DE AD BE EF", "43A-678.90", "Pham Thi D",   false, 0};
  database[4] = {"11 22 33 44", "60B-112.23", "Hoang Van E",  false, 0};
  database[5] = {"AA BB CC DD", "72C-445.56", "Vo Thi F",     false, 0};
  database[6] = {"12 34 56 78", "99A-333.33", "Do Van G",     false, 0};
  database[7] = {"55 66 77 88", "37B-777.77", "Bui Thi H",    false, 0};
}

String CardManager::normalizeUID(const String &uid) {
  String clean = "";
  for (unsigned int i = 0; i < uid.length(); i++) {
    char c = toupper(uid[i]);
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
      clean += c;
    }
  }
  return clean;
}

int CardManager::findCardIndex(const String &uid) const {
  String normTarget = normalizeUID(uid);
  if (normTarget.length() == 0) return -1;

  for (int i = 0; i < MAX_CARDS; i++) {
    if (normTarget == normalizeUID(database[i].uid)) {
      return i;
    }
  }
  return -1;
}

bool CardManager::isCardValid(const String &uid) const {
  return findCardIndex(uid) != -1;
}

VehicleCard& CardManager::getCard(int index) {
  return database[index];
}

const VehicleCard& CardManager::getCard(int index) const {
  return database[index];
}

int CardManager::getTotalCards() const {
  return MAX_CARDS;
}

int CardManager::getParkedCount() const {
  int count = 0;
  for (int i = 0; i < MAX_CARDS; i++) {
    if (database[i].isParked) count++;
  }
  return count;
}

ScanResult CardManager::processCard(int cardIndex, unsigned long currentTime) {
  ScanResult res;
  res.cardIndex = cardIndex;

  if (cardIndex < 0 || cardIndex >= MAX_CARDS) {
    res.type = SCAN_RESULT_INVALID;
    return res;
  }

  VehicleCard &card = database[cardIndex];

  if (!card.isParked) {
    // VÀO BÃI
    card.isParked = true;
    card.entryTime = currentTime;
    res.type = SCAN_RESULT_CHECK_IN;
    res.durationSec = 0;
    res.fee = 0;
  } else {
    // RA BÃI
    unsigned long durationMs = currentTime - card.entryTime;
    res.durationSec = durationMs / 1000;
    res.fee = BASE_PARKING_FEE + (res.durationSec / 10) * HOURLY_PARKING_FEE;
    card.isParked = false;
    res.type = SCAN_RESULT_CHECK_OUT;
  }

  return res;
}

void CardManager::resetAllCards() {
  for (int i = 0; i < MAX_CARDS; i++) {
    database[i].isParked = false;
    database[i].entryTime = 0;
  }
}

void CardManager::printDatabase() const {
  Serial.println("\n+----+-------------+-------------+------------------+-----------+---------------+");
  Serial.println("| STT|     UID     |   BIEN SO   |     CHU XE       | TRANG THAI| THOI GIAN VAO |");
  Serial.println("+----+-------------+-------------+------------------+-----------+---------------+");
  for (int i = 0; i < MAX_CARDS; i++) {
    char timeStr[24];
    if (database[i].isParked) {
      snprintf(timeStr, sizeof(timeStr), "%lus", database[i].entryTime / 1000);
    } else {
      snprintf(timeStr, sizeof(timeStr), "-");
    }

    char buf[128];
    snprintf(buf, sizeof(buf), "| %02d | %-11s | %-11s | %-16s | %-9s | %-13s |",
            i + 1,
            database[i].uid.c_str(),
            database[i].licensePlate.c_str(),
            database[i].ownerName.c_str(),
            database[i].isParked ? "TRONG BAI" : "NGOAI",
            timeStr);
    Serial.println(buf);
  }
  Serial.println("+----+-------------+-------------+------------------+-----------+---------------+\n");
}

