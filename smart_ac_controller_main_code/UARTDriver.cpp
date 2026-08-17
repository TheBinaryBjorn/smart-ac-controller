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
        &uart_config_struct);
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

/*
    Writes a given byte array to the UART TX Pin.
*/
size_t UARTDriver::write(const uint8_t* data, size_t length) {
    // Check if Driver is initialized, If the pointer is null or length is null return 0
    if (!m_initialized || data == nullptr || length == 0) {
        return 0;
    }
    // Utilize ESP-IDF uart_write_bytes
    int write_result = uart_write_bytes(UART_PORT, data, length);
    // Check result
    if (write_result < 0) {
        return 0;
    }
    // Return result
    return static_cast<size_t>(write_result);
}

/*
    Writes a single byte to the UART TX Pin.
*/
size_t UARTDriver::write(const uint8_t byte) {
    if (!m_initialized) {
        return 0;
    }
    return write(&byte, 1);
}

/*
    Prints a given c style string to the console 
*/
size_t UARTDriver::print(const char* text) {
    // Check if driver is initialized or pointer is null
    if (!m_initialized || text == nullptr) {
        return 0;
    }
    // Measure the length
    size_t text_length{std::strlen(text)};
    // Write to UART TX pin with write function
    return write(reinterpret_cast<const uint8_t*>(text), text_length);
}

/*
    Prints a given c style string to the console ending
    with a new line.
*/
size_t UARTDriver::println(const char* text) {
    // Check if driver is initialized or pointer is null
    if (!m_initialized || text == nullptr) {
        return 0;
    }
    // Utilize print and write
    size_t text_written_bytes{print(text)};
    size_t newline_written_bytes{write(NEWLINE_CHAR)};
    return text_written_bytes + newline_written_bytes;
}