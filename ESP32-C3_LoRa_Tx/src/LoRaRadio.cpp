#include "LoRaRadio.h"

/**
 * Konstruktor LoRaRadio klase.
 */
LoRaRadio::LoRaRadio() = default;

/**
 * Inicijalizacija LoRa modula s definiranim parametrima.
 * @return true ako je inicijalizacija uspješna, inače false.
 */
bool LoRaRadio::begin() {
    SPI.begin(config::SPI_SCK, config::SPI_MISO, config::SPI_MOSI, config::LORA_SS);
    LoRa.setPins(config::LORA_SS, config::LORA_RST, config::LORA_DIO0);
    int retries = 10;
    while (!LoRa.begin(config::FREQ_HZ) && retries--) {
        Serial.println(F("LoRa init retry..."));
        delay(500);
    }
    if (retries < 0) {
        Serial.println(F("FATAL: LoRa init failed."));
        return false;
    }
    LoRa.setSpreadingFactor(config::SF);
    LoRa.setSignalBandwidth(config::BW);
    LoRa.setCodingRate4(config::CR);
    if (config::ENABLE_CRC) LoRa.enableCrc(); else LoRa.disableCrc();
    LoRa.setSyncWord(config::SYNC_WORD);
    LoRa.setTxPower(config::TX_POWER_DBM);
    return true;
}

/**
 * Šalje podatke putem LoRa modula.
 * @param payload String s podacima za slanje.
 * @return true ako je slanje uspješno, inače false.
 */
bool LoRaRadio::send(const String& payload) {
    LoRa.beginPacket();
    LoRa.print(payload);
    int rc = LoRa.endPacket();
    return rc == 1;
}

/**
 * Izračunava Time-on-Air (ToA) za LoRa paket.
 * @param payloadBytes Broj bajtova u paketu.
 * @return Vrijeme trajanja paketa u milisekundama.
 */
double LoRaRadio::calcToA(int payloadBytes) {
    const int preambleLen = 8;
    const bool lowDR = (config::BW == 125000 && config::SF >= 11);
    const double tSym = (double)(1UL << config::SF) / (double)config::BW * 1000.0;
    double tPreamble = (preambleLen + 4.25) * tSym;
    int IH = 0;
    int CRCon = config::ENABLE_CRC ? 1 : 0;
    int DE = lowDR ? 1 : 0;
    double tmp = ceil(((8.0*payloadBytes - 4.0*config::SF + 28 + 16*CRCon - 20*IH) / (4.0*(config::SF - 2*DE)))) * (config::CR);
    double payloadSymbNb = 8 + (tmp < 0 ? 0 : tmp);
    return tPreamble + payloadSymbNb * tSym;
}
