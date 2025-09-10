/*********
  ESP32-C3 SuperMini: AHT20 + BMP280 + Water sensor + LoRa TX (Ra-02 / SX1278)
  - HR/EU 433 MHz ISM (433.375 MHz)
  - Power-gated water senzor (GPIO20)
  - 1% duty-cycle helper i ToA procjena
  - Dodani periodični Serial ispisi svake sekunde
*********/

#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

// ---------- I2C senzori ----------
#define I2C_SDA 8
#define I2C_SCL 9
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp; // I2C

// ---------- Water senzor ----------
#define WATER_SIG_PIN   1    // ADC ulaz
#define WATER_VCC_PIN   20   // napajanje senzora (power gating)

// ---------- LoRa pinovi (ESP32-C3 + Ra-02) ----------
#define LORA_SS    7
#define LORA_RST   10
#define LORA_DIO0  2
#define SPI_SCK    4
#define SPI_MISO   5
#define SPI_MOSI   6

// ---------- LoRa radio postavke ----------
const long   FREQ_HZ = 433375000L;
const int    TX_POWER_DBM = 10;
const int    SF = 9;
const long   BW = 125E3;
const int    CR = 5;
const byte   SYNC_WORD = 0x12;
const bool   ENABLE_CRC = true;

const bool   ENFORCE_1_PERCENT_DUTY = true;

// ---------------- HELPER FUNKCIJE ----------------
static double toa_ms_LoRa(int payloadBytes, int sf, long bw, int cr, bool crc) {
  const int preambleLen = 8;
  const bool lowDR = (bw == 125000 && sf >= 11);
  const double tSym = (double)(1UL << sf) / (double)bw * 1000.0;
  double tPreamble = (preambleLen + 4.25) * tSym;
  int IH = 0;
  int CRCon = crc ? 1 : 0;
  int DE = lowDR ? 1 : 0;
  double tmp = ceil(((8.0*payloadBytes - 4.0*sf + 28 + 16*CRCon - 20*IH) / (4.0*(sf - 2*DE)))) * (cr);
  double payloadSymbNb = 8 + (tmp < 0 ? 0 : tmp);
  return tPreamble + payloadSymbNb * tSym;
}

// Kalibracija water senzora
int waterPercent(int adc) {
  const int dry = 200;   // suho
  const int wet = 2800;  // mokro
  int p = (adc - dry) * 100 / (wet - dry);
  if (p < 0) p = 0;
  if (p > 100) p = 100;
  return p;
}

int readWaterRawOnce() {
  digitalWrite(WATER_VCC_PIN, HIGH);
  delay(10);
  int raw = analogRead(WATER_SIG_PIN);
  digitalWrite(WATER_VCC_PIN, LOW);
  return raw;
}

int readWaterMedian() {
  const int N = 7;
  int v[N];
  for (int i = 0; i < N; i++) { v[i] = readWaterRawOnce(); delay(2); }
  for (int i = 0; i < N; i++) for (int j = i+1; j < N; j++) if (v[j] < v[i]) { int t=v[i]; v[i]=v[j]; v[j]=t; }
  return v[N/2];
}

// ---------------- GLOBAL VARS ----------------
bool haveAHT=false, haveBMP=false;
uint32_t counter = 0;

float last_ahtT=NAN, last_ahtH=NAN, last_bmpT=NAN, last_bmpP=NAN;
int last_waterADC=0, last_waterPct=0;

unsigned long lastSensorPrint = 0;
unsigned long lastLoRaSend = 0;
unsigned long loraInterval = 10000; // početno 10s (kasnije se računa prema duty-cycle)

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("\nBoot: ESP32-C3 + AHT20 + BMP280 + Water + LoRa TX"));

  // --- I2C ---
  Wire.begin(I2C_SDA, I2C_SCL);

  // --- Water pins ---
  pinMode(WATER_VCC_PIN, OUTPUT);
  digitalWrite(WATER_VCC_PIN, LOW);
  pinMode(WATER_SIG_PIN, INPUT);

  // --- AHT20 ---
  haveAHT = aht.begin(&Wire);
  Serial.println(haveAHT ? F("AHT20 OK") : F("AHT20 not found"));

  // --- BMP280 ---
  haveBMP = bmp.begin(0x76);
  if (!haveBMP) haveBMP = bmp.begin(0x77);
  if (haveBMP) {
    Serial.println(F("BMP280 OK"));
    bmp.setSampling(
      Adafruit_BMP280::MODE_NORMAL,
      Adafruit_BMP280::SAMPLING_X2,
      Adafruit_BMP280::SAMPLING_X16,
      Adafruit_BMP280::FILTER_X16,
      Adafruit_BMP280::STANDBY_MS_63
    );
  } else {
    Serial.println(F("BMP280 not found"));
  }

  // --- LoRa SPI + pinovi ---
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  int retries = 10;
  while (!LoRa.begin(FREQ_HZ) && retries--) {
    Serial.println(F("LoRa init retry..."));
    delay(500);
  }
  if (retries < 0) {
    Serial.println(F("FATAL: LoRa init failed."));
    while (true) delay(1000);
  }

  LoRa.setSpreadingFactor(SF);
  LoRa.setSignalBandwidth(BW);
  LoRa.setCodingRate4(CR);
  if (ENABLE_CRC) LoRa.enableCrc(); else LoRa.disableCrc();
  LoRa.setSyncWord(SYNC_WORD);
  LoRa.setTxPower(TX_POWER_DBM);

  Serial.println(F("LoRa OK"));
}

// ---------------- LOOP ----------------
void loop() {
  unsigned long now = millis();

  // --- Čitanje senzora ---
  if (haveAHT) {
    sensors_event_t hum, temp;
    if (aht.getEvent(&hum, &temp)) {
      last_ahtT = temp.temperature;
      last_ahtH = hum.relative_humidity;
    }
  }
  if (haveBMP) {
    last_bmpT = bmp.readTemperature();
    last_bmpP = bmp.readPressure() / 100.0f;
  }
  last_waterADC = readWaterMedian();
  last_waterPct = waterPercent(last_waterADC);

  // --- Serijski ispisi samo kada se šalje LoRa poruka ---
  // (ispis je premješten u blok za slanje LoRa poruke)

  // --- Slanje LoRa poruke prema duty-cycle ---
  if (now - lastLoRaSend >= loraInterval) {
    lastLoRaSend = now;

    String payload = "{";
    payload += "\"c\":" + String(counter);
    if (!isnan(last_ahtT)) payload += ",\"t\":" + String(last_ahtT,1);
    if (!isnan(last_ahtH)) payload += ",\"h\":" + String(last_ahtH,1);
    if (!isnan(last_bmpT)) payload += ",\"tp\":" + String(last_bmpT,1);
    if (!isnan(last_bmpP)) payload += ",\"p\":" + String(last_bmpP,1);
    payload += ",\"w\":" + String(last_waterPct);
    payload += ",\"a\":" + String(last_waterADC);
    payload += "}";

    Serial.print(F("TX payload: "));
    Serial.println(payload);

    // --- Serijski ispisi senzorskih podataka ---
    Serial.print("AHT20 Temp: "); Serial.print(last_ahtT,1); Serial.print(" °C, ");
    Serial.print("Hum: "); Serial.print(last_ahtH,1); Serial.print(" %, ");
    Serial.print("BMP280 Temp: "); Serial.print(last_bmpT,1); Serial.print(" °C, ");
    Serial.print("Press: "); Serial.print(last_bmpP,1); Serial.print(" hPa, ");
    Serial.print("Water: "); Serial.print(last_waterPct); Serial.print(" % (ADC=");
    Serial.print(last_waterADC); Serial.println(")");

    LoRa.beginPacket();
    LoRa.print(payload);
    int rc = LoRa.endPacket();

    if (rc == 1) {
      Serial.println(F("LoRa sent OK"));
      double toa_ms = toa_ms_LoRa(payload.length(), SF, BW, CR, ENABLE_CRC);
      loraInterval = ENFORCE_1_PERCENT_DUTY
                     ? (unsigned long)max(1000.0, toa_ms * 99.0)
                     : 10000UL;
      Serial.print(F("ToA(ms): ")); Serial.print(toa_ms,2);
      Serial.print(F(" -> Next TX in(ms): ")); Serial.println(loraInterval);
    } else {
      Serial.println(F("LoRa send FAILED"));
      loraInterval = 2000;
    }

    counter++;
  }
}
