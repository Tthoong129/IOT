#include "EnvironmentService.h"

EnvironmentService::EnvironmentService(int dhtPin, float fireTempThreshold)
    : _dht(dhtPin, DHT22), _fireTempThreshold(fireTempThreshold),
      _temperature(28.0), _humidity(65.0), _lastSampleTime(0) {}

void EnvironmentService::begin() {
    _dht.begin();
}

void EnvironmentService::update() {
    unsigned long now = millis();
    if (now - _lastSampleTime >= 1000) {
        _lastSampleTime = now;
        float t = _dht.readTemperature();
        float h = _dht.readHumidity();
        if (!isnan(t) && !isnan(h)) {
            _temperature = t;
            _humidity = h;
        }
    }
}

float EnvironmentService::getTemperature() const {
    return _temperature;
}

float EnvironmentService::getHumidity() const {
    return _humidity;
}

bool EnvironmentService::isFireThresholdExceeded() const {
    return _temperature >= _fireTempThreshold;
}
