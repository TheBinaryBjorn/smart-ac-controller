#pragma once
#include <cstdint>
#include <cstddef>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/uart.h"

class UARTDriver {
private:
    bool m_initialized{false};
    static constexpr uart_port_t UART_PORT{UART_NUM_0};
    static constexpr int RX_BUFFER_SIZE{256};
    static constexpr int TX_BUFFER_SIZE{0};
    static constexpr int QUEUE_SIZE{0};
    static constexpr QueueHandle_t* UART_QUEUE{nullptr};
    static constexpr int INTERRUPT_ALLOCATION_FLAGS{0};
    static constexpr int TX_PIN{1};
    static constexpr int RX_PIN{3}; 
    static constexpr char NEWLINE_CHAR{'\n'};
    static constexpr int PRINTF_BUFFER_SIZE{128};
public:
    bool begin(const uint32_t baud_rate);
    size_t write(const uint8_t byte);
    size_t write(const uint8_t* data, size_t length);
    size_t print(const char* text);
    size_t println(const char* text);
    size_t printf(const char* format, ...);

    // Optional, currently not needed in the project
    //void read();
    //void available();
    //void end();
    //void flush();
};