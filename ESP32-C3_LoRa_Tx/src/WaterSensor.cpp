#include "WaterSensor.h"
#include <Arduino.h>

#include "WaterSensor.h"
#include <Arduino.h>

/**
 * Konstruktor klase WaterSensor.
 * @param sigPin Pin za analogni signal senzora.
 * @param pwrPin Pin za upravljanje napajanjem senzora (PNP tranzistor).
 */
WaterSensor::WaterSensor(int sigPin, int pwrPin) : sigPin_(sigPin), pwrPin_(pwrPin) {}

/**
 * Inicijalizacija pinova za senzor vode.
 * Postavlja pinove i rezoluciju ADC-a.
 */
void WaterSensor::begin() const {
    pinMode(pwrPin_, OUTPUT);
    digitalWrite(pwrPin_, HIGH); // OFF
    pinMode(sigPin_, INPUT);
    analogReadResolution(12);
}

/**
 * Jednokratno čitanje sirove vrijednosti ADC-a s vodnog senzora.
 * Uključuje napajanje, čeka stabilizaciju, očitava i isključuje napajanje.
 * @return Sirova ADC vrijednost.
 */
int WaterSensor::readRawOnce() const {
    digitalWrite(pwrPin_, LOW); // ON
    delay(50);
    int raw = analogRead(sigPin_);
    digitalWrite(pwrPin_, HIGH); // OFF
    return raw;
}

/**
 * Vraća medijan od 7 uzoraka ADC-a za robusno mjerenje vlage.
 * Senzor se uključi jednom, uzima 7 uzoraka, sortira i vraća medijan.
 * @return Medijan ADC vrijednosti.
 */
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

/**
 * Pretvara ADC vrijednost u nivo vode na senzoru.
 * Kalibracija: dry=200, wet=2800.
 * @param adc Sirova ADC vrijednost.
 * @return Postotak vlage (0-100%).
 */
int WaterSensor::percent(int adc) const {
    constexpr int dry = 200;
    constexpr int wet = 2800;
    int p = (adc - dry) * 100 / (wet - dry);
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    return p;
}
