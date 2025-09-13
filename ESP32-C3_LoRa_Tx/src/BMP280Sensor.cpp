#include "BMP280Sensor.h"

BMP280Sensor::BMP280Sensor() = default;

void BMP280Sensor::begin() {
    have_ = bmp_.begin(0x76) || bmp_.begin(0x77);
    if (have_) {
        bmp_.setSampling(
            Adafruit_BMP280::MODE_NORMAL,
            Adafruit_BMP280::SAMPLING_X2,
            Adafruit_BMP280::SAMPLING_X16,
            Adafruit_BMP280::FILTER_X16,
            Adafruit_BMP280::STANDBY_MS_63
        );
    }
}

bool BMP280Sensor::available() const {
    return have_;
}

bool BMP280Sensor::read(float& temp, float& press) {
    if (!have_) return false;
    temp = bmp_.readTemperature();
    press = bmp_.readPressure() / 100.0f;
    return true;
}
