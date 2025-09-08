/*********
  Adapted for HR/EU 433 MHz ISM
  - Frequency set away from 433.92 MHz (common remotes)
  - Conservative power and optional 1% duty-cycle helper
*********/

#include <SPI.h>
#include <LoRa.h>

// --- HW pins (PRILAGODI PO POTREBI) ---
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  2

// --- Radio postavke (HR/EU 433) ---
const long   FREQ_HZ = 433375000L; // 433.375 MHz (izbjegava 433.92 MHz)
const int    TX_POWER_DBM = 10;    // ≈10 dBm (≈10 mW ERP target)
const int    SF = 9;               // Balanced: SF9
const long   BW = 125E3;           // 125 kHz
const int    CR = 5;               // coding rate 4/5
const byte   SYNC_WORD = 0x12;     // dogovori sa Rx (isti na obje strane)
const bool   ENABLE_CRC = true;    // eksplicitni header + CRC

// --- Duty-cycle helper (≈1%): set true da automatski „odspava” dovoljno ---
const bool   ENFORCE_1_PERCENT_DUTY = true;

int counter = 0;

// ---- Pomoćni izračun Time-on-Air (gruba, ali korisna aproksimacija) ----
static double toa_ms_LoRa(int payloadBytes, int sf, long bw, int cr, bool crc) {
  // pretpostavke: explicit header (IH=0), preambleLen = 8, low data rate DE za SF>=11 @125k
  const int preambleLen = 8;
  const bool lowDR = (bw == 125000 && sf >= 11);
  const double tSym = (double)(1UL << sf) / (double)bw * 1000.0; // ms

  double tPreamble = (preambleLen + 4.25) * tSym;

  int IH = 0;                // explicit header
  int CRCon = crc ? 1 : 0;
  int DE = lowDR ? 1 : 0;

  // payload symbol calculation (ETSI/SEMTEC formula)
  double tmp = ceil( ( (8.0*payloadBytes - 4.0*sf + 28 + 16*CRCon - 20*IH)
                      / (4.0*(sf - 2*DE)) ) ) * (cr);
  double payloadSymbNb = 8 + (tmp < 0 ? 0 : tmp);

  double tPayload = payloadSymbNb * tSym;
  return tPreamble + tPayload; // ukupno u ms
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("LoRa Sender (HR 433 MHz)"));

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  // Inicijalizacija na odabranoj frekvenciji
  while (!LoRa.begin(FREQ_HZ)) {
    Serial.println(F("LoRa init retry..."));
    delay(500);
  }
  // Postavke linka
  LoRa.setSpreadingFactor(SF);
  LoRa.setSignalBandwidth(BW);
  LoRa.setCodingRate4(CR);
  if (ENABLE_CRC) LoRa.enableCrc(); else LoRa.disableCrc();
  LoRa.setSyncWord(SYNC_WORD);

  // Snaga (pazi na ERP/EIRP uz antenu i kabele)
  LoRa.setTxPower(TX_POWER_DBM);

  Serial.println(F("LoRa Initializing OK!"));
  Serial.print(F("Freq: ")); Serial.print(FREQ_HZ / 1e6, 3); Serial.println(F(" MHz"));
  Serial.print(F("SF/BW/CR: SF")); Serial.print(SF);
  Serial.print(F(" / ")); Serial.print((int)(BW/1000)); Serial.print(F(" kHz / 4/")); Serial.println(CR);
  Serial.print(F("TX power: ")); Serial.print(TX_POWER_DBM); Serial.println(F(" dBm"));
  Serial.print(F("SyncWord: 0x")); Serial.println(SYNC_WORD, HEX);
}

void loop() {
  // --- priprema poruke ---
  String msg = "hello " + String(counter);
  int payloadBytes = msg.length();

  // --- Slanje ---
  Serial.print(F("Sending packet: "));
  Serial.println(counter);

  LoRa.beginPacket();
  LoRa.print(msg);
  int rc = LoRa.endPacket(); // blokira do kraja slanja

  if (rc == 1) {
    Serial.println(F("Sent OK"));

    // Procijeni Time-on-Air i poštuj ≈1% duty-cycle (ako uključeno)
    double toa_ms = toa_ms_LoRa(payloadBytes, SF, BW, CR, ENABLE_CRC);
    Serial.print(F("Estimated ToA: "));
    Serial.print(toa_ms, 2);
    Serial.println(F(" ms"));

    if (ENFORCE_1_PERCENT_DUTY) {
      // 1% -> idle ~99x ToA
      unsigned long sleep_ms = (unsigned long)(toa_ms * 99.0);
      // dodatni minimum da ne spamamo eter (npr. >= 1s)
      if (sleep_ms < 1000UL) sleep_ms = 1000UL;
      Serial.print(F("Duty-cycle sleep(ms): "));
      Serial.println(sleep_ms);
      delay(sleep_ms);
    } else {
      delay(10000); // tvoj originalni delay
    }

  } else {
    Serial.println(F("Send failed"));
    delay(2000);
  }

  counter++;
}
