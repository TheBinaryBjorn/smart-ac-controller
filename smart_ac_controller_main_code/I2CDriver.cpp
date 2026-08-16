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

/*
    Insert a READ command into the command link queue.
*/
uint8_t I2CDriver::read() {
    // TODO: Implement read function
    return 0;
}


uint8_t I2CDriver::requestFrom(uint8_t i2c_address, uint8_t quantity) {
    // Reset Input Buffer
    m_read_bytes_received = 0;
    m_read_buffer_index = 0;

    // Cap quantity at buffer size
    if (quantity > READ_BUFFER_SIZE) {
        quantity = READ_BUFFER_SIZE;
    }

    // Handling reading 0 bytes early.
    if (quantity == 0) {
        return 0;
    }

    // Create local command link queue
    i2c_cmd_handle_t local_cmd_link_queue{i2c_cmd_link_create()};
    if (local_cmd_link_queue == nullptr) {
        return 0;
    }

    // Queue a START cmd
    i2c_master_start(local_cmd_link_queue);

    // Prepare i2c address
    i2c_address <<= 1;
    i2c_address |= I2C_MASTER_READ;

    // Queue the address with ACK on
    i2c_master_write_byte(local_cmd_link_queue, i2c_address, ACK_CHECK_EN);

    // Queue READ Commands according to quantity
    if (quantity > 1) {
        i2c_master_read(local_cmd_link_queue, m_read_buffer,
            quantity-1, I2C_MASTER_ACK);
    }
    i2c_master_read(local_cmd_link_queue, &m_read_buffer[quantity-1],
        1, I2C_MASTER_NACK);
    
    // Queue STOP command
    i2c_master_stop(local_cmd_link_queue);

    // Execute the command link queue:
    esp_err_t queueExecutionResult = i2c_master_cmd_begin(I2C_NUM_0,
        local_cmd_link_queue, pdMS_TO_TICKS(1000));
    
    // Free the memory:
    i2c_cmd_link_delete(local_cmd_link_queue);
    local_cmd_link_queue = nullptr; // local variable won't cause a dangling pointer.

    // Verify command queue execution result
    if (queueExecutionResult == ESP_OK) {
        m_read_bytes_received = quantity;
        return quantity;
    }
    return 0;
}