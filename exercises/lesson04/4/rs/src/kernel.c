#include "fork.h"
#include "irq.h"
#include "mini_uart.h"
#include "printf.h"
#include "sched.h"
#include "timer.h"
#include "utils.h"

void process(char *array) {
  while (1) {
    for (int i = 0; i < 5; i++) {
      uart_send(array[i]);
      delay(100000);
    }
  }
}

void kernel_main(void) {
  uart_init();
  init_printf(0, putc);
  irq_vector_init();
  timer_init();
  enable_interrupt_controller();
  enable_irq();

  printf("cp 1\r\n");
  int res = copy_process((unsigned long)&process, (unsigned long)"12345", 1);
  if (res != 0) {
    printf("error while starting process 1");
    return;
  }
  printf("cp 2\r\n");
  res = copy_process((unsigned long)&process, (unsigned long)"abcde", 2);
  if (res != 0) {
    printf("error while starting process 2");
    return;
  }

  printf("sched\r\n");
  while (1) {
    schedule();
  }
}
