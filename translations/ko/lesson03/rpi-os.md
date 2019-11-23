## 3.1: 인터럽트

1번 레슨에서 우리는 이미 하드웨어와 통신하는 방법을 알고 있습니다. 그러나 대부분의 경우 의사소통의 패턴은 그렇게 간단하지 않습니다. 일반적으로 이러한 패턴은 비동기식으로, 어떤 명령을 장치에 보내지만, 즉시 응답하지 않습니다. 대신 작업이 완료되면 알려줍니다. 이러한 비동기 알림은 정상적인 실행 흐름을 방해하고 프로세서가 "인터럽트 핸들러"를 실행하도록 하기 때문에 "인터럽트"라고 불립니다.

운영체제 개발에 특히 유용한 장치가 하나 있는데, 바로 시스템 타이머입니다. 일부 사전 정의된 주파수로 프로세서를 주기적으로 인터럽트하도록 구성할 수 있는 장치입니다. 프로세스 스케줄링에 사용되는 타이머의 특정 응용 프로그램입니다. 스케줄러는 각 프로세스가 실행된 기간을 측정하고 이 정보를 사용하여 실행할 다음 프로세스를 선택해야 합니다. 이 측정은 타이머 인터럽트를 기반으로 합니다.

다음 레슨에서는 공정 스케줄에 대해 자세히 이야기하겠지만, 현재로서는 시스템 타이머를 초기화하고 타이머 인터럽트 핸들러를 구현하는 것이 과제입니다.

### 인터럽트 vs 예외(익셉션)

ARM.v8 아키텍처에서, 인터럽트는 익셉션의 더 일반적인 용어입니다. 4가지 유형의 익셉션이 있습니다.

* **Synchronous exception** 이 유형의 예외는 항상 현재 실행된 명령어에 의해 발생합니다. 예를 들어 일부 데이터를 존재하지 않는 메모리 위치에 저장하려면 `str` 명령을 사용할 수 있습니다. 이 경우 동기 예외를 생성합니다. 동기식 예외를 사용하여 `소프트웨어 인터럽트`를 생성할 수도 있습니다. 소프트웨어 인터럽트는 `svc`의 명령어에 의해 의도적으로 발생하는 동기식 예외입니다. 우리는 5과에서 이 기술을 시스템 호출을 구현하기 위해 사용할 것입니다.
* **IRQ (Interrupt Request)** 정상적인 인터럽트들입니다. 항상 비동기적이며, 이는 현재 실행되고 있는 명령어와 아무 관련이 없다는 것을 의미합니다. 동기 예외와는 대조적으로, 프로세서가 아니라 외부 하드웨어에 의해 항상 생성됩니다.
* **FIQ (Fast Interrupt Request)** 이러한 유형의 예외는 "빠른 인터럽트"라고 불리며 예외의 우선순위를 정하는 목적으로만 존재합니다. 일부 인터럽트는 "normal"으로, 다른 인터럽트는 "fast"으로 구성할 수 있습니다. 빠른 인터럽트는 먼저 신호를 받고 별도의 예외 처리자에 의해 처리될 것입니다. 리눅스는 빠른 인터럽트를 사용하지 않으며 우리도 사용하지 않을 것 입니다.
* **SError (System Error)** `IRQ`와 `FIQ` 같이, `SError` 예외는 비동기적이고 외부 하드웨어에 의해 발생합니다. `IRQ`와 `FIQ` 다르게, `SError` 항상 같은 에러 조건을 가르킵니다. [여기](https://community.arm.com/processors/f/discussions/3205/re-what-is-serror-detailed-explanation-is-required)에서 언제 `SError` 발생하는지 확인할 수 있습니다.

### 예외 벡터들

각 예외 유형은 자체 처리기를 필요로 한다. 또한 예외가 발생하는 각 실행 상태에 대해 별도의 핸들러를 정의해야 합니다. 예외 처리 관점에서 흥미있는 4개의 실행 상태가 있습니다. 만약 우리가 EL1에서 일하고 있다면, 그러한 상태는 다음과 같이 정의될 수 있습니다.

1. **EL1t** 스택 포인터가 EL0와 공유되는 동안 EL1으로부터 예외가 발생합니다. `SPSel` 레지스터가 `0`의 값을 갖을 때 발생합니다.
1. **EL1h** EL1에 전용 스택 포인터가 할당되었을 때 EL1에서 예외가 발생합니다. 이것은 `SPSel`이 값 `1`을 유지하고 있으며 이것이 현재 사용하고 있는 모드라는 것을 의미합니다.
1. **EL0_64** 64비트 모드에서 실행 중인 EL0에서 예외가 발생.
1. **EL0_32** 32비트 모드에서 실행 중인 EL0에서 예외가 발생.

총 16개(4개의 예외 레벨에 4개의 실행 상태를 곱한 것)의 예외 핸들러를 정의해야 합니다. 모든 예외 핸들러의 주소를 보유하는 특수 구조를 예외 `벡터 테이블` 또는 `벡터 테이블`이라고 합니다. 벡터 테이블의 구조는 [AArch64-Reference-Manual](https://developer.arm.com/docs/ddi0487/ca/arm-architecture-reference-manual-armv8-for-armv8-a-architecture-profile) 1876페이지의 `Table D1-7 Vector offsets from vector table base address`에 정의되어 있습니다. 벡터 표를 예외 벡터의 배열로 생각할 수 있습니다. 여기서 각 예외 벡터(또는 핸들러)는 특정 예외를 취급하는 연속적인 일련의 명령입니다. 따라서, `Table D1-7`에서 `AArch64-Reference-Manual`까지, 각 예외 벡터는 최대 `0x80` 바이트를 사용할 수 있다. 이것은 많지 않지만, 예외적인 벡터에서 다른 메모리 위치로 뛰어드는 것을 막는 사람은 아무도 없습니다.

이 모든 것이 훨씬 더 명확해질 것이라고 생각하므로, 이제 RPI-OS에서 예외 벡터가 어떻게 구현되는지 볼 때가 되었습니다.
예외 처리와 관련된 모든 것은 [entry.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S)에 정의되어 있고 지금 설명할 것입니다.

제일 유용한 매크고는 [ventry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S#L12)라고 불리고 이것은 벡터 테이블 안에서 새로운 앤트리를 정의하기 위해서 사용됩니다.

```
    .macro    ventry    label
    .align    7
    b    \label
    .endm
```

이 정의에서 유추할 수 있듯이, 우리는 예외 벡터 안에서 바로 예외를 다루지 않고, 대신 우리는 `라벨` 인수로 매크로에 제공된 라벨로 점프합니다. 모든 예외 벡터는 0x80 바이트의 오프셋으로 배치해야 하므로 `.align 7`의 명령어가 필요합니다.

벡터 테이블은 [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S#L64)에 정의되어 있고 이것은 16개의 `venrty` 정의로 구성되어 있습니다. 현재로서는 `EL1h`로부터 `IRQ`를 취급하는 데만 관심이 습니다. 그러나 여전히 16개의 핸들러 정의가 필요합니다. 이는 일부 하드웨어 요구 사항 때문이 아니라, 문제가 발생할 경우 의미 있는 오류 메시지를 보고 싶기 때문입니다. 일반적인 흐름 안에서 수행되어서는 안되는 모든 핸들러는 `invalid` 접두사를 갖고 있고 이것은 [handle_invalid_entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S#L3) 매크로를 사용합니다. 이 매크로가 어떻게 정의되는지 살펴봅시다.

```
    .macro handle_invalid_entry type
    kernel_entry
    mov    x0, #\type
    mrs    x1, esr_el1
    mrs    x2, elr_el1
    bl    show_invalid_entry_message
    b    err_hang
    .endm
```

첫번째 줄에서는, `kernel_entry`라는 다른 매크로가 사용되는 것을 볼 수 있습니다. 나중에 간단히 논의할 예정입니다. 그때에 [show_invalid_entry_message](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.c#L34)을 호출할 수 있고 이는 3개의 전달인자를 준비합니다. 첫번째 전달인자는 [이들](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/include/entry.h#L6) 값의 하나가 취할 수 있는 예외 유형입니다. 그것은 우리에게 어떤 예외 처리기가 실행되었는지 정확히 말해줍니다. 두 번째 매개 변수는 예외 신드롬 레지스터의 약자인 `ESR`이다. 이 전달인자는 `AArch64-Reference-Manual` 2431페이지에 설명되어 있는 `esr_el1` 레지스터로부터 취득됩니다. 이 레지스터에는 예외를 유발하는 요소에 대한 세부 정보가 포함되어 있습니다. 세 번째 전달인자는 대부분 동기 예외의 경우에 중요합니다. 예외 발생 시 실행된 지침의 주소가 포함된 `er_el1` 레지스터에서 이미 익숙한 값입니다. 동기 예외의 경우, 이것은 예외를 유발하는 명령어이기도 합니다. `show_invalid_entry_message` 함수가 모든 정보를 화면에 출력한 이후에 할 수 있는 것이 얼마 없으므로 프로세서를 무한 루프 상태로 둡니다.

### 레지스터 상태 저장

예외 처리기가 실행을 마친 후, 우리는 모든 일반 목적 레지스터가 예외를 생성하기 전에 가졌던 동일한 값을 갖기를 원합니다. 만약 우리가 그러한 기능을 구현하지 않는다면, 현재 코드를 실행하는 것과 무관한 중단은 이 코드의 동작에 예상치 못하게 영향을 미칠 수 있습니다. 그렇기 때문에 예외를 만든 후에 가장 먼저 해야 할 일은 프로세서 상태를 저장하는 것입니다. 이는 [kernel entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S) 매크로 안에서 수행됩니다. 이 매크로는 매우 간단합니다. `x0 - x30` 레지스터 값들을 스택에 저장합니다. 또한 예외 핸들러가 수행된 이후 호출되는 상응하는 [kernel exit](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S#L37) 매크로가 있습니다. `kernel_exit`는 `x0 - x30` 레지스터를 복사하고 복원함으로서 프로세서의 상태를 복원합니다. 또 정상적인 실행 흐름으로 되돌아가는 `eret` 명령도 실행합니다. 그런데, 범용 레지스터만 예외 핸들러를 실행하기 전에 저장해야 하므로, 현재로서는 우리의 간단한 커널로 충분하다. 마지막 레슨에서, `kernel_entry`와 `kernel_exit` 매크로를 추가할 것이다.

### 벡터 테이블 설정하기 

이제 벡터 테이블을 준비했는데 프로세서는 이게 어디에 있는지 몰라서 사용할 수 없습니다. 예외 처리가 작동하려면 벡터 테이블 주소에 `vbar_el1`(Vector Base Address Register)을 설정해야 한다. 이는 [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.S#L2)에서 행해진다.

```
.globl irq_vector_init
irq_vector_init:
    adr    x0, vectors        // load VBAR_EL1 with virtual
    msr    vbar_el1, x0        // vector table address
    ret
```

### 인터럽트 마스킹/언마스킹

우리가 해야 할 또 다른 일은 모든 종류의 인터럽트를 푸는 것입니다. 인터럽트를 "unmasking" 것이 무슨 뜻인지 설명하겠습니다. 때때로 특정한 코드 조각이 비동기 방해에 의해 절대로 가로채지 않아야 한다는 것을 말할 필요가 있습니다. 예를 들어, `kernel_entry` 매크로의 바로 한가운데에서 인터럽트가 발생하면 어떻게 될까요? 이 경우 프로세서 상태는 덮어쓰고 손실될 수 있습니다. 그렇기 때문에 예외 처리기가 실행될 때마다 프로세서는 모든 유형의 인터럽트를 자동으로 비활성화해야합니다. 이것을 "masking"이라고 하는데, 이것 또한 우리가 필요하다면 수동으로 할 수 있다. 많은 사람들은 인터럽트가 예외 처리자의 전체 기간 동안 마스크를 착용해야 한다고 잘못 생각합니다. 이는 사실이 아닙니다. 프로세서 상태를 저장한 후 인터럽트를 마스킹 해제하는 것은 완전히 합법적이며, 따라서 인터럽트를 내포한 것도 합법적입니다. 지금 이것을 하지 않을 것이지만, 이것은 명심해야 할 중요한 정보입니다.

[뒤의 2함수는](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.S#L7-L15) 인터럽트들을 마스킹과 언마스킹합니다.

```
.globl enable_irq
enable_irq:
    msr    daifclr, #2
    ret

.globl disable_irq
disable_irq:
    msr    daifset, #2
        ret
```

ARM 프로세서 상태에는 서로 다른 유형의 인터럽트에 대한 마스크 상태를 유지하는 역할을 하는 4비트가 있습니다. 그 비트는 다음과 같이 정의됩니다.

* **D**  마스크 디버그 예외. 이것들은 동기식 예외의 특별한 유형입니다. 분명한 이유로 모든 동기 예외를 마스킹할 수는 없지만 디버그 예외를 마스킹할 수 있는 별도의 플래그가 있으면 편리합니다.
* **A** `SErrors`를 마스크한다. `SErrors`를 비동기적 중단이라고 부르기도 해서 `A`라고 부른다.
* **I** `IRQs`를 마스크한다.
* **F** `FIQs`를 마스크한다.

이제 인터럽트 마스크 상태 변경을 담당하는 레지스터를 `daifclr`와 `daifset`이라고 부르는 이유를 짐작할 수 있을 것입니다. 이러한 레지스터는 프로세서 상태에서 인터럽트 마스크 상태 비트를 설정 및 삭제합니다.

우리가 왜 두 가지 함수 모두에서 상수치인 `2`를 사용하는지 궁금하지 않는가? 두번째 (`I`) 비트만 세팅하고 싶기 때문입니다.

### 인터럽트 컨트롤러 구성하기 

기기들은 대개 프로세서를 직접 중단하지 않습니다. 대신, 그들은 일을 하기 위해 인터럽트 컨트롤러에 의존합니다. 인터럽트 컨트롤러는 하드웨어가 전송하는 인터럽트를 활성화/비활성화하는 데 사용할 수 있습니다. 인터럽트 컨트롤러를 사용하여 어떤 장치가 인터럽트를 생성하는지 알아낼 수도 있습니다. Raspberry PI에는 [BCM2837 ARM 주변기기 매뉴얼](https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf)의 109페이지에 설명된 자체 인터럽트 컨트롤러가 있습니다.

라스베리 파이 인터럽트 컨트롤러에는 모든 유형의 인터럽트에 대해 활성화/비활성화 상태를 유지하는 레지스터 3개가 있습니다. 현재는 타이머 인터럽트에만 관심이 있습니다. 타이머 인터럽트는 `BCM2837 ARM Peripherals manual`의 116페이지에 설명되어 있는 [ENABLE_IRQS_1](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/include/peripherals/irq.h#L10) 레지스터를 사용하여 활성화 할 수 있습니다. 문서에 따르면, 인터럽트들은 두 개의 뱅크로 나뉩니다. 첫번째 뱅크는 `0 - 31`의 인터럽트 들로 구성되고 `ENBLE)IRQS_1` 레지스터의 다른 비트들을 셋팅함으로써 각각은 활성화되기도 비활성화되기도 합니다. 32개 인터럽트-`ENABLE_IRQS_2`를 위한 상응하는 레지스터와 ARM 로컬 인터럽트와 함께 일반적인 인터럽트를 제어하는 레지스터 - `ENABLE_BASIC IRQS`들이 존재합니다. (이 레슨의 다음 장에서는 ARM 로컬 인터럽트에 대해 이야기하겠습니다) 그러나 주변기기 매뉴얼은 많은 실수를 가지고 있고 그 중 하나는 우리의 논의와 직접적으로 관련이 있습니다. 주변 인터럽트 테이블(매뉴얼 113페이지에 설명됨)은 `0 - 3` 라인에서 시스템 타이머의 인터럽트 4개를 포함해야 한다. 리눅스 소스 코드 리버스 엔지니어링과 [몇몇 다른 소스](http://embedded-xinu.readthedocs.io/en/latest/arm/rpi/BCM2835-System-Timer.html)를 읽은 것으로부터 나는 타이머 인터럽트 0과 2가 GPU에 의해 예약되고 사용되며 인터럽트 1과 3은 어떤 다른 용도로도 사용될 수 있다는 것을 알아낼 수 있었습니다. 타이머 인터럽트 0과 2가 GPU에 의해 예약되고 사용되며 인터럽트 1과 3은 어떤 다른 용도로도 사용될 수 있다는 것을 알아낼 수 있었습니다. 그래서 여기에 시스템 타이머를 IRQ 숫자 1을 가능하게 하는 [함수](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.c#L29)가 있습니다.

```
void enable_interrupt_controller()
{
    put32(ENABLE_IRQS_1, SYSTEM_TIMER_IRQ_1);
}
```

### 포괄적인 IRQ 핸들러

이전의 토론에서, 당신은 우리가 모든 `IRQ들`을 처리하는 단일 예외 처리 핸들러를 가지고 있다는 것을 기억해야 합니다. 이 핸들러는 [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.c#L39)에 정의되어 있습니다.

```
void handle_irq(void)
{
    unsigned int irq = get32(IRQ_PENDING_1);
    switch (irq) {
        case (SYSTEM_TIMER_IRQ_1):
            handle_timer_irq();
            break;
        default:
            printf("Unknown pending irq: %x\r\n", irq);
    }
}
```

핸들러에서, 우리는 어떤 장치가 인터럽트를 발생시키는지 알아낼 방법이 필요합니다. 인터럽트 컨트롤러는 우리에게 이 작업을 도와줄 수 있습니다. 인터럽트 컨트롤러는 `0 - 31` 인터럽트를 위한 인터럽트 상태 저장하는 `IRQ_PENDING_1` 레지스터를 갖고 있습니다. 이 레지스터를 사용하여 현재 인터럽트가 타이머에 의해 생성되었는지 또는 다른 장치 및 호출 장치별 인터럽트 핸들러에 의해 생성되었는지 확인할 수 있습니다. 여러 인터럽트가 동시에 보류될 수 있다는 점에 유의하십시오. 그렇기 때문에 각 기기별 인터럽트 핸들러는 인터럽트 처리를 완료하고 그 후에 `IRQ_PENDING_1`의 인터럽트 비트가 삭제된다는 것을 알아야합니다.

동일한 이유로, 생산 준비 OS의 경우 인터럽트 핸들러의 스위치 구조를 루프에 넣고 싶을 것입니다. 이렇게 하면 단일 핸들러 실행 중에 여러 개의 인터럽트를 처리할 수 있게 됩니다.

### 타이머 초기화 

라스베리 파이 시스템 타이머는 매우 간단한 장치입니다. 매 시계 체크 후 1씩 값을 올리는 카운터를 가지고 있습니다. 또한 인터럽트 컨트롤러에 연결되는 4개의 인터럽트 라인과 4개의 해당 비교 레지스터가 있습니다. 카운터의 값이 비교 레지스터 중 하나에 저장된 값과 같아지면 해당 인터럽트가 실행됩니다. 그렇기 때문에 시스템 타이머 인터럽트를 사용하기 전에 0이 아닌 값으로 비교 레지스터 중 하나를 초기화해야 하는데, 값이 클수록 인터럽트는 나중에 생성됩니다. 이는 [timer_init](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/timer.c#L8) 함수 안에서 수행됩니다.

```
const unsigned int interval = 200000;
unsigned int curVal = 0;

void timer_init ( void )
{
    curVal = get32(TIMER_CLO);
    curVal += interval;
    put32(TIMER_C1, curVal);
}
```

첫 번째 라인은 전류 카운터 값을 읽고, 두 번째 라인은 이를 증가시키며, 세 번째 라인은 인터럽트 번호 1에 대한 비교 레지스터 값을 설정합니다. `interval` 값을 조작하면 첫 번째 타이머 인터럽트가 생성되는 시간을 조정할 수 있습니다.

### 타이머 인터럽트 핸들링

마침내, 우리는 타이머 인터럽트 핸들러에 도달했다. 그것은 사실 매우 간단합니다.

```
void handle_timer_irq( void )
{
    curVal += interval;
    put32(TIMER_C1, curVal);
    put32(TIMER_CS, TIMER_CS_M1);
    printf("Timer iterrupt received\n\r");
}
```

여기서는 먼저 비교 레지스터를 업데이트하여 동일한 시간 간격 후에 다음 인터럽트가 생성되도록 하십시오. 다음으로 `TIMER_CS` 레지스터에 1을 써넣어 인터럽트를 인정합니다. 문서에서 `TIMER_CS`는 "타이머 제어/상태" 레지스터라고 합니다. 이 레지스터의 비트 [0:3]는 사용 가능한 4개의 인터럽트 라인 중 하나에서 발생하는 인터럽트를 확인하는 데 사용할 수 있습니다.

### 결론

마지막으로 살펴봐야할 것은 [kernel_main](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/kernel.c#L7) 함수입니다. 그곳에 모든 이전에 논의됐던 함수들이 조화됩니다. 샘플을 컴파일하고 실행한 후 인터럽트가 실행된 후 "Timer interrupt received"(타이머 인터럽트 수신됨) 메시지를 출력해야 합니다. 혼자서 그것을 하도록 노력하고 코드를 주의 깊게 검사하고 그것을 실험하는 것을 잊지마세요.

##### 이전 페이지

2.3 [Processor initialization: Exercises](../../docs/lesson02/exercises.md)

##### 다음 페이지

3.2 [Interrupt handling: Low-level exception handling in Linux](../../docs/lesson03/linux/low_level-exception_handling.md)
