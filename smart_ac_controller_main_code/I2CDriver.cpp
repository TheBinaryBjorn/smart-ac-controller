#include "I2CDriver.h"

/*
    Creates a configuration container for the FreeRTOS,
    configuring the SDA and SCL pins, Mode(Master) and Clock speed,
    installs the driver
*/
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

/*
    Creates an ESP-IDF I2C command link queue in memory.
    Queues the I2C START condition and the target device address,
    instructing the hardware to wait for an ACK from the slave
    when the transmission is eventually executed.
*/
bool I2CDriver::beginTransmission(uint8_t i2c_address) {
    // Create the Command Queue
    m_command_link_queue = i2c_cmd_link_create();

    // Verify Memory Allocation
    if (m_command_link_queue == nullptr) {
        return false;
    }

    // Queue the START Condition
    i2c_master_start(m_command_link_queue);

    // Prepare the Address Byte
    i2c_address <<= 1;
    i2c_address |= I2C_MASTER_WRITE;

    // Queue the Address Byte
    i2c_master_write_byte(m_command_link_queue, i2c_address, ACK_CHECK_EN);

    // Return Success
    return true;
}

/*
    Adds a stop signal and passes the entire command link
    queue to the i2c hardware engine. checks for ack and 
    deletes the queue.
*/
bool I2CDriver::endTransmission() {
    // Safety Check
    if (m_command_link_queue == nullptr) {
        return false;
    }

    // Queue the STOP Condition
    i2c_master_stop(m_command_link_queue);

    // Execute the Queue
    esp_err_t queueExecutionResult = i2c_master_cmd_begin(I2C_NUM_0,
        m_command_link_queue, pdMS_TO_TICKS(1000));

    // Free the Memory
    i2c_cmd_link_delete(m_command_link_queue);
    m_command_link_queue = nullptr;

    // Return status
    return queueExecutionResult == ESP_OK;
}

/*
    Enqueues a given byte into the command link queue
*/
bool I2CDriver::write(const uint8_t data) {
    // Safety Check
    if (m_command_link_queue == nullptr) {
        return false;
    }

    // Queue the Data Byte
    i2c_master_write_byte(m_command_link_queue, data, ACK_CHECK_EN);

    // Return Status
    return true;
}


