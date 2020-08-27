## 5.1: User processes and system calls

RPi OS를 더 이상 bare-metal 프로그램이 아닌 실제 운영 체제처럼 만들어주는 많은 기능들을 추가하였다. 이제 RPi OS는 프로세스를 관리할 수 있지만  여전히 프로세스를 격리할 수는 없다는 큰 단점이 존재한다. 이번 lesson에서 이 문제를 해결해 볼 것이다. 먼저,  모든 사용자 프로세스를 EL0으로 옮겨서 특권 프로세스 동작에 대한 접근을 제한할 것이다. 이 작업 없이는 어떤 사용자 프로그램이 우리의 보안 설정을 덮어쓸 수 있고 그러면 격리가 무너지게 된다.

사용자 프로그램이 커널 함수에 직접 접근하는 것을 제한한다면 다른 문제가 발생한다. 예를 들어 사용자 프로그램이 사용자에게 무언가를 출력해야 한다면? 우리는 UART 장치를 직접적으로 동작시키길 절대 원하지 않는다. 대신에 OS가 API method 집합으로 각 프로그램을 제공하는 것이 좋을 듯하다. 이런 API는 사용자 프로그램이 호출할 때마다 현재 exception level을 EL1으로 올려야 하기 때문에 단일 method의 집합으로 구현될 수는 없다. 개별적인 API method를 system call이라고 하며 이번 lesson에서 RPi OS에 시스템 콜 집합을 추가할 것이다.

프로세스 격리에 대한 세 번째 개념이 있다. 각 프로세스는 독립적인 메모리를 가진다고 생각해야 한다. - 이 문제에 대해서는 lesson6에서 다룰 것이다.

### System calls implementation

시스템 콜에 대한 핵심 아이디어는 매우 간단하다. 각 시스템 콜은 사실 동기적 예외이다. 사용자 프로그램이 시스템 콜을 실행해야 할 때, 먼저 필요한 모든 전달인자를 준비하고  `svc`명령어를 실행한다. 이 명령어는 동기적 예외를 발생시킨다. 이런 예외는 운영체제에 의해 EL1에서 처리된다. 운영체제는 모든 전달인자의 유효성을 검사하고, 요청된 작업을 수행한 뒤, 정상적인 예외 리턴을 실행하여 EL0에서  `svc`명령어 바로 다음부터 실행이 재개되도록 한다. RPi OS는 4개의 간단한 시스템 콜을 정의한다:

1. `write` 이 시스템 콜은 UART 장치를 사용해 화면에 무언가를 출력한다. 첫 번째 전달인자로 출력할 문자열이 담긴 버퍼를 받는다.
1. `clone` 이 시스템 콜은 새로운 사용자 스레드를 생성한다. 첫 번째 전달인자로 새로 생성된 스레드의 스택 위치를 넘겨준다.
1. `malloc` 이 시스템 콜은 사용자 프로세스를 위해 메모리 페이지를 할당한다. 리눅스에는 이런 역할의 시스템 콜이 없다. RPi OS에서 필요한 이유는 아직 가상 메모리가 구현되지 않아서 모든 사용자 프로세스가 물리 메모리에서 동작하기 때문이다. 때문에 각 프로세스는 어떤 메모리 페이지를 사용할 수 있는지 찾을 방법이 필요하다. `malloc` 시스템 콜은 새로 할당된 페이지에 대한 포인터를 리턴하며, 에러가 발생한 경우에는 1을 리턴한다.
1. `exit` 각 프로세스는 실행을 마친 뒤 이 시스템 콜을 반드시 호출해야 한다. 이 시스템 콜은 필요한 모든 것을 정리할 것이다.

모든 시스템 콜은 [sys.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.c) 파일에 정의되어 있다.  여기에는 모든 시스템 콜의 handler에 대한 포인터를 포함하는 [sys_call_table](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.c) 배열이 있다.  각 시스템 콜은 syscall number를 가진다. — 이는 단순히 `sys_call_table` 배열의 인덱스이다.  모든 syscall numbers 는 [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/sys.h#L6)에 정의되어 있다. — syscall number는 어셈블러 코드에서 원하는 시스템 콜을 지정할 때 사용된다. `write` 시스템 콜을 예로 들어 시스템 콜 wrapper 함수를 살펴보자. 이는 [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.S#L4)에서 찾을 수 있다.

```
.globl call_sys_write
call_sys_write:
    mov w8, #SYS_WRITE_NUMBER
    svc #0
    ret
```
 이 함수는 매우 간단하다. `w8` 레지스터에 시스템 콜 번호를 저장하고 `svc` 명령어를 실행해 동기적 예외를 생성한다. `w8` 규약에 따라 시스템 콜 번호로 사용된다. `x0`-`x7` 레지스터는 시스템 콜 전달인자에 사용되며 `x8`은 시스템 콜 번호로 사용되어 시스템 콜은 최대 8개의 전달인자를 가질 수 있다.

이러한 wrapper 함수는 보통 커널 자체에 포함되지 않는다. [glibc](https://www.gnu.org/software/libc/)와 같은 다른 언어의 표준 라이브러리에서 찾을 가능성이 높다.


### Handling synchronous exceptions

동기적 예외가 발생한 후에는 exception table에 등록된 [handler](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/entry.S#L98)가 호출된다. Handler 자체는 [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/entry.S#L157)에서 찾을 수 있으며 이는 다음 코드로 시작한다.
```
el0_sync:
    kernel_entry 0
    mrs    x25, esr_el1                // read the syndrome register
    lsr    x24, x25, #ESR_ELx_EC_SHIFT // exception class
    cmp    x24, #ESR_ELx_EC_SVC64      // SVC in 64-bit state
    b.eq   el0_svc
    handle_invalid_entry 0, SYNC_ERROR
```


먼저, 모든 exception handler에서는 `kernel_entry` 매크로를 호출한다. 그런 후에 `esr_el1`(Exception Syndrome Register)를 확인한다. 이 레지스터는 오프셋  [ESR_ELx_EC_SHIFT](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/arm/sysregs.h#L46)에 exception class 필드를 포함한다. 만약 exception class가  [ESR_ELx_EC_SVC64](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/arm/sysregs.h#L47) 와 동일하다면 이는 현재 예외가 `svc` 명령어로부터 초래된 시스템 콜임을 의미한다. 이 경우, `el0_svc` 레이블로 분기하거나 아니면 오류 메시지를 표시한다.


```
sc_nr   .req    x25                  // number of system calls
scno    .req    x26                  // syscall number
stbl    .req    x27                  // syscall table pointer

el0_svc:
    adr    stbl, sys_call_table      // load syscall table pointer
    uxtw   scno, w8                  // syscall number in w8
    mov    sc_nr, #__NR_syscalls
    bl     enable_irq
    cmp    scno, sc_nr               // check upper syscall limit
    b.hs   ni_sys

    ldr    x16, [stbl, scno, lsl #3] // address in the syscall table
    blr    x16                       // call sys_* routine
    b      ret_from_syscall
ni_sys:
    handle_invalid_entry 0, SYSCALL_ERROR
```
`el0_svc`는 먼저 `stbl`(x27의 별칭)에 있는 syscall table의 주소와 `scno` 변수의 시스템 콜 번호를 로드한다. 그런 뒤에 인터럽트가 활성화되고 시스템 콜 번호를 시스템의 총 시스템 콜 개수와 비교한다. 시스템 콜 번호가 전체 개수보다 크거나 같다면 오류 메시지를 출력한다. 시스템 콜 번호가 요구되는 범위 내로 들어오면 syscall handler의 포인터를 얻기 위해 syscall table 배열의 인덱스로 사용된다. 그런 다음 handler가 실행되고, 종료한 후에는 `ret_from_syscall`이 호출된다. 여기서 `x0`-`x7` 레지스터를 건들지 않는다는 것에 주의하자. 이 레지스터들은 그대로 handler에 전달된다.
```
ret_from_syscall:
    bl    disable_irq
    str   x0, [sp, #S_X0]             // returned x0
    kernel_exit 0
```
`ret_from_syscall`에서는 먼저 인터럽트를 비활성화한다. 그리고 `x0` 레지스터의 값을 스택에 저장한다. `kernel_exit`에서 모든 저장된 범용 레지스터를 복구하지만 `x0`은 이제 syscall handler의 리턴값을 가지므로 사용자 코드에 이 값을 전달하기 위해서는 스택에 저장해야 한다. 드디어 `kernel_exit`가 호출되어 사용자 코드로 돌아간다.

### Switching between EL0 and EL1

이전 lesson을 주의깊게 읽었다면 `kernel_entry`와 `kernel_exit` 매크로의 변화를 눈치챘을 것이다. 이제 이 둘은 추가적인 전달인자를 받는다. 이 전달인자는 예외가 발생한 exception level을 나타낸다. 기존 exception level에 대한 정보는 스택 포인터를 적절히 저장 및 복구하기 위해 필요하다. 다음은 `kernel_entry`매크로와 `kernel_exit`매크로의 대응되는 부분이다.

```
    .if    \el == 0
    mrs    x21, sp_el0
    .else
    add    x21, sp, #S_FRAME_SIZE
    .endif /* \el == 0 */
```

```
    .if    \el == 0
    msr    sp_el0, x21
    .endif /* \el == 0 */
```

우리는 EL0과 EL1에 대해 개별적인 2개의 스택 포인터를 사용한다. 따라서 EL0에서 예외가 전달된 직후 스택 포인터를 덮어쓴다. 기존의 스택 포인터는 `sp_el0` 레지스터에서 찾을 수 있다. 이 레지스터의 값은 handler에서 사용되지 않더라도 예외가 전달되기 전과 후에 저장 및 복구되어야 한다. 이 과정을 진행하지 않는다면 문맥 교환 후에 `sp` 레지스터는 결국 틀린 값을 가지게 될 것이다.

EL1에서 예외가 발생한 경우에 `sp` 레지스터 값을 저장하지 않는 이유도 궁금할 것이다. 그 이유는 exception handler에서 동일한 커널 스택을 재사용할 것이기 때문이다. 예외 처리 도중에 `kernel_exit`에 의한 문맥 교환이 발생하더라도 `sp`는 이미 `cpu_switch_to` 함수에 의해 전환되었을 것이다. 리눅스에서는 exception handler에 다른 스택을 사용하기 때문에 동작이 이와 다르다.

또한 `eret` 명령 전에 어떤 exception level이 리턴되어야 하는지 명시적으로 지정할 필요가 없다. 이 정보는 `spsr_el1` 레지스터에 인코딩되어 있기 때문에 항상 예외가 발생했던 level로 리턴한다.

### Moving a task to user mode

어떤 시스템 콜이 발생하기 전에는 사용자 모드에서 task가 실행되어야 한다. 새로운 사용자 task를 생성하는 방법에는 두 가지가 있다. 커널 스레드가 사용자 모드로 이동하거나 사용자 task가 fork되어 새로운 사용자 task를 생성하는 것이다. 이 섹션에서는 첫 번째 경우를 살펴볼 것이다.

이 작업을 수행하는 함수는 [move_to_user_mode](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/fork.c#Li47)이다. 이 함수를 살펴보기 전에 어떻게 사용되는지 살펴보자. 그러기 위해 [kernel.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/kernel.c) 파일을 먼저 살펴보자. 다음이 관련 코드이다.

```
    int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0, 0);
    if (res < 0) {
        printf("error while starting kernel process");
        return;
    }
```

먼저 `kernel_main` 함수에서 새로운 커널 스레드를 생성한다. 이전 lesson에서 했던 것과 동일한 방법을 사용한다. 스케줄러가 새로 생성된 task를 실행한 뒤, `kernel_process` 함수가 커널 모드에서 실행된다.

```
void kernel_process(){
    printf("Kernel process started. EL %d\r\n", get_el());
    int err = move_to_user_mode((unsigned long)&user_process);
    if (err < 0){
        printf("Error while moving process to user mode\n\r");
    }
}
```
`kernel_process` 함수는 상태 메시지를 출력하고, `user_process`에 대한 포인터를 첫 번째 전달인자로 하여 `move_to_user_mode` 함수를 호출한다. 이제 이 함수가 하는 일을 살펴보자.
```
int move_to_user_mode(unsigned long pc)
{
    struct pt_regs *regs = task_pt_regs(current);
    memzero((unsigned long)regs, sizeof(*regs));
    regs->pc = pc;
    regs->pstate = PSR_MODE_EL0t;
    unsigned long stack = get_free_page(); //allocate new user stack
    if (!stack) {
        return -1;
    }
    regs->sp = stack + PAGE_SIZE;
    current->stack = stack;
    return 0;
}
```
지금은 init task로부터 fork된 새로운 커널 스레드를 실행하는 중이다. 이전 lesson에서 프로세스를 fork하는 것에 대해 다루었고, 새로 생성된 task의 스택의 top에 `pt_regs` 영역이 예약되는 것을 보았다. 이번에 처음으로 이 영역을 사용할 것이다. 우리는 수동으로 준비된 프로세서 상태를 저장할 것이다. 이 상태는 `kernel_exit` 매크로가 기대하는 것과 정확히 같은 형식을 가지며 [pt_regs](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/fork.h#L21) 구조체에 의해 묘사된다.

다음은 `move_to_user_mode` 함수에서 초기화되는 `pt_regs` 구조체의 필드들이다.

* `pc`: 이제 `pc`는 사용자 모드에서 실행되어야 할 함수를 가리킨다. `kernel_exit`는 `pc` 값을 `elr_el1` 레지스터에 복사하여 예외 리턴 후 `pc`주소로 돌아가게 한다.
* `pstate` 이 필드는 `kernel_exit` 매크로에 의해 `spsr_el1`에 복사되어 예외가 리턴된 후의 프로세서 상태를 가진다. `pstate` 필드에 복사된 [PSR_MODE_EL0t](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/fork.h#L9) 상수를 준비하는 방식으로 예외는 EL0로 리턴될 수 있다. lesson2에서 EL3에서 EL1으로 전환할 때 이미 동일한 방법을 사용한 적이 있다.
* `stack` `move_to_user_mode` 함수는 사용자 스택을 위한 새로운 페이지를 할당하고 sp 필드를 해당 페이지의 top에 대한 포인터로 설정한다.

`task_pt_regs` 함수는 `pt_regs` 영역의 위치를 계산하는 데 사용된다. 현재의 커널 스레드를 초기화한 방식 때문에 `sp`가 `pt_regs` 영역 바로 앞을 가리킬 것이다. 이 동작은 [ret_from_fork](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/entry.S#L188) 함수에서 일어난다.
```
.globl ret_from_fork
ret_from_fork:
    bl    schedule_tail
    cbz   x19, ret_to_user            // not a kernel thread
    mov   x0, x20
    blr   x19
ret_to_user:
    bl disable_irq
    kernel_exit 0
```
`ret_from_fork` 함수가 업데이트되었다. 이제 커널 스레드가 종료한 후에 `ret_to_user` 레이블을 실행하여 인터럽트를 비활성화하고, 이전에 준비해 둔 프로세서 상태를 사용하여 정상적인 예외 리턴을 수행한다.

### Forking user processes

이제 `kernel.c` 파일로 돌아가자. 이전 섹션에서 보았듯이 `kernel_process` 함수가 끝난 뒤에 [user_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/kernel.c#L22) 함수가 사용자 모드에서 실행된다. 이 함수는 `clone` 시스템 콜을 두 번 호출하여 [user_process1](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/kernel.c#L10) 함수를 2개의 스레드에서 병렬적으로 실행한다. `clone` 시스템 콜은 전달인자로 새로운 사용자 스택의 위치가 필요하며, 두 개의 새로운 메모리 페이지를 할당하기 위해 `malloc` 시스템 콜도 호출해야 한다. 이제 `clone` 시스템 콜 wrapping 함수가 어떻게 생겼는지 [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.S#L22)에서 살펴보자.
```
.globl call_sys_clone
call_sys_clone:
    /* Save args for the child.  */
    mov    x10, x0                    /*fn*/
    mov    x11, x1                    /*arg*/
    mov    x12, x2                    /*stack*/

    /* Do the system call.  */
    mov    x0, x2                     /* stack  */
    mov    x8, #SYS_CLONE_NUMBER
    svc    0x0

    cmp    x0, #0
    beq    thread_start
    ret

thread_start:
    mov    x29, 0

    /* Pick the function arg and execute.  */
    mov    x0, x11
    blr    x10

    /* We are done, pass the return value through x0.  */
    mov    x8, #SYS_EXIT_NUMBER
    svc    0x0
```
`clone` 시스템 콜 wrapping 함수의 설계는 `glibc` 라이브러리의 [대응되는 함수](https://sourceware.org/git/?p=glibc.git;a=blob;f=sysdeps/unix/sysv/linux/aarch64/clone.S;h=e0653048259dd9a3d3eb3103ec2ae86acb43ef48;hb=HEAD#l35)의 동작을 열심히 흉내내보았다.

1. `x0`-`x2` 레지스터를 저장한다. 이 레지스터들은 시스템 콜의 파라미터를 포함하며 나중에 시스템 콜 handler에 의해 덮어씌워진다.
1.  시스템 콜 handler를 호출한다.
1.  시스템 콜의 리턴값을 확인한다. 그 값이 `0`이라면 새로 생성된 스레드에서 실행할 것이며, 이 경우 `thread_start` 레이블로 실행이 옮겨진다.
1. 0이 아닌 값이라면 이는 새로운 task의 PID이다. 이것은 시스템 콜 처리가 끝난 후 호출했던 위치로 돌아올 것이며 기존의 스레드에서 계속 실행한다는 의미이다.
1.  첫 번째 전달인자로 넘겨진 함수가 새로운 스레드에서 호출된다.
1. 이 함수의 수행이 끝난 후, 리턴하는 것이 아니라 `exit` 시스템 콜이 수행된다.

보다시피 clone wrapper 함수와 clone 시스템 콜의 의미는 다르다. Clone wrapper 함수는 전달인자로 실행될 함수에 대한 포인터를 받아서 호출자에게 두 번 리턴하게 된다. 첫 번째 리턴은 기존의 task에서 두 번째 리턴은 cloned task에서 발생한다.

Clone 시스템 콜 핸들러는 [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.c#L11)에 있으며 매우 간단하다. 이미 우리에게 익숙한 `copy_process` 함수를 호출할 뿐이다. `copy_process` 함수는 이전 lesson 이후에 수정되었다. 이제 커널 스레드뿐만 아니라 사용자 스레드를 복제하는 것도 지원한다. 그 코드는 다음과 같다.

```
int copy_process(unsigned long clone_flags, unsigned long fn, unsigned long arg, unsigned long stack)
{
    preempt_disable();
    struct task_struct *p;

    p = (struct task_struct *) get_free_page();
    if (!p) {
        return -1;
    }

    struct pt_regs *childregs = task_pt_regs(p);
    memzero((unsigned long)childregs, sizeof(struct pt_regs));
    memzero((unsigned long)&p->cpu_context, sizeof(struct cpu_context));

    if (clone_flags & PF_KTHREAD) {
        p->cpu_context.x19 = fn;
        p->cpu_context.x20 = arg;
    } else {
        struct pt_regs * cur_regs = task_pt_regs(current);
        *childregs = *cur_regs;
        childregs->regs[0] = 0;
        childregs->sp = stack + PAGE_SIZE;
        p->stack = stack;
    }
    p->flags = clone_flags;
    p->priority = current->priority;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1; //disable preemtion until schedule_tail

    p->cpu_context.pc = (unsigned long)ret_from_fork;
    p->cpu_context.sp = (unsigned long)childregs;
    int pid = nr_tasks++;
    task[pid] = p;
    preempt_enable();
    return pid;
}
```
새로운 커널 스레드를 생성하는 경우, 이전 lesson에서 설명했던 것과 동일하게 동작한다. 사용자 스레드를 복제하는 경우에는 이 코드가 실행된다.
```
        struct pt_regs * cur_regs = task_pt_regs(current);
        *childregs = *cur_regs;
        childregs->regs[0] = 0;
        childregs->sp = stack + PAGE_SIZE;
        p->stack = stack;
```
먼저 `kernel_entry` 매크로에 의해 저장된 프로세서 상태에 접근한다. 여기서 커널 스택의 top에 위치한 `pt_regs` 영역을 반환할 때와 동일한 `task_pt_regs` 함수를 사용할 수 있는 이유는 명확하지 않다. `pt_regs`가 스택의 다른 위치에 저장될 수 없는 이유는 무엇일까? 그 답은 이 코드가 `clone` 시스템 콜이 호출된 후에만 실행될 수 있다는 것이다. 시스템 콜이 발생할 당시의 커널 스택은 비어있었다. 사용자 모드로 이동한 후 비워둔 것이다. 이것이 `pt_regs`가 항상 커널 스택의 top에 저장되는 이유이다. 각 시스템 콜은 사용자 모드로 돌아가기 전에 커널 스택을 비울 것이기 때문에 이 규칙은 모든 시스템 콜에서 유지된다.

두 번째 줄에서 현재의 프로세서 상태가 새로운 task 상태로 복사된다. `x0`은 호출자가 시스템 콜의 리턴값으로 해석하기 때문에 새로운 상태에서 `0`으로 설정된다. Clone wrapper 함수는 이 값을 이용하여 기존 스레드에서 계속 실행할지 아니면 새로운 스레드에서 실행할지를 결정한다.

다음으로 새로운 task의 `sp`가 새로운 사용자 스택 페이지의 top에 대한 포인터로 설정된다. 또한 task가 종료된 후 cleanup하기 위해 스택 페이지에 대한 포인터를 저장한다.

### Exiting a task

각 사용자 task가 종료된 후에는 `exit` 시스템 콜을 호출해야 한다. 현재 구현된 것에서는 `clone` wrapper 함수에 의해 `exit`가 호출된다. `exit`시스템 콜은 task 비활성화를 담당하는 [exit_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sched.c) 함수를 호출한다. 이 함수는 다음과 같다.

```
void exit_process(){
    preempt_disable();
    for (int i = 0; i < NR_TASKS; i++){
        if (task[i] == current) {
            task[i]->state = TASK_ZOMBIE;
            break;
        }
    }
    if (current->stack) {
        free_page(current->stack);
    }
    preempt_enable();
    schedule();
}
```
리눅스의 관례에 따라 한 번에 task를 삭제하지 않는 대신에 상태를 `TASK_ZOMBIE`로 설정한다. 이 상태는 스케줄러에 의해 task가 선택되어 실행되지 않도록 한다. 리눅스에서는 자식 프로세스가 종료한 후 부모 프로세스가 그 정보를 알 수 있도록 하기 위해 이러한 접근방식이 사용된다.

`exit_process`는 불필요한 사용자 스택을 즉시 삭제하고 `schedule` 함수를 호출한다. `schedule` 함수가 호출된 후에 새로운 task가 선택되며, 따라서 시스템 콜은 리턴하지 않게 된다.

### Conclusion

이제 RPi OS는 사용자 스택을 관리할 수 있고, 완전한 프로세스 격리에 훨씬 더 가까워졌다. 하지만 한 가지 중요한 단계가 남아있다. 모든 사용자 task는 동일한 물리 메모리를 공유하여 서로의 데이터를 쉽게 읽을 수 있어야 한다. 다음 lesson에서 가상 메모리를 구현하여 이 문제를 해결해보자.

##### Previous Page

4.5 [Process scheduler: Exercises](../../docs/lesson04/exercises.md)

##### Next Page

5.2 [User processes and system calls: Linux](../../docs/lesson05/linux.md)