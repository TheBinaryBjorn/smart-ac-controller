#include "I2CDriver.h"
#include "driver/i2c.h"

bool I2CDriver::begin(const uint8_t sda_pin, const uint8_t scl_pin) {
    m_sda_pin = sda_pin;
    m_scl_pin = scl_pin;
    // Create Configuration Container for ESP-IDF
    i2c_config_t configurationContainer{};
    // Assign SDA and SCL pins to Configuration Container fields
    configurationContainer.sda_io_num = sda_pin;
    configurationContainer.scl_io_num = scl_pin;
    // Define the Role (ESP32 is Master)
    configurationContainer.mode = I2C_MODE_MASTER;
    // Activate Internal Pull Ups
    configurationContainer.sda_pullup_en = GPIO_PULLUP_ENABLE;
    configurationContainer.scl_pullup_en = GPIO_PULLUP_ENABLE;
    // Set the Clock Frequency
    configurationContainer.master.clk_speed = 100000;
    // Apply the Parameters
    esp_err_t applicationResult = i2c_param_config(I2C_NUM_0, &configurationContainer);
    if (applicationResult != ESP_OK) {
        return false;
    }
    // Install the Driver
    esp_err_t installResult = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (installResult != ESP_OK) {
        return false;
    }
    // Verify and return.
    return true;
}

bool I2CDriver::beginTransmission(const uint8_t i2c_address) {

    return true;
}

