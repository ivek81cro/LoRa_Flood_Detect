#pragma once
#include <SPI.h>
#include <LoRa.h>

namespace config {
constexpr int I2C_SDA = 8;
constexpr int I2C_SCL = 9;
constexpr int WATER_SIG_PIN = 1;
constexpr int WATER_PWR_PIN = 4;
constexpr int SPI_SCK = 7;
constexpr int SPI_MISO = 5;
constexpr int SPI_MOSI = 6;
constexpr int LORA_SS = 10;
constexpr int LORA_RST = 21;
constexpr int LORA_DIO0 = 20;
constexpr long FREQ_HZ = 433375000L;
constexpr int TX_POWER_DBM = 10;
constexpr int SF = 9;
constexpr long BW = 125000L;
constexpr int CR = 5;
constexpr uint8_t SYNC_WORD = 0x12;
constexpr bool ENABLE_CRC = true;
constexpr bool ENFORCE_1_PERCENT_DUTY = true;
}

class LoRaRadio {
public:
    LoRaRadio();
    bool begin();
    bool send(const String& payload);
    static double calcToA(int payloadBytes);
};
