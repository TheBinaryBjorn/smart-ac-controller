#include "SHT30Driver.h"

SHT30::SHT30(I2CDriver& i2c_driver) : m_i2c_driver(i2c_driver) {}

bool SHT30::begin(const uint8_t i2c_address){
    m_i2c_address = i2c_address;
    // Separate the SOFT RESET command into MSB and LSB.
    const uint8_t soft_reset_msb{static_cast<uint8_t>(SOFT_RESET >> 8)};
    const uint8_t soft_reset_lsb{static_cast<uint8_t>(SOFT_RESET & 0xFF)};
    
    // Transmit commands to SHT30 throught the i2c bus
    m_i2c_driver.beginTransmission(i2c_address);
    m_i2c_driver.write(soft_reset_msb);
    m_i2c_driver.write(soft_reset_lsb);
    const bool transmissionResult{m_i2c_driver.endTransmission()};
    if(!transmissionResult) {
        return false;
    }
    // Wait for sensor to perform soft reset
    vTaskDelay(pdMS_TO_TICKS(SOFT_RESET_DELAY_TIME));

    // Return result
    return true;
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
    m_i2c_driver.beginTransmission(m_i2c_address);
    m_i2c_driver.write(perform_measurement_msb);
    m_i2c_driver.write(perform_measurement_lsb);
    const bool transmissionSuccessful{m_i2c_driver.endTransmission()};
    if (!transmissionSuccessful) {
        return {NAN, NAN};
    }
    // Delay - wait for sensor to perform measurement.
    vTaskDelay(pdMS_TO_TICKS(MEASUREMENT_DELAY_TIME));

    // Read Result
    const uint8_t read_bytes{m_i2c_driver.requestFrom(m_i2c_address, SENSOR_READING_BYTE_COUNT)};
    if ( read_bytes != SENSOR_READING_BYTE_COUNT) {
        return {NAN, NAN};
    }
    const uint8_t temperature_msb{static_cast<uint8_t>(m_i2c_driver.read())};
    const uint8_t temperature_lsb{static_cast<uint8_t>(m_i2c_driver.read())};
    const uint8_t temperature_crc{static_cast<uint8_t>(m_i2c_driver.read())};
    const uint8_t humidity_msb{static_cast<uint8_t>(m_i2c_driver.read())};
    const uint8_t humidity_lsb{static_cast<uint8_t>(m_i2c_driver.read())};
    const uint8_t humidity_crc{static_cast<uint8_t>(m_i2c_driver.read())};

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