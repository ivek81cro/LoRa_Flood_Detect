#pragma once
#include <Adafruit_BMP280.h>

class BMP280Sensor {
public:
    BMP280Sensor();
    void begin();
    bool available() const;
    bool read(float& temp, float& press);
private:
    Adafruit_BMP280 bmp_;
    bool have_ = false;
};
