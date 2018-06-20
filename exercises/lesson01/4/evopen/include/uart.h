#ifndef _UART_H
#define _UART_H

void uart_init(void);
char uart_recv(void);
void uart_send(char c);
void uart_send_string(char* str);
unsigned int get_reg_baud(unsigned int core_freq, unsigned int baud);

#endif
