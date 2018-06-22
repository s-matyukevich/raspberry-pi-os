#include "irq.h"
#include "mini_uart.h"
#include "printf.h"
#include "timer.h"

void kernel_main(void) {
  uart_init();
  init_printf(0, putc);
  irq_vector_init();
  arm_timer_init();
  enable_interrupt_controller();
  enable_irq();

  while (1) {
    uart_send(uart_recv());
  }
}
