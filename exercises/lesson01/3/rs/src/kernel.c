#include "mini_uart.h"
#include "utils.h"

static unsigned int cur_proc = 0;

void kernel_main(void) {
  unsigned long mpid = getmpid();

  // Only init UART on proc #0
  if (mpid == 0) {
    uart_init();
  }

  // Wait for previous proc to finish printing their message.
  while (cur_proc != mpid)
    ;

  uart_send_string("Hello, from processor ");
  uart_send(mpid + '0');
  uart_send_string("\r\n");

  // Signal the next proc to go.
  ++cur_proc;

  // Only proc #0 handles echo
  if (mpid == 0) {
    while (cur_proc != 4)
      ;

    while (1) {
      uart_send(uart_recv());
    }
  }
}
