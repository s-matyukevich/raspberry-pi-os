#ifndef	_MINI_UART_H
#define	_MINI_UART_H

// values according to N = (ClockFreq./(8*BAUD RATE))-1

#define BAUDRATE_9600	3254
#define BAUDRATE_19200	1626
#define BAUDRATE_38400	812
#define BAUDRATE_57600	541
#define BAUDRATE_115200	270


void uart_init ( int baudrate );
char uart_recv ( void );
void uart_send ( char c );
void uart_send_string(char* str);

#endif  /*_MINI_UART_H */
