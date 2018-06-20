#include "peripherals/uart.h"
#include "peripherals/gpio.h"
#include "utils.h"

void uart_send(char c) {
  // TXFF: Transmit FIFO full.
  while (get32(UART_FR_REG) & (1 << 5))
    ;
  put32(UART_DR_REG, c);
}

char uart_recv(void) {
  // RXFE: Receive FIFO empty.
  while (get32(UART_FR_REG) & (1 << 4))
    ;
  return (get32(UART_DR_REG) & 0xFF);
}

void uart_send_string(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    uart_send((char)str[i]);
  }
}

void uart_init(void) {
  unsigned int selector;

  selector = get32(GPFSEL1);
  selector &= ~(7 << 12); // clean gpio14
  selector |= 4 << 12;    // set alt0 for gpio14
  selector &= ~(7 << 15); // clean gpio15
  selector |= 4 << 15;    // set alt0 for gpio15
  put32(GPFSEL1, selector);

  put32(GPPUD, 0);
  delay(150);
  put32(GPPUDCLK0, (1 << 14) | (1 << 15));
  delay(150);
  put32(GPPUDCLK0, 0);

  // Baud rate divisor BAUDDIV = (FUARTCLK/(16 Baud rate)) where FUARTCLK is the
  // UART reference clock frequency. The BAUDDIV is comprised of the integer
  // value IBRD and the fractional value FBRD.
  unsigned int baud = UART_CLOCK * 4 / UART_BAUD_RATE;
  put32(UART_IBRD_REG, ((unsigned int)(baud >> 6)) & IBRD_BDIVINT);
  put32(UART_FBRD_REG, (unsigned int)(baud & 0x3F) & FBRD_BDIVFRAC);

  // Enable FIFO and set word length to 8bits.
  put32(UART_LCRH_REG, LCRH_FEN | LCRH_WLEN_8BITS);

  // Enable UART.
  put32(UART_CR_REG, CR_ENABLE | CR_TXE | CR_RXE);
}
