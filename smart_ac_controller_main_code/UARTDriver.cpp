#include "UARTDriver.h"

/*
    Initializes the serial UART driver using the uart_config_t
    struct, installs the drivers, applies parameters, sets pins
    and returns result.
*/
bool UARTDriver::begin(const uint32_t baud_rate) {
    // Store the baud rate
    m_baud_rate = baud_rate;

    // Create a configuration struct
    uart_config_t uart_config_struct{};
    uart_config_struct.baud_rate = baud_rate;
    uart_config_struct.data_bits = UART_DATA_8_BITS;
    uart_config_struct.parity = UART_PARITY_DISABLE;
    uart_config_struct.stop_bits = UART_STOP_BITS_1;
    uart_config_struct.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config_struct.source_clk = UART_SCLK_DEFAULT;

    // install driver
    esp_err_t driver_installation_result = uart_driver_install(UART_PORT,
        RX_BUFFER_SIZE, TX_BUFFER_SIZE, QUEUE_SIZE,
        UART_QUEUE, INTERRUPT_ALLOCATION_FLAGS);
    if (driver_installation_result != ESP_OK) {
        return false;
    }

    // apply config
    esp_err_t parameter_application_result = uart_param_config(UART_PORT,
        & uart_config_struct);
    if (parameter_application_result != ESP_OK) {
        uart_driver_delete(UART_PORT);
        return false;
    }

    // set TX and RX pins
    esp_err_t set_pin_result = uart_set_pin(UART_PORT,
        TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (set_pin_result != ESP_OK) {
        uart_driver_delete(UART_PORT);
        return false;
    }

    // return result
    m_initialized = true;
    return true;
}

size_t UARTDriver::write(uint8_t byte) {
    // To be Implemented.
    return 0;
}