
#include "App.h"
#include "LoRaRadio.h"
#include <Arduino.h>

App::App() : water_(config::WATER_SIG_PIN, config::WATER_PWR_PIN), aht_(config::I2C_SDA, config::I2C_SCL) {}

void App::setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println(F("\nBoot: ESP32-C3 + AHT20 + BMP280 + Water + LoRa TX"));
    aht_.begin();
    Serial.println(aht_.available() ? F("AHT20 OK") : F("AHT20 not found"));
    bmp_.begin();
    Serial.println(bmp_.available() ? F("BMP280 OK") : F("BMP280 not found"));
    water_.begin();
    if (radio_.begin()) {
        Serial.println(F("LoRa OK"));
    } else {
        while (true) delay(1000);
    }
}

int App::smoothADC_(int sample) {
    // 1) ring-buffer zadnjih 5 "median-of-7" uzoraka
    if (adcHistLen_ < 5) {
        adcHist_[adcHistLen_++] = sample;
    } else {
        adcHist_[adcHistIdx_] = sample;
        adcHistIdx_ = (adcHistIdx_ + 1) % 5;
    }

    // 2) median preko vremenske povijesti (robustan na outliere)
    int tmp[5];
    for (uint8_t i = 0; i < adcHistLen_; ++i) tmp[i] = adcHist_[i];
    for (uint8_t i = 0; i < adcHistLen_; ++i)
      for (uint8_t j = i + 1; j < adcHistLen_; ++j)
        if (tmp[j] < tmp[i]) { int t = tmp[i]; tmp[i] = tmp[j]; tmp[j] = t; }
    int med = tmp[adcHistLen_ / 2];

    // 3) anti-spike clamp (limitiraj nagle skokove)
    const int SPIKE = 400; // ~15% tvog opsega (200..2800)
    if (emaADC_ >= 0) {
        int diff = med - emaADC_;
        if (diff >  SPIKE) med = emaADC_ + SPIKE;
        if (diff < -SPIKE) med = emaADC_ - SPIKE;
    }

    // 4) EMA low-pass (α = 0.25 => nova mjera 25%, povijest 75%)
    if (emaADC_ < 0) emaADC_ = med;
    else             emaADC_ = (emaADC_ * 3 + med) / 4;

    return emaADC_;
}

void App::loop() {
    unsigned long now = millis();
    float ahtT = NAN, ahtH = NAN, bmpT = NAN, bmpP = NAN;
    int waterADC = 0, waterPct = 0;
    if (aht_.read(ahtT, ahtH)) {
        last_ahtT_ = ahtT;
        last_ahtH_ = ahtH;
    }
    if (bmp_.read(bmpT, bmpP)) {
        last_bmpT_ = bmpT;
        last_bmpP_ = bmpP;
    }
    waterADC = smoothADC_(water_.readMedian());
    waterPct = water_.percent(waterADC);
    last_waterADC_ = waterADC;
    last_waterPct_ = waterPct;

    if (now - lastLoRaSend_ >= loraInterval_) {
        lastLoRaSend_ = now;
        String payload = "{";
        payload += "\"c\":" + String(counter_);
        if (!isnan(last_ahtT_)) payload += ",\"t\":" + String(last_ahtT_,1);
        if (!isnan(last_ahtH_)) payload += ",\"h\":" + String(last_ahtH_,1);
        if (!isnan(last_bmpT_)) payload += ",\"tp\":" + String(last_bmpT_,1);
        if (!isnan(last_bmpP_)) payload += ",\"p\":" + String(last_bmpP_,1);
        payload += ",\"w\":" + String(last_waterPct_);
        payload += ",\"a\":" + String(last_waterADC_);
        payload += "}";

        Serial.print(F("TX payload: "));
        Serial.println(payload);
        Serial.print("AHT20 Temp: "); Serial.print(last_ahtT_,1); Serial.print(" °C, ");
        Serial.print("Hum: "); Serial.print(last_ahtH_,1); Serial.print(" %, ");
        Serial.print("BMP280 Temp: "); Serial.print(last_bmpT_,1); Serial.print(" °C, ");
        Serial.print("Press: "); Serial.print(last_bmpP_,1); Serial.print(" hPa, ");
        Serial.print("Water: "); Serial.print(last_waterPct_); Serial.print(" % (ADC=");
        Serial.print(last_waterADC_); Serial.println(")");

        bool sent = radio_.send(payload);
        if (sent) {
            Serial.println(F("LoRa sent OK"));
            double toa_ms = LoRaRadio::calcToA(payload.length());
            loraInterval_ = config::ENFORCE_1_PERCENT_DUTY
                ? (unsigned long)max(1000.0, toa_ms * 99.0)
                : 10000UL;
            Serial.print(F("ToA(ms): ")); Serial.print(toa_ms,2);
            Serial.print(F(" -> Next TX in(ms): ")); Serial.println(loraInterval_);
        } else {
            Serial.println(F("LoRa send FAILED"));
            loraInterval_ = 2000;
        }
        ++counter_;
    }
}
