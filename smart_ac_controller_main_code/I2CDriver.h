#pragma once
#include <cstdint>
class I2CDriver {
private:
    uint8_t m_sda_pin;
    uint8_t m_scl_pin;
public:
    bool begin(const uint8_t sda_pin, const uint8_t scl_pin);
    bool beginTransmission(const uint8_t i2c_address);
    void write(const uint8_t data);
    void endTransmission();
    uint8_t read();
    uint8_t requestFrom();
};