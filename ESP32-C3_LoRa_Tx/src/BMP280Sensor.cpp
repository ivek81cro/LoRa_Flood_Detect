#include "BMP280Sensor.h"

/**
 * Konstruktor BMP280 senzora.
 */
BMP280Sensor::BMP280Sensor() = default;

/**
 * Inicijalizacija BMP280 senzora.
 * Pokušava pronaći senzor na adresama 0x76 i 0x77 te postavlja način rada.
 */
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

/**
 * Provjerava je li BMP280 senzor uspješno inicijaliziran.
 * @return true ako je senzor pronađen, inače false.
 */
bool BMP280Sensor::available() const {
    return have_;
}

/**
 * Čita temperaturu i tlak s BMP280 senzora.
 * @param temp Referenca na varijablu za temperaturu.
 * @param press Referenca na varijablu za tlak.
 * @return true ako je čitanje uspješno, inače false.
 */
bool BMP280Sensor::read(float& temp, float& press) {
    if (!have_) return false;
    temp = bmp_.readTemperature();
    press = bmp_.readPressure() / 100.0f;
    return true;
}
