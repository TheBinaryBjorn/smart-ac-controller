#pragma once
#include <cstdint>
#include "driver/i2c.h"

class I2CDriver {
private:
    uint8_t m_sda_pin;
    uint8_t m_scl_pin;
    i2c_cmd_handle_t m_command_link_queue;
public:
    bool begin(const uint8_t sda_pin, const uint8_t scl_pin);
    bool beginTransmission(const uint8_t i2c_address);
    bool endTransmission();
    bool write(const uint8_t data);
    uint8_t read();
    uint8_t requestFrom();
};