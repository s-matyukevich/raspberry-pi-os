#ifndef	_MINI_UART_H
#define	_MINI_UART_H

char uart_recv ( void );
void uart_send ( char c );
void uart_send_string(char* str);

/* Support 1200, 2400, 4800, 9600, 19200, 38400, 57600, and 115200 Baud */
typedef enum {
    BAUD_1200 = 1200,
    BAUD_2400 = 2400,
    BAUD_4800 = 4800,
    BAUD_9600 = 9600,
    BAUD_19200 = 19200,
    BAUD_38400 = 38400,
    BAUD_57600 = 57600,
    BAUD_115200 = 115200
} miniuart_baud_t;

void uart_init ( miniuart_baud_t baudrate );

#endif  /*_MINI_UART_H */
