#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// --- Pinovi (ESP32 DevKit v1 + Ra-02) ---
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  2
#define LED_PIN    4   // LED na GPIO4

// --- Radio postavke (mora biti isto kao na TX) ---
const long  FREQ_HZ   = 433375000L;
const int   SF        = 9;
const long  BW        = 125E3;
const int   CR        = 5;
const byte  SYNC_WORD = 0x12;
const bool  USE_CRC   = true;

// stanje LED logike
bool waterAlarm = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("LoRa Receiver (ESP32 DevKit v1 + Ra-02)"));

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  // Inicijalizacija
  while (!LoRa.begin(FREQ_HZ)) {
    Serial.println(F("LoRa init retry..."));
    delay(500);
  }

  LoRa.setSpreadingFactor(SF);
  LoRa.setSignalBandwidth(BW);
  LoRa.setCodingRate4(CR);
  if (USE_CRC) LoRa.enableCrc(); else LoRa.disableCrc();
  LoRa.setSyncWord(SYNC_WORD);

  Serial.println(F("LoRa Initializing OK!"));
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) {
    // održava LED prema stanju alarm flag-a
    digitalWrite(LED_PIN, waterAlarm ? HIGH : LOW);
    return;
  }

  String msg;
  while (LoRa.available()) {
    msg += (char)LoRa.read();
  }

  long rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();

  Serial.println(F("---- PACKET ----"));
  Serial.print(F("RAW: ")); Serial.println(msg);
  Serial.print(F("RSSI: ")); Serial.print(rssi);
  Serial.print(F(" dBm | SNR: ")); Serial.print(snr, 1); Serial.println(F(" dB"));

  // JSON parsiranje
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, msg);

  if (!err) {
    if (doc["w"].is<int>()) {
      int water = doc["w"].as<int>();
      Serial.print(F("Water Level : ")); Serial.print(water); Serial.println(F(" %"));

      // logika alarma
      if (water > 20) {
        waterAlarm = true;  // upali i drži upaljeno
      } else if (water == 0) {
        waterAlarm = false; // ugasi kad padne na 0
      }
    }
  } else {
    Serial.print(F("JSON parse error: "));
    Serial.println(err.f_str());
  }

  // primjeni stanje na LED
  digitalWrite(LED_PIN, waterAlarm ? HIGH : LOW);

  Serial.println(F("-----------------"));
}
