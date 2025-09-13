/*********
  ESP32-C3 SuperMini: AHT20 + BMP280 + Water sensor + LoRa TX (Ra-02 / SX1278)
  - HR/EU 433 MHz ISM (433.375 MHz)
  - Power-gated water senzor preko PNP tranzistora (GPIO4: HIGH=OFF, LOW=ON)
  - 1% duty-cycle helper i ToA procjena
  - Dodani periodični Serial ispisi pri svakoj TX poruci
*********/


#include "App.h"

App app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}
