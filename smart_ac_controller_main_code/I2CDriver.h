#pragma once
#include <cstdint>
#include "driver/i2c.h"

class I2CDriver {
private:
    uint8_t m_sda_pin{};
    uint8_t m_scl_pin{};
    i2c_cmd_handle_t m_command_link_queue{};
    static constexpr bool ACK_CHECK_EN{true};

    // Read Buffer
    static constexpr uint8_t READ_BUFFER_SIZE{32};
    uint8_t m_read_buffer[READ_BUFFER_SIZE]{};
    uint8_t m_read_bytes_received{};
    uint8_t m_read_buffer_index{};

public:
    bool begin(const uint8_t sda_pin, const uint8_t scl_pin);
    bool beginTransmission(const uint8_t i2c_address);
    bool endTransmission();
    bool write(const uint8_t data);
    int read();
    uint8_t requestFrom(uint8_t i2c_address, uint8_t quantity);
};