#ifndef ATGM336H_UART_H // include guard
#define ATGM336H_UART_H


#ifdef __cplusplus
extern "C" {
#endif

bool atgm336h_uart_init();
int atgm336h_uart_read_line(char *buf, size_t bufsize);

#ifdef __cplusplus
}
#endif

#endif // ATGM336H_UART_H
