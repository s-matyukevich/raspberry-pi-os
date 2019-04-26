#ifndef	_UART_H
#define	_UART_H

void uart_init (int baudrate);
char uart_recv ( void );
void uart_send ( char c );
void uart_send_string(char* str);

#endif  /*_UART_H */
