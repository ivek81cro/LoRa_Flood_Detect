#include "AHT20Sensor.h"

AHT20Sensor::AHT20Sensor(int sda, int scl) : sda_(sda), scl_(scl) {}

void AHT20Sensor::begin() {
    Wire.begin(sda_, scl_);
    have_ = aht_.begin(&Wire);
}

bool AHT20Sensor::available() const {
    return have_;
}

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
