#pragma once

class WaterSensor {
public:
    WaterSensor(int sigPin, int pwrPin);
    void begin() const;
    int readRawOnce() const;
    int readMedian() const;
    int percent(int adc) const;
private:
    int sigPin_;
    int pwrPin_;
};
