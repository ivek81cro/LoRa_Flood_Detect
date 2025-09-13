#include "AHT20Sensor.h"

/**
 * Konstruktor AHT20 senzora.
 * @param sda Pin za SDA liniju I2C-a.
 * @param scl Pin za SCL liniju I2C-a.
 */
AHT20Sensor::AHT20Sensor(int sda, int scl) : sda_(sda), scl_(scl) {}

/**
 * Inicijalizacija AHT20 senzora na zadanim I2C pinovima.
 */
void AHT20Sensor::begin() {
    Wire.begin(sda_, scl_);
    have_ = aht_.begin(&Wire);
}

/**
 * Provjerava je li AHT20 senzor uspješno inicijaliziran.
 * @return true ako je senzor pronađen, inače false.
 */
bool AHT20Sensor::available() const {
    return have_;
}

/**
 * Čita temperaturu i vlagu s AHT20 senzora.
 * @param temp Referenca na varijablu za temperaturu.
 * @param hum Referenca na varijablu za vlagu.
 * @return true ako je čitanje uspješno, inače false.
 */
bool AHT20Sensor::read(float& temp, float& hum) {
    if (!have_) return false;
    sensors_event_t hum_ev, temp_ev;
    if (aht_.getEvent(&hum_ev, &temp_ev)) {
        temp = temp_ev.temperature;
        hum = hum_ev.relative_humidity;
        return true;
    }
    return false;
}
