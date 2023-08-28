#include "utils.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"
#include "custom_printf.h"

void uart_send(char c)
{
	while (get32(UART0_BASE + UART_FR_OFFSET) & (1 << 5))
	{
	}
	put32(UART0_BASE + UART_DR_OFFSET, c);
}

char uart_recv(void)
{
	while (get32(UART0_BASE + UART_FR_OFFSET) & UART_RX_MASK)
	{
	}
	return (get32(UART0_BASE + UART_DR_OFFSET) & 0xFF);
}

void uart_send_string(char *str)
{
	for (int i = 0; str[i] != '\0'; i++)
	{
		uart_send((char)str[i]);
	}
}

void uart_init(void)
{

	unsigned int selector;

	selector = get32(GPFSEL1);
	selector &= ~(7 << 12); // clean gpio14
	selector |= 4 << 12;	// set alt0 for gpio14
	selector &= ~(7 << 15); // clean gpio15
	selector |= 4 << 15;	// set alt0 for gpio15
	put32(GPFSEL1, selector);

	unsigned int pullRegister;
	pullRegister = get32(GPIO_PUP_PDN_CNTRL_REG0);
	pullRegister &= ~(3 << 30);
	pullRegister &= ~(3 << 28);
	put32(GPIO_PUP_PDN_CNTRL_REG0, pullRegister);

	// first disable uart
	put32(UART0_BASE + UART_CR_OFFSET, 0);

	// unmask all interrupts -> disable them
	put32(UART0_BASE + UART_IMSC_OFFSET, 0);

	//from ../adkaster/src/uart.c
	// Assume 48MHz UART Reference Clock (Standard)
	// Calculate UART clock divider per datasheet
	// BAUDDIV = (FUARTCLK/(16 Baud rate))
	//  Note: We get 6 bits of fraction for the baud div
	// 48000000/(16 * 115200) = 3000000/115200 = 26.0416666...
	// Integer part = 26 :)
	// From http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0183g/I54603.html
	// we want floor(0.04166666.. * 64 + 0.5) = 3
	put32(UART0_BASE + UART_IBRD_OFFSET, 26);
	put32(UART0_BASE + UART_FBRD_OFFSET, 3);

	//0111|0000 => 8 bits and enable fifos
	put32(UART0_BASE + UART_LCRH_OFFSET, 7 << 4);

	// if the fifo queue is enabled set the lowest levels to trigger an interrupt.
	put32(UART0_BASE + UART_IFLS_OFFSET, 0);

	// 0011|0000|0001 => enable: rx tx uart
	put32(UART0_BASE + UART_CR_OFFSET, (1 << 9) | (1 << 8) | (1 << 0));

	// enable the RX and RX timeout interrupt
	put32(UART0_BASE + UART_IMSC_OFFSET, (UART_IRQ_RXIM_MASK | UART_IRQ_RTIM_MASK));
}

// This function is required by printf function
void putc(void *p, char c)
{
	uart_send(c);
}

void handle_uart_interrupt(void)
{

	// Which UART was the interrupt source
	// UART5 IRQ bit 16
	// UART4 IRQ bit 17
	// UART3 IRQ bit 18
	// UART2 IRQ bit 19
	// UART0 IRQ bit 20
	unsigned int uart = get32(PACTL_CS_REG) & (0b11111 << 16);
	switch (uart)
	{
	case 1 << 20:
		uart = 0;
		break;
	case 1 << 19:
		uart = 2;
		break;
	case 1 << 18:
		uart = 3;
		break;
	case 1 << 17:
		uart = 4;
		break;
	case 1 << 16:
		uart = 5;
		break;
	default:
		uart = 1;
	}

	static unsigned int uart_bases[] = {UART0_BASE, -1, UART2_BASE, UART3_BASE, UART4_BASE, UART5_BASE};

	unsigned int base = uart_bases[uart];


	// Read the exact interrupt type from the masked interrupt status register
	// and check whether it is a RX
	unsigned int uart_irq_status = get32(base + UART_MIS_OFFSET);

	printf("Detected interrupt 0x%x for UART%d at 0x%x: ", uart_irq_status, uart, base);
	
	if (uart_irq_status & (UART_IRQ_RXIM_MASK | UART_IRQ_RTIM_MASK))
	{		
		uart_send(uart_recv());
	}

	// Clear the interrupt
	put32(base + UART_ICR_OFFSET, uart_irq_status);
}
