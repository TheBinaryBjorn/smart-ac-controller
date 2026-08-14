#include "SHT30Driver.h"

bool SHT30::begin(const uint8_t i2c_address){
    m_i2c_address = i2c_address;
    // Separate the SOFT RESET command into MSB and LSB.
    const uint8_t soft_reset_msb{static_cast<uint8_t>(SOFT_RESET >> 8)};
    const uint8_t soft_reset_lsb{static_cast<uint8_t>(SOFT_RESET & 0xFF)};
    Wire.beginTransmission(i2c_address);
    Wire.write(soft_reset_msb);
    Wire.write(soft_reset_lsb);
    return Wire.endTransmission() ? false : true;
}

float SHT30::convertTemperatureReadingToCelsius(const uint16_t temperature) {
    return TEMP_OFFSET + TEMP_SCALAR *temperature / ADC_MAX_VALUE;
}

float SHT30::convertHumidityReadingToRH(const uint16_t humidity) {
    return HUMIDITY_OFFSET * humidity / ADC_MAX_VALUE;
}

bool SHT30::validateChecksum(uint8_t msb, uint8_t lsb, uint8_t received_checksum) {
    uint8_t crc{CHECKSUM_INITIALIZATION};
    uint8_t bytes[2]{msb, lsb};
    for (uint8_t byte : bytes) {
        crc ^= byte;
        for (int i = 0; i < 8; i++) {
            bool isOne{(crc & 0x80) != 0};
            crc <<= 1;
            if(isOne) {
                crc ^= CHECKSUM_POLYNOMIAL;
            }
        }
    }
    return crc == received_checksum;
}

SHT30Reading SHT30::readTemperatureAndHumidity(){
    // Perform Measurement
    const uint8_t perform_measurement_msb{static_cast<uint8_t>(PERFORM_MEASUREMENT_COMMAND >> 8)};
    const uint8_t perform_measurement_lsb{static_cast<uint8_t>(PERFORM_MEASUREMENT_COMMAND & 0xFF)};
    Wire.beginTransmission(m_i2c_address);
    Wire.write(perform_measurement_msb);
    Wire.write(perform_measurement_lsb);
    Wire.endTransmission();

    const uint8_t read_bytes{Wire.requestFrom(m_i2c_address, SENSOR_READING_BYTE_COUNT)};
    if ( read_bytes != SENSOR_READING_BYTE_COUNT) {
        return {NAN, NAN};
    }

    // Read Result
    const uint8_t temperature_msb{Wire.read()};
    const uint8_t temperature_lsb{Wire.read()};
    const uint8_t temperature_crc{Wire.read()};
    const uint8_t humidity_msb{Wire.read()};
    const uint8_t humidity_lsb{Wire.read()};
    const uint8_t humidity_crc{Wire.read()};

    // Checksum Verification
    if (!validateChecksum(temperature_msb, temperature_lsb, temperature_crc) ||
        !validateChecksum(humidity_msb, humidity_lsb, humidity_crc)) {
        return {NAN, NAN};
    } 

    // Parse Result
    uint16_t temperature{static_cast<uint16_t>(temperature_msb)<<8};
    temperature |= temperature_lsb;

    uint16_t humidity{static_cast<uint16_t>(humidity_msb)<<8};
    humidity |= humidity_lsb;

    // Convert to Celsius and RH
    float temperature_celsius{convertTemperatureReadingToCelsius(temperature)};
    float relative_humidity{convertHumidityReadingToRH(humidity)};

    return {temperature_celsius, relative_humidity};
}