#pragma once
#include "AHT20Sensor.h"
#include "BMP280Sensor.h"
#include "WaterSensor.h"
#include "LoRaRadio.h"

class App {
public:
    App();
    void setup();
    void loop();
private:
    WaterSensor water_;
    AHT20Sensor aht_;
    BMP280Sensor bmp_;
    LoRaRadio radio_;
    uint32_t counter_ = 0;
    float last_ahtT_ = NAN, last_ahtH_ = NAN, last_bmpT_ = NAN, last_bmpP_ = NAN;
    int last_waterADC_ = 0, last_waterPct_ = 0;
    unsigned long lastLoRaSend_ = 0;
    unsigned long loraInterval_ = 10000;
};
