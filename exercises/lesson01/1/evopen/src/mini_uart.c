#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"
#include "utils.h"

unsigned int get_reg_baud(unsigned int core_freq, unsigned int baud) {
    return core_freq * 1000000 / (8 * baud) - 1;
}

void uart_init(void) {
    unsigned int selector;  // 32 bits

    selector = get32(GPFSEL1);
    selector &= ~(7 << 12);
    selector |= 2 << 12;
    selector &= ~(7 << 15);
    selector |= 2 << 15;
    put32(GPFSEL1, selector);

    put32(GPPUD, 0);
    delay(150);
    put32(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);
    put32(GPPUDCLK0, 0);

    put32(AUX_ENABLES,
          1);  // Enable mini uart (this also enables access to it registers)
    put32(AUX_MU_CNTL_REG, 0);  // Disable auto flow control
    put32(AUX_MU_IER_REG, 0);   // Disable receive and transmit interrupts
    put32(AUX_MU_LCR_REG, 3);   // Enable 8 bit mode
    put32(AUX_MU_MCR_REG, 0);   // Set RTS line to be always high
    put32(AUX_MU_BAUD_REG,
          get_reg_baud(CORE_FREQ, BAUD_FREQ));  // Set baud rate to 115200
    put32(AUX_MU_IIR_REG, 6);                   // Clear FIFO

    put32(AUX_MU_CNTL_REG, 3);  // Finally, enable transmitter and receiver
}

void uart_send(char c) {
    while (1) {
        if (get32(AUX_MU_LSR_REG) & 32) break;
    }
    put32(AUX_MU_IO_REG, c);
}

char uart_recv(void) {
    while (1) {
        if (get32(AUX_MU_LSR_REG) & 1) break;
    }
    return (get32(AUX_MU_IO_REG) & 0xFF);
}

void uart_send_string(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        uart_send((char)str[i]);
    }
}
