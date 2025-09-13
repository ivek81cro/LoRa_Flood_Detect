#pragma once
#include <Wire.h>
#include <Adafruit_AHTX0.h>

class AHT20Sensor {
public:
    explicit AHT20Sensor(int sda, int scl);
    void begin();
    bool available() const;
    bool read(float& temp, float& hum);
private:
    int sda_, scl_;
    Adafruit_AHTX0 aht_;
    bool have_ = false;
};
