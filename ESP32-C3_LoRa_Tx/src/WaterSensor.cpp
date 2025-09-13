#include "WaterSensor.h"
#include <Arduino.h>

WaterSensor::WaterSensor(int sigPin, int pwrPin) : sigPin_(sigPin), pwrPin_(pwrPin) {}

void WaterSensor::begin() const {
    pinMode(pwrPin_, OUTPUT);
    digitalWrite(pwrPin_, HIGH); // OFF
    pinMode(sigPin_, INPUT);
    analogReadResolution(12);
}

int WaterSensor::readRawOnce() const {
    digitalWrite(pwrPin_, LOW); // ON
    delay(50);
    int raw = analogRead(sigPin_);
    digitalWrite(pwrPin_, HIGH); // OFF
    return raw;
}

int WaterSensor::readMedian() const {
    constexpr int N = 7;
    int v[N];

    // Uključi senzor samo jednom
    digitalWrite(pwrPin_, LOW);   // ON (PNP high-side)
    delay(50);                    // 30–100 ms, po želji

    for (int i = 0; i < N; ++i) {
        v[i] = analogRead(sigPin_);
        delay(2);
    }

    digitalWrite(pwrPin_, HIGH);  // OFF

    // selection sort za N=7 je sasvim ok
    for (int i = 0; i < N; ++i)
      for (int j = i + 1; j < N; ++j)
        if (v[j] < v[i]) { int t=v[i]; v[i]=v[j]; v[j]=t; }

    return v[N/2];
}

int WaterSensor::percent(int adc) const {
    constexpr int dry = 200;
    constexpr int wet = 2800;
    int p = (adc - dry) * 100 / (wet - dry);
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    return p;
}
