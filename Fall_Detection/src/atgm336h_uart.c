#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "atgm336h_uart.h"
#include "FreeRTOS.h"
#include "task.h"


#define UART_ID uart1
#define BAUD_RATE 9600
#define ATGM336H_TX_PIN 8
#define ATGM336H_RX_PIN 9

bool atgm336h_uart_init()
{
    uint band = uart_init(UART_ID, BAUD_RATE);
    if (band == 0)
    {
        printf("Failed to initialize UART\n");
        return false;
    }
    gpio_set_function(ATGM336H_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(ATGM336H_RX_PIN, GPIO_FUNC_UART);
    
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, true);

    printf("UART initialized at %d baud\n", band);
    sleep_ms(2000);

    uart_puts(UART_ID, "$PCAS03,1,0,0,0,0,0,0,0,0,0,0,0,0,0*03\r\n");
    sleep_ms(1000);

    printf("Waiting gps module to respond...\n");
    
    bool gps_detected = false;
    uint32_t timeout = 5000;
    
    while (timeout > 0 && !gps_detected) {
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            if (c == '$') {
                gps_detected = true;
                printf("GPS module detected\n");
                break;
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
            timeout -= 10;
        }
    }
    
    if (!gps_detected) {
        printf("Warning: No GPS detected\n");
        return false;
    }
    
    printf("ATGM336H UART init complete\n");
    return true;
}

int atgm336h_uart_read_line(char *buf, size_t bufsize)
{
    uint32_t timeout = 0;
    const uint32_t max_timeout = 2000;
    size_t i = 0;
    bool found_start = false;
    memset(buf, 0, bufsize);
    
    while (uart_is_readable(UART_ID)) {
        uart_getc(UART_ID);
    }
    
    while (timeout < max_timeout && !found_start)
    {
        if (uart_is_readable(UART_ID))
        {
            char c = uart_getc(UART_ID);
            if (c == '$')
            {
                buf[i++] = c;
                found_start = true;
                break;
            }
            timeout = 0;
        }
        else {
            vTaskDelay(pdMS_TO_TICKS(1));
            timeout++;
        }
    }

    if (!found_start) {
        return 0;
    }

    bool valid_sentence = false;
    while (i < bufsize - 1 && timeout < max_timeout)
    {
        if (uart_is_readable(UART_ID))
        {
            char c = uart_getc(UART_ID);
            
            if (c == '$' && i > 1) {
                i = 0;
                buf[i++] = c;
                timeout = 0;
                continue;
            }
            
            buf[i++] = c;
            
            if (i >= 2 && buf[i-2] == '\r' && buf[i-1] == '\n')
            {
                buf[i] = '\0';
                valid_sentence = true;
                break;
            }
            timeout = 0;
        }
        else {
            vTaskDelay(pdMS_TO_TICKS(1));
            timeout++;
        }
    }
    
    buf[i] = '\0';
    return valid_sentence ? i : 0;
}
