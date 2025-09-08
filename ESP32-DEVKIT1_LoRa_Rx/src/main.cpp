#include <SPI.h>
#include <LoRa.h>

// --- Pinovi (prilagodi ako treba) ---
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  2
#define LED_PIN    4   // LED na GPIO4

// --- Radio postavke (uskladi sa senderom) ---
const long  FREQ_HZ   = 433375000L; // 433.375 MHz (izbjegava 433.92)
const int   SF        = 9;          // SF9 "Balanced"
const long  BW        = 125E3;      // 125 kHz
const int   CR        = 5;          // coding rate 4/5
const byte  SYNC_WORD = 0x12;       // isti kao na senderu!
const bool  USE_CRC   = true;

void blinkLed3x() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH); delay(150);
    digitalWrite(LED_PIN, LOW);  delay(150);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("LoRa Receiver (HR 433 MHz)"));

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  // Inicijalizacija na točnoj frekvenciji
  while (!LoRa.begin(FREQ_HZ)) {
    Serial.println(F("LoRa init retry..."));
    delay(500);
  }

  // Uskladi PHY parametre sa senderom
  LoRa.setSpreadingFactor(SF);
  LoRa.setSignalBandwidth(BW);
  LoRa.setCodingRate4(CR);
  if (USE_CRC) LoRa.enableCrc(); else LoRa.disableCrc();
  LoRa.setSyncWord(SYNC_WORD);

  Serial.println(F("LoRa Initializing OK!"));
  Serial.print(F("Freq: ")); Serial.print(FREQ_HZ / 1e6, 3); Serial.println(F(" MHz"));
  Serial.print(F("SF/BW/CR: SF")); Serial.print(SF);
  Serial.print(F(" / ")); Serial.print((int)(BW/1000)); Serial.print(F(" kHz / 4/")); Serial.println(CR);
  Serial.print(F("SyncWord: 0x")); Serial.println(SYNC_WORD, HEX);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) return;

  // Čitanje bajt-po-bajt (bez blokiranja readString() u petlji)
  String msg;
  while (LoRa.available()) {
    msg += (char)LoRa.read();
  }

  long rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();

  Serial.print(F("RX '"));
  Serial.print(msg);
  Serial.print(F("' | RSSI "));
  Serial.print(rssi);
  Serial.print(F(" dBm | SNR "));
  Serial.print(snr, 1);
  Serial.println(F(" dB"));

  // LED blink ako sadrži "hello"
  if (msg.indexOf("hello") != -1) {
    blinkLed3x();
  }
}
