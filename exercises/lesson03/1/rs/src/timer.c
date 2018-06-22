#include "peripherals/timer.h"
#include "printf.h"
#include "utils.h"

const unsigned int interval = 200000;
unsigned int curVal = 0;

void timer_init(void) {
  curVal = get32(TIMER_CLO);
  curVal += interval;
  put32(TIMER_C1, curVal);
}

void handle_timer_irq(void) {
  curVal += interval;
  put32(TIMER_C1, curVal);
  put32(TIMER_CS, TIMER_CS_M1);
  printf("Timer interrupt received\n\r");
}

void arm_timer_init(void) {
  // According to the timer pre-divider register documentation, the default
  // pre-devider is configured to run at apb_clock/126 which is 1Mhz like the
  // system timer.
  put32(ARM_TIMER_LOAD, interval);
  put32(ARM_TIMER_CTRL, CTRL_ENABLE | CTRL_INT_ENABLE | CTRL_23BIT);
}

void handle_arm_timer_irq(void) {
  put32(ARM_TIMER_CLR, 1);
  printf("Timer interrupt received\n\r");
}
