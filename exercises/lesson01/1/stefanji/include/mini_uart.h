#ifndef	_MINI_UART_H
#define	_MINI_UART_H

void uart_init (unsigned int);
char uart_recv ( void );
void uart_send ( char c );
void uart_send_string(char* str);

#endif  /*_MINI_UART_H */
