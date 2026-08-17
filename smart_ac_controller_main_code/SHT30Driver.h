#pragma once
#include <cstdint>
#include <cmath>
#include "I2CDriver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct SHT30Reading {
    float temperature;
    float humidity;
};

class SHT30 {
private:
    // I2C Driver
    I2CDriver& m_i2c_driver;

    // Sensor I2C address.
    uint8_t m_i2c_address{};

    // Sensor Commands
    static constexpr uint16_t SOFT_RESET{0x30A2};
    static constexpr uint16_t PERFORM_MEASUREMENT_COMMAND{0x2C06};
    static constexpr uint8_t SENSOR_READING_BYTE_COUNT{6};

    // Sensor Delay Times
    static constexpr uint8_t SOFT_RESET_DELAY_TIME{2};
    static constexpr uint8_t MEASUREMENT_DELAY_TIME{15};

    // Reading Parsing
    static constexpr float TEMP_SCALAR{175.0f};
    static constexpr float TEMP_OFFSET{-45.0f};
    static constexpr float ADC_MAX_VALUE{65535.0f};
    static constexpr float HUMIDITY_OFFSET{100.0f};

    float convertTemperatureReadingToCelsius(const uint16_t temperature);
    float convertHumidityReadingToRH(const uint16_t humidity);

    // CRC CheckSum
    static constexpr uint8_t CHECKSUM_POLYNOMIAL{0x31};
    static constexpr uint8_t CHECKSUM_INITIALIZATION{0xFF};
    bool validateChecksum(uint8_t msb, uint8_t lsb, uint8_t crc);
public:
    SHT30(I2CDriver& i2c_driver);
    bool begin(const uint8_t i2c_address);
    SHT30Reading readTemperatureAndHumidity();
};