## 2.1: 프로세서 초기화

이 레슨에서는 ARM 프로세서와 더욱 밀접하게  작업합니다.  OS에서 활용 할 수 있는 몇 가지 필수 기능이 있습니다. 첫 번째 그러한 기능은 "Exception levels" 입니다.

### Exception levels (예외 레벨)
ARM.v8 아키텍처를 지원하는 각 ARM 프로세서에는 4 개의 Exception level(예외 레벨)이 있습니다. Exception level (또는 간단히 `EL`)은 모든 작업 및 레지스터의 하위 집합 만 사용할 수있는 프로세서 실행 모드로 생각할 수 있습니다. 최소 권한 Exception level은 0 (EL0) 입니다. 프로세서가 이 level에서 작동 할 때는 대부분 범용 레지스터 (X0-X30)와 스택 포인터 레지스터 (SP) 만 사용합니다.  또한 EL0는 `STR` 및`LDR` 명령을 사용하여 사용자 프로그램에서 일반적으로 사용하는 몇 가지 명령어으로 메모리에 데이터를 로드하고 저장할 수 있습니다.

운영 체제는 프로세스 격리를 구현해야하므로 Exception levels(예외 레벨)을 처리해야합니다. 사용자 프로세스는 다른 프로세스의 데이터에 액세스할 수 없어야 합니다. 이러한 동작을 하기 위해 운영 체제는 항상 각 사용자 프로세스를 EL0에서 실행합니다. 이 예외수준에서 실행되는 프로세스는 자신의 가상 메모리만 사용 할 수 있으며 가상 메모리 설정을 변경하는 명령에 접근 할 수 없습니다.  따라서 프로세스 격리를 보장하기 위해 OS는 사용자 프로세스로 실행을 전송하기 전에 각 프로세스에 대해 별도의 가상 메모리 매핑을 준비하고 프로세서를 EL0에 배치해야합니다.

운영 체제 자체는 일반적으로 EL1에서 작동합니다. 이 예외 수준에서 실행되는 동안 프로세서는 일부 시스템 레지스터뿐만 아니라 가상 메모리 설정을 구성 할 수있는 레지스터에 액세스 할 수 있습니다. Raspberry Pi OS도 EL1을 사용합니다.

우리는 예외 레벨(EL) 2와 3을 많이 사용하지 않을 것입니다. 하지만 간단히 설명해서 이것들이 왜 필요한지 여러분이 알 수 있도록 하고 싶습니다.

하이퍼 바이저를 사용하는 시나리오에서 EL2가 사용됩니다. 이 경우 호스트 운영 체제는 EL2에서 실행되며 게스트 운영 체제는 EL 1 만 사용할 수 있습니다. 이렇게하면 OS가 사용자 프로세스를 분리하는 것과 비슷한 방식으로 호스트 OS가 게스트 OS를 격리 할 수 있습니다.

EL3은 ARM "Secure World"에서 "Insecure world"로 전환하는 데 사용됩니다. 이 추상화는 서로 다른 두 "World"에서 실행되는 소프트웨어간에 완전한 하드웨어 격리를 제공하기 위해 존재합니다. "Insecure world"의 응용 프로그램은 "Secure World"에 속하는 정보 (명령 및 데이터 모두)에 액세스하거나 수정할 수 없으며 이러한 제한은 하드웨어 level에서 적용됩니다.

### Debugging the kernel (커널 디버깅)

다음으로 해야 할 일은 현재 사용중인 예외 레벨(`EL`)을 파악하는 것입니다. 그러나 이 작업을 시도했을 때 커널이 화면에 일정한 문자열 만 보여 줄 수 있다는 것을 알게되었습니다. 그래서 이 작업을 위해 필요한 것이 간단한 형식의 [printf](https://en.wikipedia.org/wiki/Printf_format_string) 함수입니다.  `printf` 를 사용하면 다른 레지스터와 변수의 값을 쉽게 표시 할 수 있습니다.  커널 개발에 있어서 다른 디버거의 지원이 없고 , `printf`가 프로그램 내에서 무슨 일이 일어나고 있는지 알아낼 수 있는 유일한 수단이 되기 때문에 이러한 기능은 필수적입니다.

RPi OS의 경우 이미 있는 것을 다시 만드느라 쓸데없이 시간을 낭비하지 않고 [기존 printf 구현](http://www.sparetimelabs.com/tinyprintf/tinyprintf.php) 중 하나를 사용하기로 결정했습니다. 이 기능은 대부분 문자열 조작으로 구성되며 커널 개발자의 관점에서 보면 그리 중요하지 않습니다. 내가 사용한 구현은 매우 작으며 외부 종속성이 없으므로 커널에 쉽게 통합 할 수 있습니다. 내가 해야 할 유일한 것은 화면에 단일 문자를 보낼 수있는`putc` 기능을 정의하는 것입니다. 이 함수는 [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/mini_uart.c#L59) 에 정의되어 있으며 기존의 uart_send 함수 만 사용합니다. 또한 printf 라이브러리를 초기화하고 putc 함수의 위치를 지정해야합니다. 이것은 [한 줄의 코드](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/kernel.c#L8)로 이루어집니다.

### Finding current Exception level (현재 예외 레벨 찾기)

이제 `printf` 기능이 갖춰지면, 원래 작업을 완료 할 수 있습니다. OS가 부팅되는 예외 레벨을 파악하십시오. 이 질문에 대답할 수 있는 작은 기능이 [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/utils.S#L1)  에 정의되어 있다. 

```
.globl get_el
get_el:
    mrs x0, CurrentEL
    lsr x0, x0, #2
    ret
```

위에서 `mrs` 명령어를 사용하여`CurrentEL` 시스템 레지스터의 값을`x0` 레지스터로 읽습니다. 그런 다음 이 값을 2 비트 오른쪽으로 옮깁니다 ( `CurrentEL`레지스터의 처음 2 비트는 예약되어 있고 항상 값 0을 갖기 때문에이 작업이 필요합니다) 마지막으로 레지스터 `x0`에는 현재 예외를 나타내는 정수가 있습니다. 이제 남은 것은 이 값을 표시하는 것뿐입니다.  [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/kernel.c#L10)에서 그 작업을 볼 수 있습니다.

```
    int el = get_el();
    printf("Exception level: %d \r\n", el);
```

이 작업을 재현해보면 화면에 'Exception level : 3'가 표시됩니다.

### Changing current exception level (현재 예외 레벨 변경)

ARM 아키텍처에서는 이미 실행되고 있는 더 높은 레벨의  소프트웨어의 참여 없이는 자체적으로 예외 수준을 증가시키는 방법이 없습니다. 이건 완전 당연한 행동입니다. 그렇지 않으면 어떤 프로그램도 할당된 EL에서 벗어나 다른 프로그램 데이터에 접근할 수 있을 것입니다. 예외가 생성 된 경우에만 현재 EL을 변경할 수 있습니다. 예를 들어, 프로그램이 잘못된 명령을 실행하는 경우 (예 : 존재하지 않는 주소의 메모리 위치에 액세스하거나 0으로 나누려고 시도하는 경우) 발생할 수 있습니다. 또한 응용 프로그램은 `svc` 명령어를 실행하여 의도적으로 예외를 생성 할 수 있습니다. 하드웨어 생성 인터럽트(Hardware generated interrupt) 도 특수한 유형의 예외로 처리됩니다. 예외가 생성 될 때마다 다음과 같은 일련의 단계가 발생합니다 (예외는 EL`n`에서 처리되고`n`은 1, 2 또는 3 일 수 있다고 가정합니다).

1. 현재 명령어의 주소는`ELR_ELn` 레지스터에 저장됩니다. ( `Exception link register` 라고 불림)
2. 현재 프로세서 상태는 `SPSR_ELn` 레지스터 (`Saved Program Status Register`)에 저장됩니다.
3. Exception handler 가 실행되고 필요한 모든 작업을 수행합니다.
4. Exception handler 는`eret` 명령을 호출합니다. 이 명령어는 `SPSR_ELn`에서 프로세서 상태를 복원하고 `ELR_ELn`레지스터에 저장된 주소부터 시작하여 실행을 재개합니다.

실제 Exception handler는 모든 범용 레지스터의 상태를 저장하고 나중에 다시 복원해야 하기 때문에 절차가 조금 더 복잡하지만, 우리는 다음 수업에서 이 과정을 자세히 논의할 것이다. 현재로서는 그 과정을 전반적으로 이해하고 `ER_ELm`과 `SPSR_ELn` 레지스터의 의미를 기억하기만 하면 됩니다.

알아야 할 중요한 것은 Exception handler가 예외가 발생한 동일한 위치로 돌아갈 의무가 없다는 것입니다. `ER_ELm`과 `SPSR_ELn`은 모두 쓰기 가능하며, Exception handler는 원할 경우 수정할 수 있습니다. 코드에서 EL3에서 EL1로 바꾸려고 할 때 이 방법이 유용하게 사용 될 것입니다.

### Switching to EL1 (EL1으로 전환)

엄밀히 말하면, 우리의 운영체제는 EL1로 전환할 의무는 없지만, EL1은 우리에게 있어서 자연스러운 선택입니다.  왜냐면 이 레벨은 모든 일반적인 OS 과제를 실행할 수 있는 적절한 권한을 가지고 있기 때문입니다. 또한 예외 레벨의 전환이 어떻게 작동하는지 알아보는 재미있는 연습이 될 것입니다. 이러한 행동을 하는 [소스 코드](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/boot.S#L17)를 보시죠.


```
master:
    ldr    x0, =SCTLR_VALUE_MMU_DISABLED
    msr    sctlr_el1, x0        

    ldr    x0, =HCR_VALUE
    msr    hcr_el2, x0

    ldr    x0, =SCR_VALUE
    msr    scr_el3, x0

    ldr    x0, =SPSR_VALUE
    msr    spsr_el3, x0

    adr    x0, el1_entry        
    msr    elr_el3, x0

    eret                
```

보시다시피 코드는 대부분 몇 개의 시스템 레지스터 구성으로 구성됩니다. 이제 레지스터들을 하나씩 살펴볼 것입니다. 이렇게하려면 먼저 [AArch64-Reference-Manual](https://developer.arm.com/docs/ddi0487/ca/arm-architecture-reference-manual-armv8-for-armv8-a-architecture-profile)을 다운로드해야합니다. 이 문서는 ARM.v8 아키텍처의 세부 사양이 포함되어 있습니다.

#### SCTLR_EL1, 시스템 제어 레지스터 (EL1), AArch64-Reference-Manual의 2654 페이지.

```
    ldr    x0, =SCTLR_VALUE_MMU_DISABLED
    msr    sctlr_el1, x0        
```

위에서 sctlr_el1 시스템 레지스터의 값을 설정합니다. `sctlr_el1`은 EL1에서 작동 할 때 프로세서의 다른 매개 변수를 구성합니다. 예를 들어, 캐시 사용 여부와 MMU (Memory Mapping Unit)가 켜져 있는지 여부를 가장 중요하게 제어합니다. `sctlr_el1`은 EL1보다 높거나 같은 모든 예외 수준(EL)에서 액세스 할 수 있습니다 (`_el1` 접미사 에서 이를 유추 할 수 있음)

SCTLR_VALUE_MMU_DISABLED 상수는 [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L16)서 정의되며 이 값의 개별 비트는 다음과 같이 정의됨:

* `#define SCTLR_RESERVED                  (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)` `sctlr_el1` 레지스터 설명에서 일부 비트는 `RES1`으로 표시됩니다. 이 비트는 향후 사용을 위해 예약되어 있으며 `1`로 초기화해야합니다.
* `#define SCTLR_EE_LITTLE_ENDIAN          (0 << 25)` [Endianness](https://en.wikipedia.org/wiki/Endianness) 예외. 이 필드는 EL1에서 명시 적 데이터 액세스의 엔디안을 제어합니다. 리틀 엔디안 형식으로 만 작동하도록 프로세서를 구성하려고합니다.
* `#define SCTLR_EOE_LITTLE_ENDIAN         (0 << 24)` 이전 필드와 비슷하지만 EL1 대신 EL 0에서 명시 적 데이터 액세스의 엔디안을 제어합니다
* `#define SCTLR_I_CACHE_DISABLED          (0 << 12)` 명령어 캐시를 비활성화합니다. 우리는 간단하게하기 위해 모든 캐시를 사용하지 않도록 할 것입니다.  데이터와 명령어 캐시에 대한 자세한 정보는 [여기](https://stackoverflow.com/questions/22394750/what-is-meant-by-data-cache-and-instruction-cache)서 찾을 수 있습니다.
* `#define SCTLR_D_CACHE_DISABLED          (0 << 2)` 데이터 캐시 비활성화.
* `#define SCTLR_MMU_DISABLED              (0 << 0)` MMU를 비활성화하십시오. 페이지 테이블을 준비하고 가상 메모리 작업을 시작하는 레슨 6까지 MMU를 비활성화해야합니다.

#### HCR_EL2, 하이퍼 바이저 구성 레지스터 (EL2), AArch64-Reference-Manual의 2487 페이지.

```
    ldr    x0, =HCR_VALUE
    msr    hcr_el2, x0
```

우리는 자체 하이퍼 바이저를 구현하지 않을 것입니다. 다른 설정 중에서도 EL1의 실행 상태를 제어하기 때문에 이 레지스터를 사용해야합니다. 실행 상태는 AArch32가 아니라 AArch64 여야합니다. [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L22)에 구성되어 있습니다.

#### SCR_EL3, AArch64-Reference-Manual의 284 페이지, Secure Configuration Register(SCR) (EL3).

```
    ldr    x0, =SCR_VALUE
    msr    scr_el3, x0
```

이 레지스터는 configuring security settings을 담당합니다. 예를 들어, 모든 하위 레벨이 "secure"또는 "nonsecure"상태에서 실행되는지 여부를 제어합니다. 또한 EL2에서 실행 상태를 제어합니다. [여기서](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L26)  우리는 EL2가 AArch64 상태에서 실행되도록 설정하고 모든 하위 예외 수준은 "nonsecure"이 됩니다.

#### SPSR_EL3, #### Saved Program Status Register (EL3), AArch64-Reference-Manual의 389 페이지

```
    ldr    x0, =SPSR_VALUE
    msr    spsr_el3, x0
```

이 레지스터는 이제 익숙해야 합니다. 예외 수준 변경 프로세스에 대해 논의할 때 언급하였습니다. `spsr_el3`에는 프로세서 상태가 포함되어 있으며, 이 상태는 `erer` 명령을 실행한 후 복원될 것입니다. 프로세서 상태란 무엇인지 간략하게 설명 할 필요가 있습니다.  프로세서 상태에는 다음 정보가 포함됩니다.

* **Condition Flags** 이러한 플래그에는 이전에 실행 된 작업에 대한 정보가 포함됩니다. 결과가 음수 (N 플래그), 0 (A 플래그), 부호없는 오버플로 (C 플래그) 또는 부호있는 오버플로 (V 플래그) 여부. 이러한 플래그의 값은 조건부 분기 명령어에서 사용될 수 있습니다. 예를 들어 b.eq 명령어는 마지막 비교 작업의 결과가 0 인 경우에만 제공된 레이블로 이동합니다. 프로세서는 Z 플래그가 1로 설정되어 있는지 테스트 하여 이를 확인합니다.

* **Interrupt disable bits** 이 비트들은 다른 유형의 인터럽트를 활성화 / 비활성화 할 수 있습니다.
* 예외를 처리한 후 프로세서 실행 상태를 완전히 복원하는 데 필요한 기타 정보.

일반적으로 `spsr_el3`은 EL3에 예외가 발생하면 자동으로 저장됩니다. 그러나이 레지스터는 쓰기 가능하므로이 사실을 활용하고 수동으로 프로세서 상태를 준비합니다. `SPSR_VALUE`는 [여기](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L35) 에서 준비되었으며 다음 필드를 초기화합니다.

* `#define SPSR_MASK_ALL        (7 << 6)`  EL을 EL1로 변경하면 모든 유형의 인터럽트가 마스킹됩니다 (또는 비활성화 됨).
* `#define SPSR_EL1h        (5 << 0)`  EL1에서는 자체 전용 스택 포인터를 사용하거나 EL0 스택 포인터를 사용할 수 있습니다. `EL1h` 모드는 EL1 전용 스택 포인터를 사용하고 있음을 의미합니다.

#### ELR_EL3, Exception Link Register  (EL3), AArch64-Reference-Manual의 351 페이지.

```
    adr    x0, el1_entry        
    msr    elr_el3, x0

    eret                
```

`elr_el3`은 `eret` 명령이 실행 된 후 반환 할 주소를 보유합니다. 여기에서이 주소를 `el1_entry` 레이블의 위치로 설정합니다.

### 결론
우리가`el1_entry` 함수에 들어갈 때 실행은 이미 EL1 모드에 있어야합니다. 시도해보십시오!

##### 이전페이지

1.5 [Kernel Initialization: Exercises](../lesson01/exercises.md)

##### 다음페이지

2.2 [Processor initialization: Linux](../lesson02/linux.md)

##### 추가 사항

이 문서는 훌륭한 오픈소스 운영체제 학습 프로젝트인 Sergey Matyukevich의 문서를 영어에 능숙하지 않은 한국인들이 학습할 수 있도록 번역한 것입니다. 오타 및 오역이 있을 수 있습니다. Sergey Matyukevich는 한국어를 능숙하게 다루지 못합니다. 대신 저에게 elxm6123@gmail.com으로 연락해주세요.
