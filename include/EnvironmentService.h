#pragma once
#include <Arduino.h>
#include <DHT.h>

class EnvironmentService {
public:
    EnvironmentService(int dhtPin, float fireTempThreshold = 50.0);

    void begin();
    void update();

    float getTemperature() const;
    float getHumidity() const;
    bool isFireThresholdExceeded() const;

private:
    DHT _dht;
    float _fireTempThreshold;
    float _temperature;
    float _humidity;
    unsigned long _lastSampleTime;
};
