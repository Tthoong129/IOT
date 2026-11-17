#pragma once
#include <Arduino.h>
#include <DHT.h>
#include "config.h"
#include "types.h"
#include "card_manager.h"
#include "rfid_reader.h"
#include "barrier.h"
#include "buzzer.h"
#include "display.h"
#include "mqtt_manager.h"

class ParkingSystem {
public:
  ParkingSystem();

  void init();
  void update();
  void handleRemoteCommand(const String &cmd);
  void handleSerialCommand(String input);
  void processCardScan(const String &uid, bool bypassDebounce = false);

  void printHelpMenu() const;
  void printSystemStatus() const;
  void printMqttTopics() const;

  void triggerFireEmergency(bool enable);

private:
  void handleRFIDScan();
  void updateStateMachine();
  void logScanEvent(const String &uid, const ScanResult &result);

  void initSensorsAndLeds();
  void updateSensorsAndLeds();
  float readDistanceCm();
  void setTrafficLeds(bool redOn, bool greenOn);

  CardManager cardMgr;
  RFIDReader rfidReader;
  Barrier barrier;
  Buzzer buzzer;
  Display display;
  MqttManager mqttMgr;
  DHT dht;

  SystemState currentState;
  unsigned long stateTimer;
  bool holdOpen;

  float currentTemp;
  float currentHumidity;
  float currentDistance;
  bool vehicleApproaching;
  bool fireEmergency;

  unsigned long lastSensorPoll;
  unsigned long lastEnvMqttPoll;
  unsigned long lastLedBlink;
  bool ledBlinkState;
};
