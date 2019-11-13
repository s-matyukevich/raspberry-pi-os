## 1.1: RPi OS 소개, 베어-메탈 “Hello, World”
“Hello, World”라는 작은 베어메탈 애플리케이션으로 OS 개발하는 여정을 시작할 예정입니다. OS 개발에 필요한 [필수 요건](../Prerequisites.md)을 모두 준비했다고 생각합니다. 그렇지 않다면 이것을 먼저하세요.

진행하기 앞서 나는 간단한 명명 규칙을 세우려 합니다. README 파일에서 전체 자습서가 레슨으로 나누어져 있음을 알 수 있습니다. 각 레슨은 챕터라고 부르는 개별 파일로 구성되어 있습니다(지금 당신은 1장 1.1절을 읽고 있습니다). 한 챕터는 제목이 있는 섹션으로 나뉩니다. 이 규칙은 자료의 다른 부분에 대해 언급하게 해줍니다.

또 하나 주목했으면 하는 것은 자습서에 소스 코드 샘플이 많이 들어있다는 것입니다. 보통 완전한 코드 블록을 제공함으로써 설명을 시작하고 그다음 라인별로 설명합니다.

### 프로젝트 구성

각 수업의 소스 코드는 구조가 같습니다. 이 수업의 소스 코드는 [여기서](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src/lesson01) 찾을 수 있습니다. 이 폴더의 구성 요소에 대해서 간략하게 설명하겠습니다.
1. **Makefile** 우리는 [make](http://www.math.tau.ac.il/~danha/courses/software1/make-intro.html) 유틸리티를 사용하여 커널을 만들 것입니다. make의 동작은 소스 코드를 컴파일 하고 연결하는 방법에 대한 지침을 표시하는 Makefile에 의해 구성됩니다.
1. **build.sh 또는 build.bat** 도커를 사용하여 커널을 작성하려면 이 파일이 필요할 것입니다. 당신의 노트북에 메이킹 유틸리티나 컴파일러 툴 체인을 설치할 필요가 없을 것입니다.
1. **src** 이 폴더에는 모든 소스코드가 포함되어 있습니다.
1. **include** 모든 헤더 파일이 이 폴더에 있습니다.

### Makefile

이제 프로젝트의 Makefile을 자세히 살펴봅시다. 메이킹 유틸리티의 주된 목적은 프로그램의 어떤 조각을 다시 컴파일 해야 하는지를 자동적으로 결정하고 그들을 다시 컴파일 하는 명령을 내리는 것입니다. 만약 당신이 make와 makefile들에 익숙하지 않다면, [이것을](http://opensourceforu.com/2012/06/gnu-make-in-detail-for-beginners/)  읽을 것을 추천합니다. 첫 번째 레슨에 사용된 Makefile은 [여기에서](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/Makefile) 찾을 수 있습니다. 전체 Makefile은 아래에 나열되어 있습니다.

```
ARMGNU ?= aarch64-linux-gnu

COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMOPS = -Iinclude 

BUILD_DIR = build
SRC_DIR = src

all : kernel8.img

clean :
    rm -rf $(BUILD_DIR) *.img 

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
    mkdir -p $(@D)
    $(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
    $(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
    $(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/kernel8.elf  $(OBJ_FILES)
    $(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img
``` 

이제 파일을 자세히 살펴봅시다.

```
ARMGNU ?= aarch64-linux-gnu
```

Makefile은 변수 정의로 시작합니다. `ARMGNU`는 크로스 컴파일러 접두사입니다. `x86` 머신에서 `arm64` 아키텍처의 소스 코드를 컴파일하고 있기 때문에 [크로스 컴파일러](https://en.wikipedia.org/wiki/Cross_compiler)가 필요합니다. 그래서 `gcc` 대신 `arch64-linux-gnu-gcc`를 사용할 것입니다.

```
COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMOPS = -Iinclude 
```

`COPS`와 `ASMOPS`는 각각 C와 어셈블리 코드를 컴파일 할 때 컴파일러에게 전달하는 옵션입니다. 아래 옵션들은 짧은 설명이 필요합니다.

* **-Wall** 모든 경고들을 보여줍니다.
* **-nostdlib** C 표준 라이브러리를 사용하지 않습니다. C표준 라이브러리의 대부분의 호출은 결국 운영체제와 상호작용한다. 우리는 베어-메탈 프로그램을 사용하고 있기 때문에 운영체제가 없으므로 C 표준 라이브러리는 동작하지 않습니다.
* **- ffreestanding** freestanding 환경은 표준 라이브러리가 존재하지 않을 수 있고 프로그램 시작이 반드시 main이 아닐 수 있는 환경입니다. 옵션 `-ffreestanding`은 컴파일러에게 표준 함수가 통상적인 정의를 가지고 있다고 가정하지 않도록 지시합니다.
* **-linclude** `include` 폴더에서 헤더 파일을 검색합니다.
* **-mgeneral-regs-only** 범용레지스터만 사용합니다. ARM 프로세서에도 [NEON](https://developer.arm.com/technologies/neon) 레지스터가 있습니다. 컴파일러는 복잡성을 더하기 때문에 컴파일러에서 이러한 레지스터를 사용하지 않도록 해야합니다.


```
BUILD_DIR = build
SRC_DIR = src
```

`SRC_DIR`와 `BUILD_DIR`는 소스코드와 컴파일된 오브젝트 파일들을 반복적으로 포함하고 있는 디렉토리들입니다.

```
all : kernel8.img

clean :
    rm -rf $(BUILD_DIR) *.img 
```

다음으로 Makefile에서 타겟을 만듭니다. 첫 번째 두 개의 타겟은 매우 간단합니다. 'all' 타겟은 기본 타겟이며, 아무 인자 없이 'make'를 입력할 때마다 실행됩니다("make"는 항상 첫 번째 타겟을 기본 타겟으로 사용합니다). 이 대상은 모든 작업을 다른 대상인kernel8.img로 리디렉션할 뿐입니다. 'clean' 대상은 모든 컴파일된 아티팩트와 컴파일된 커널 이미지를 삭제하는 역할을 합니다.

```
$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
    mkdir -p $(@D)
    $(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
    $(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@
```

다음 두 타겟은 C와 어셈블리 파일을 컴파일하는 일을 담당합니다. 예를 들어, `src` 디렉토리에 `foo.c`와 `foo.S`가 있습니다. 그 파일들은 각각 `build/foo_c.o`와 `build/foo_s.o`로 컴파일 됩니다. `$<`와 `$@`는 런타임에 입력과 출력 파일 이름(`foo.c`와 `foo_c.o`)로 대체됩니다. C 파일을 컴파일하기 전에 아직 존재하지 않을 경우를 대비해 `build` 디렉토리를 만듭니다.

```
C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)
```

여기서는 C와 어셈블리 소스 파일의 결합으로 만들어진 모든 `OBJ_FILES`의 배열을 만들고 있습니다.

```
DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)
```

다음 두 줄은 좀 까다롭습니다. C와 어셈블리 소스 파일의 컴파일 목표를 어떻게 정의했는지 살펴보면, 우리가 `-MMD` 매개변수를 사용했다는 것을 알 수 있습니다. 이 매개변수는 생성된 각 개체 파일에 대한 모든 종속성을 정의합니다. 이러한 종속성에는 일반적으로 포함된 모든 헤더의 목록이 포함됩니다. 헤더가 변경될 경우 정확히 무엇을 다시 컴파일해야 하는지 알 수 있도록 생성된 모든 종속 파일을 포함해야합니다.
```
$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o kernel8.elf  $(OBJ_FILES)
``` 

우리는 `kernel8.elf` 파일을 빌드하기 위해서 `OBJ_FILES` 배열을 사용합니다. 우리는 결과적으로 실행가능한 이미지의 기본적인 레이아웃을 정의하기 위해서 링커 스크립트 `src/linker.ld`를 사용합니다(다음 섹션에서 링커 스크립트에 대해 논의합니다).

```
$(ARMGNU)-objcopy kernel8.elf -O binary kernel8.img
```

`kernel8.elf`는 [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) 형식입니다. 문제는 ELF 파일이 운영 체제에 의해 실행되도록 설계되어 있다는 것입니다. 베어메탈 프로그램을 작성하려면 ELF 파일에서 실행 파일과 데이터 섹션을 모두 추출해 'kernel8.img' 이미지에 넣어야 합니다. `8`은 64비트 아키텍처인 ARMv8을 의미합니다. 이 파일 이름은 프로세서를 64비트 모드로 부팅하도록 펌웨어에 알려줍니다. 또한 `config.txt` 파일에서 `arm_control=0x200` 플래그를 사용하여 CPU를 64비트 모드로 부팅할 수도 있습니다. RPi OS 이전에 이 방법을 사용했으며, 일부 exercise의 답변에서도 여전히 이 방법을 찾을 수 있습니다. 그러나 `arm_control` 프래그 문서화 되지 않았으며 대신 `kernel8.img` 명명 규칙을 사용하는 것이 바람직합니다.

### 링커 스크립트

링커 스크립트의 주요 목적은 입력 객체 파일(`_c.o와 `_s.o`)의 섹션을 출력 파일(`elf`)에 매핑하는 방법을 설명하는 것입니다. 링커 스크립트에 대한 자세한 내용을 [여기](https://sourceware.org/binutils/docs/ld/Scripts.html#Scripts)에서 알 수 있습니다. 이제 RPi OS 링커 스크립트를 살펴봅시다.

```
SECTIONS
{
    .text.boot : { *(.text.boot) }
    .text :  { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    . = ALIGN(0x8);
    bss_begin = .;
    .bss : { *(.bss*) } 
    bss_end = .;
}
``` 

시작 후 라즈베리 파이는 `kernel8.img`를 메모리에 로드하고 이미지 파일 시작 부분부터 실행을 시작합니다. 그래서 `.text.boot` 섹센이 먼저 있어야합니다. 이 섹션 안에 OS의 시작 코드를 넣겠습니다. `.text`, `.rodata`, `.data` 섹션에는 커널을 컴파일한 명령어, 읽기 전용 데이터, 일반 데이터 등이 수록되어 있어 특별한 추가 사항은 없습니다. 
`.bss` 섹션에는 0으로 초기화해야할 데이터들이 들어 있습니다. 이러한 데이터들을 별도의 섹션에 배치함으로써 컴파일러는 ELF 이진 수에 약간의 공간을 절약할 수 있습니다. 단, 섹션 크기는 ELF 헤더에 저장되지만 섹션 자체는 생략됩니다. 이미지를 메모리에 로드한 이후, `.bss` 섹션을 0으로 초기화해야합니다. 그렇기 때문에 섹션의 시작과 끝(`bss_begin`과 `bss_end`)을 기록하고 섹션이 8의 배수로 시작되도록 정렬해야합니다. 이 구간이 정렬되지 않으면 8바이트 정렬 주소로만 `str`을 사용할 수 있기 때문에 `bss` 섹션의 시작 부분에 0을 저장하는 `str`명령어를 사용하는 것이 더 어려울 것입니다.


### Booting the kernel

이제 [boot.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/src/boot.S) 파일을 볼 시간입니다. 이 파일은 다음과 같은 커널 시작 코드가 포함되어 있습니다.

```
#include "mm.h"

.section ".text.boot"

.globl _start
_start:
    mrs    x0, mpidr_el1        
    and    x0, x0,#0xFF        // Check processor id
    cbz    x0, master        // Hang for all non-primary CPU
    b    proc_hang

proc_hang: 
    b proc_hang

master:
    adr    x0, bss_begin
    adr    x1, bss_end
    sub    x1, x1, x0
    bl     memzero

    mov    sp, #LOW_MEMORY
    bl    kernel_main
```
이 파일을 자세히 검토해봅시다.
```
.section ".text.boot"
```
먼저, `boot.S` 안에 모든 것은 `.text.boot` 섹션 안에 정의되어 있다고 명시합니다. 이전에 이 섹션이 링커 스크립트에 의해 커널 이미지의 시작 부분에 배치되는 것을 보았습니다. 따라서 커널이 시작되면 `start` 함수에서 실행이 시작됩니다.
```
.globl _start
_start:
    mrs    x0, mpidr_el1        
    and    x0, x0,#0xFF        // Check processor id
    cbz    x0, master        // Hang for all non-primary CPU
    b    proc_hang
```
이 함수가 하는 첫번째 일은 프로세서 ID를 확인하는 것입니다. 라즈베리 파이 3는 4개의 코어 프로세서를 가지고 있으며, 장치 전원을 켠 후에 각 코어가 동일한 코드를 실행하기 시작합니다. 그러나 네 개의 코어를 가지고 동작하고 싶지 않습니다. 첫번째 코어만 작동시키고 싶기 때문에 다른 모든 코어들에게 무한 루프를 실행 시킵니다. 바로 이것이 `_start` 함수가 담당하는 것입니다. 이것은 [mpider_el1](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0500g/BABHBJCI.html) 시스템 레지스터로부터 프로세서 ID를 얻습니다. 현재 프로세스 ID가 0이라면, 실행은 `master` 함수로 이행됩니다.

```
master:
    adr    x0, bss_begin
    adr    x1, bss_end
    sub    x1, x1, x0
    bl     memzero
```
여기서 memzero를 불러 `.bss`구간을 클린합니다. 나중에 이 함수를 정의할 것입니다. ARMv8 아키텍처에서는 관례에 따라 처음 7개의 인수가 레지스터 x0-x6을 통해 호출된 함수에 전달됩니다. `memzero` 함수는 시작주소 (`bss_begin`)와 클린해야할 구간 크기(`bss_end-bss-begin`)의 두 가지 인수(x0,x1)만 허용합니다.

```
    mov    sp, #LOW_MEMORY
    bl    kernel_main
```

`.bss` 섹션을 클리닝한 이후에 우리는 스택 포인터를 초기화하여 실행 상태를 `kernel_main` 함수로 전달합니다. 라즈베리 파이는 주소0에서 커널을 로드합니다. 그렇기 때문에 초기 스택 포인터를 충분히 높게 설정하여 스택이 커널 이미지를 오버라이드하지 않도록 해야합니다. `LOW_MEMORY`는 [mm.h](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/include/mm.h)에 정의되어 있으며, 4MB와 같습니다. 커널의 스택은 늘어나지 않을 것이고 이미지 자체가 작기 때문에 4MB는 우리에게 충분하고도 남습니다.

ARM 어셈블리 구문에 익숙하지 않은 사람들을 위해 우리가 사용하는 명령어들을 요약해 봅시다.

* [**mrs**](http://www.keil.com/support/man/docs/armasm/armasm_dom1361289881374.htm) 시스템 레지스터에서 범용 레지스터 중 하나로 값을 로드한다(x0–x30).
* [**and**](http://www.keil.com/support/man/docs/armasm/armasm_dom1361289863017.htm) 논리적인 AND 연산을 수행한다. 이 명령을 사용하여 `mpidr_el1` 레지스터에서 얻은 값에서 마지막 바이트를 떼어낸다.
* [**cbz**](http://www.keil.com/support/man/docs/armasm/armasm_dom1361289867296.htm) 이전에 실행한 작업 결과를 0과 비교하고 결과가 참이면 제공된 레이블로 점프(ARM에선 branch)한다.
* [**b**](http://www.keil.com/support/man/docs/armasm/armasm_dom1361289863797.htm) 일부 레이블에 대해 무조건 분기를 수행한다.
* [**adr**](http://www.keil.com/support/man/docs/armasm/armasm_dom1361289862147.htm) 레이블의 상대 주소를 타겟 레지스터에 로드한다. 이 경우 `bss` 영역의 시작과 끝의 포인터를 원한다.
* [**sub**](http://www.keil.com/support/man/docs/armasm/armasm_dom1361289908389.htm) 두 레지스터에서 값을 빼라.
* [**bl**](http://www.keil.com/support/man/docs/armasm/armasm_dom1361289865686.htm) “링크가 있는 브랜치”: 무조건 분기를 수행하고 반환 주소를 x30(링크 레지스터)에 저장하라. 서브루틴이 완료되면 `ret`명령을 사용하여 다시 리턴 주소로 점프하라.
* [**mov**](http://www.keil.com/support/man/docs/armasm/armasm_dom1361289878994.htm) 레지스터 간 또는 상수에서 레지스토러 값을 이동하라.

[여기](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0024a/index.html) ARMv8-A 개발자 가이드가 있다. ARM ISA가 익숙하지 않다면 좋은 자원이다. [이 페이지](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0024a/ch09s01s01.html)는 특히 ABI의 레지스터 사용 규칙을 개략적으로 설명한다.

### `kernel_main` 함수

우리는 부팅 코드가 결국 `kernel_main` 함수로 제어를 전달한다는 것을 알았다. 한번 보자.

```
#include "mini_uart.h"

void kernel_main(void)
{
    uart_init();
    uart_send_string("Hello, world!\r\n");

    while (1) {
        uart_send(uart_recv());
    }
}

```

이 함수는 커널 안에 있는 가장 간단한 함수 중의 하나입니다. `Mini UART` 기기와 연동하여 스크린에 출력하고 사용자 입력 내용을 읽습니다. 이 커널은 `Hello, World!`를 출력하고 사용자로부터 문자를 읽고 다시 화면으로 보내는 무한 루프에 들어갑니다.


### 라즈베리 파이 디바이스

이제 라즈베리 파이 특징을 파헤칠 것입니다. 시작하기 전에 [BCM2837 ARM Peripherals 설명서](https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf)를 다운로드하십시오. BCM2837은 라즈베리 파이 3 모델 B와 B+에서 사용하는 보드입니다. 논의 중에 BCM2835와 BCM2836도 언급할 것입니다. 이들은 라즈베리 파이 이전 버전에서 사용되는 보드의 이름입니다. 구현 세부사항으로 진행하기 전에 메모리 매핑된 장치와 함께 작동하는 방법에 대한 몇 가지 기본 개념을 공유하고자 합니다. BCM2837은 단순한 [SOC(System on a chip)](https://en.wikipedia.org/wiki/System_on_a_chip) 보드입니다. 이러한 보드에서는 메모리 매핑된 레지스터를 통해 모든 장치에 대한 액세스가 수행됩니다. 라즈베리 파이 3는 장치에 0x3F000000 주소를 초과하는 메모리를 예약합니다. 특정 장치를 활성화하거나 구성하려면 장치의 레지스터 중 하나에 일부 데이터를 기록해야 합니다. 장치 레지스터는 메모리의 32비트 영역일 뿐입니다. 각 기기 레지스터의 각 비트의 의미는 `BCM2837 ARM Peripherals` 매뉴얼에 설명되어 있습니다. 메뉴얼에서 `0x7E000000`이 사용됨에도 불구하고 기본 주소로 `0x3F000000`을 사용하는 이유에 대한 자세한 내용은 매뉴얼의 섹션 1.2.3 ARM 물리적 주소와 주변 설명서를 참조하십시오. 

`kernal_main` 함수로부터, 작은 UART 디바이스와 함께 진행해야함을 추측할 수 있을 것입니다. UART는 [Universal asynchronous receiver-transmitter](https://en.wikipedia.org/wiki/Universal_asynchronous_receiver-transmitter)입니다. 이 장치는 메모리 매핑된 레지스터 중 하나에 저장된 값을 일련의 고전압 및 저전압으로 변환할 수 있습니다. 이 시퀀스는 "TTL 대 시리얼 케이블"을 통해 컴퓨터로 전달되며 단말 에뮬레이터에 의해 해석됩니다. 라즈베리 파이와의 통신을 용이하게 하기 위해 미니 UART를 사용할 것입니다. 미니 UART 레지스터의 사양을 보려면 `BCM2837 ARM Peripherals` 매뉴얼의 8페이지로 이동하세요. 

라즈베리 파이는 두 개의 UART를 가지고 있습니다: 미니 UART와 PL011 UART. 이 튜토리얼에서는, 미니 UART로만 작업할 것입니다. 그것이 더 간단하기 때문입니다. 그러나 PL011 UART와 함께 작업하는 방법을 보여주는 선택적 [연습](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson01/exercises.md)도 있습니다. 라즈베리 파이의 UART들에 대해 자세히 알아보고 그 차이를 알고 싶다면 [공식 문서](https://www.raspberrypi.org/documentation/configuration/uart.md)를 참조하십시오. 

더 친숙해져야할 또 다른 디바이스는 GPIO[General-purpose input/output](https://en.wikipedia.org/wiki/General-purpose_input/output)입니다. GPIO는 `GPIO 핀`들을 제어합니다. 이를 아래 이미지에서 쉽게 알아볼 수 있어야합니다.

![Raspberry Pi GPIO pins](../../../images/gpio-pins.jpg)

GPIO는 다른 GPIO 핀의 동작을 구성하는 데 사용될 수 있습니다. 예를 들어 미니 UART를 사용할 수 있으려면 핀 14와 15를 활성화하고 이 장치를 사용하도록 설정해야 합니다. 아래 이미지는 GPIO 핀에 번호가 할당되는 방법을 보여줍니다.

![Raspberry Pi GPIO pin numbers](../../images/gpio-numbers.png)

### 미니 UART 초기화

이제 미니 UART가 어떻게 초기화 되었는지 살펴봅시다. 이 코드는 [mini_uart.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/src/mini_uart.c)에 정의되어 있습.

```
void uart_init ( void )
{
    unsigned int selector;

    selector = get32(GPFSEL1);
    selector &= ~(7<<12);                   // clean gpio14
    selector |= 2<<12;                      // set alt5 for gpio14
    selector &= ~(7<<15);                   // clean gpio15
    selector |= 2<<15;                      // set alt5 for gpio 15
    put32(GPFSEL1,selector);

    put32(GPPUD,0);
    delay(150);
    put32(GPPUDCLK0,(1<<14)|(1<<15));
    delay(150);
    put32(GPPUDCLK0,0);

    put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to it registers)
    put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
    put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
    put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
    put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
    put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200

    put32(AUX_MU_CNTL_REG,3);               //Finally, enable transmitter and receiver
}
``` 

여기서는 `put32`와 `get32`의 두 가지 함수를 사용합니다. 이 함수들은 매우 간단합니다. 32비트 레지스터에서 데이터를 읽고 쓸 수 있게 합니다. 이 함수들은 [utils.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/src/utils.S)에서 어떻게 구현되어 있는지 볼 수 있습니다. `uart_init`은 이 레슨에서 가장 복잡하고 중요한 함수 중 하나이며, 다음 세 개의 섹션에서 계속 검토할 것입니다.

#### GPIO 대체 함수 선택

먼저, 반드시 GPIO 핀을 활성화해야합니다. 핀들의 대부분은 다른 디바이스를 위해 사용될 수 있습니다. 그래서 특정 핀을 사용하기 전에 우리는 핀의 `alternative function`을 선택해야 합니다. `alternative function`는 0~5에서 각각의 핀을 위해 설정될 수 있고 핀에 연결된 디바이스를 구성할 수 있는 숫자에 불과합니다. 아래 메뉴얼 이미지에서 GPIO 대체 함수의 기능 목록을 볼 수 있습니다(`BCM2837 ARM Peripherals` 메뉴얼의 102페이지부터 얻은 이미지입니다.).

![Raspberry Pi GPIO alternative functions](../../../images/alt.png?raw=true)

핀 14번과 15번은 TXD1과 RXD1의 대체 이용 가능한 함수로 볼 수 있습니다. 이는 만약 우리가 핀 14번과 15번을 위한 대체 함수 5번을 선택한다면  이들을 미니 UART 전송 핀이나 미니 UART 수신 핀으로 반복적으로 사용될 수 있습니다. `GPFSEL1` 레지스터 핀은 10~19번에 대한 대체 함수를 제어하기 위해 사용됩니다. 레지스터 안의 모든 비트의 의미는 다음 테이블안에 나타나 있습니다(`BCM2837 ARM Peripherals` 메뉴얼의 102페이지부터 얻은 이미지입니다.).

![Raspberry Pi GPIO function selector](../../../images/gpfsel1.png?raw=true)

그래서 미니 UART 디바이스와 함께 동작하는 GPIO 핀 14번과 15번을 구성하기 위해 사용되는 다음 코드를 이해하기 위해 필요한 모든 것을 알았습니다.

```
    unsigned int selector;

    selector = get32(GPFSEL1);
    selector &= ~(7<<12);                   // clean gpio14
    selector |= 2<<12;                      // set alt5 for gpio14
    selector &= ~(7<<15);                   // clean gpio15
    selector |= 2<<15;                      // set alt5 for gpio 15
    put32(GPFSEL1,selector);
```

#### GPIO pull-up/down

GPIO 핀을 사용할 때 종종 pull-up/pull-down이라는 용어를 만날 것입니다. 이 개념들은 [여기](https://grantwinney.com/using-pullup-and-pulldown-resistors-on-the-raspberry-pi/)에 자세하게 설명되어 있습니다. 이 기사를 읽기에 너무 게으른 사람들을 위해 pull-up/pull-down 개념에 대해 짧게 설명하겠습니다.

특정 핀을 입력으로 사용하고 이 핀에 아무것도 연결하지 않으면 핀의 값이 1인지 0인지를 식별할 수 없습니다. 사실, 이 장치는 무작위 값을 보고할 것이다. pull-up/pull-down 메커니즘은 당신이 이 문제를 극복할 수 있게 해줍니다. 핀을 풀업 상태로 설정했는데 아무 것도 연결되어 있지 않으면 항상 "1"이 보고됩니다(풀다운 상태의 경우 값은 항상 0이 됩니다). 우리의 경우 14핀과 15핀 모두 항상 연결되기 때문에 풀업 상태도 풀다운 상태도 필요하지 않습니다. 핀 상태는 재부팅 후에도 보존되기 때문에 핀을 사용하기 전에 항상 초기화를 해야 합니다. 3가지 상태(풀업, 풀다운, 둘다 아닌)가 있습니다.

핀 상태들 간의 스위칭은 전기 회로 위의 물리적인 토글링 전환이 필요하기 때문에 간단한 절차가 아닙니다. 이 과정은 `GPPUD`과 `GPPUDCLK` 레지스터를 포함하고 이것은 `BCM2837 ARM Peripherals` 메뉴얼 101 페이지에 설명되어 있습니다. 여기에 그것을 복사했습니다.

```
The GPIO Pull-up/down Clock Registers control the actuation of internal pull-downs on
the respective GPIO pins. These registers must be used in conjunction with the GPPUD
register to effect GPIO Pull-up/down changes. The following sequence of events is
required:
1. Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither
to remove the current Pull-up/down)
2. Wait 150 cycles – this provides the required set-up time for the control signal
3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to
modify – NOTE only the pads which receive a clock will be modified, all others will
retain their previous state.
4. Wait 150 cycles – this provides the required hold time for the control signal
5. Write to GPPUD to remove the control signal
6. Write to GPPUDCLK0/1 to remove the clock
``` 

이 절차는 우리가 어떻게 핀으로부터 풀업과 풀다운 상태를 제거할 수 있는지 설명합니다. 그런데 그 핀들은 아래 코드에서 핀 14번과 15번을 위해 행해지고 있는 것입니다.

```
    put32(GPPUD,0);
    delay(150);
    put32(GPPUDCLK0,(1<<14)|(1<<15));
    delay(150);
    put32(GPPUDCLK0,0);
```

#### 미니 UART 초기화하기

이제 미니 UART가 GPIO 핀에 연결되어 있으며 핀이 구성되어 있다. 나머지 `uart_init` 함수는 미니 UART를 초기화하는 전용이다.

```
    put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to its registers)
    put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
    put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
    put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
    put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
    put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200
    put32(AUX_MU_IIR_REG,6);                //Clear FIFO

    put32(AUX_MU_CNTL_REG,3);               //Finally, enable transmitter and receiver
```

코드를 한줄 한줄 봐보자.

```
    put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to its registers)
```

이 코드는 미니 UART를 가능하게 합니다. 이것은 또한 다른 미니 UART 레지스터에 대한 접근을 가능하게 하기 때문에 처음에 이 코드를 실행해야합니다.

```
    put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
```

여기서는 구성이 완료 되기 전에 수신기와 송신기를 비활성화 합니다. 또한 GPIO 핀을 추가로 사용해야하고, TTL-to-Serial 케이블이 이를 지원하지 않기 때문에자동 흐름 제어 기능을 영구적으로 비활성화합다. 흐름 제어에 관한 추가 정보를 알려면 [여기](http://www.deater.net/weave/vmwprod/hardware/pi-rts/)를 참조할 수 있습니다.

```
    put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
```

새로운 데이터를 사용할 수 있을 때마다 프로세서 인터럽트를 생성하도록 미니 UART를 구성할 수 있습니다. 우리는 레슨 3에서 인터럽트를 사용하기 시작할 것입니다. 그래서 현재는 이 기능을 사용하지 않습니다.

```
    put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
```

미니 UART는 7비트 또는 8비트 작업을 지원할 수 있습니다. 이는 ASCII 문자가 표준 집합의 경우 7비트, 확장된 문자의 경우 8비트이기 때문입니다. 우리는 8비트 모드를 사용할 것입니다.

```
    put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
```

RTS 라인은 흐름 제어에 활용되며 우리는 사용하지 않습니다. 그래서 항상 HIGH로 설정합니다.
```
    put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200
```
보드 비율은 통신 채널에서 정보가 전송되는 속도입니다. “115200 보드는 직렬 포트가 초당 최대 115200 비트를 전송할 수 있다는 것을 의미합니다. 라즈베리 파이 미니 UART 장치의 보드 속도는 터미널 에뮬레이터의 보드 속도와 동일해야합니다.
```
baudrate = system_clock_freq / (8 * ( baudrate_reg + 1 )) 
```
`system_clock_frq`는 250MHz이므로 `baudrate_reg` 값을 270으로 쉽게 계산할 수 있습니다.
``` 
    put32(AUX_MU_CNTL_REG,3);               //Finally, enable transmitter and receiver
```
마지막으로 수신기와 송신기를 활성화합니다. 미니 UART 작업 준비가 완료됩니다.

### 미니 UART를 통해서 데이터 전송하기

미니 UART가 준비되면, 데이터를 주고 받을 수 있습니다. 이를 위해서 다음 두 함수를 사용할 수 있습니다.

```
void uart_send ( char c )
{
    while(1) {
        if(get32(AUX_MU_LSR_REG)&0x20) 
            break;
    }
    put32(AUX_MU_IO_REG,c);
}

char uart_recv ( void )
{
    while(1) {
        if(get32(AUX_MU_LSR_REG)&0x01) 
            break;
    }
    return(get32(AUX_MU_IO_REG)&0xFF);
}
```

두 함수는 모두 기기가 데이터를 전송 또는 수신할 준비가 되었는지 여부를 확인하기 위해서 무한 루프에서 시작합니다. 이를 위해 `AUX_MU_LSR_REG` 레지스터를 사용하고 있습니다. 비트 0은 1로 설정하면 데이터가 준비되었음을 나타내며, 이는 UART에서 읽을 수 있음을 의미합니다. 비트 5는 1로 설정하면 송신기가 비어 있다는 것을 말해, UART를 쓸 수 있다는 것을 의미합니다. 다음으로 `AUX_MU_IO_REG`를 사용하여 전송된 문자의 값을 저장하거나 수신된 문자의 값을 읽습니다.

또한 문자 대신 문자열을 보낼 수 있는 매우 간단한 함수가 있습니다.

```
void uart_send_string(char* str)
{
    for (int i = 0; str[i] != '\0'; i ++) {
        uart_send((char)str[i]);
    }
}
```
이 함수는 문자열에서 하나하나 모든 문자에 대해 반복합니다.

### 라즈베리 파이 설정

라즈베리 파이 시작 순서는 다음과 같습니다.

1. 라즈베리 파이 전원 ON
1. GPU가 시작되어 부팅 파티션에서 `config.txt`를 읽습니다. 이 파일에는 GPU가 시동 시퀀스를 추가로 조정하는데 사용하는 몇 가지 구성 파라미터가 포함 되어 있습니다.
1. `kernel8.img` 는 로드되고 실행됩니다.

OS를 실행하기 위해서 `config.txt` 파일은 다음과 같아야합니다.

```
kernel_old=1
disable_commandline_tags=1
```
* `kernel_old=1` 는 커널 이미지 주소 0에서 로드되도록 지정합니다.
* `disable_commandline_tags`는 부팅된 이미지에 명령줄 인수를 전달하지 않도록 GPU에 지시합니다.


### 커널 테스트하기

이제 모든 소스 코드를 다 살펴봤으니 실행해볼 때가 되었습니다. 커널을 만들고 테스트 하려면 다음을 수행해야합니다.

1. [src/lesson01](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src/lesson01)에서 온 `./build.sh`이나 `./build.bat`을 커널을 빌드하기 위해서 실행하세요. 
1. 라즈베리 파이의 플래시 메모리의 `boot` 파티션으로 `kernel8.img`를 복사하세요. 그리고 `kernel7.img`을 삭제하세요. 부트 파티션의 다른 파일들은 모두 그대로 둡니다.([여기](https://github.com/s-matyukevich/raspberry-pi-os/issues/43)를 참고할 수 있습니다.)
1. 이전 섹션의 설명대로 `config.txt`를 수정하세요.
1. USB-to-TTL 시리얼 케이블을 연결하세요.  [Prerequisites](../Prerequisites.md).
1. 라즈베리 파이에 전원을 키세요. 
1. 터미널 에뮬레이터를 킵니다. 이제 `Hello, World!` 메시지를 확인할 수 있습니다.

위에 설명된 단계 순서는 당신이 당신의 SD 카드에 라즈비언을 설치한 것으로 간주합니다. 또한 빈 SD 카드를 사용하여 RPi OS를 실행할 수도 있습니다.

1. SD 카드 준비:
	* MBR 파티션 테이블 사용
	* 부팅 파티션을 FAT32로 포맷
	> 라즈비언 설치에 필요한 것과 같은 방식으로 SD 카드를 포맷해야합니다. 자세한 내용은 [공식 문서](https://www.raspberrypi.org/documentation/installation/noobs.md) 안에 `HOW TO FORMAT AN SD CARD AS FAT` 섹션을 참고하세요.
1. 다음 파일들을 SD 카드로 복사하세요.
	*  [bootcode.bin](https://github.com/raspberrypi/firmware/blob/master/boot/bootcode.bin) GPU 부트로더인데, 이것은 GPU를시작하는 코드와 GPU 펌웨어를 로드하는 코드를 포함하고 있습니다.
	* [start.elf](https://github.com/raspberrypi/firmware/blob/master/boot/start.elf) GPU 펌웨어입니다. `config.txt`를 읽고 GPU가 로드 되는 것을 가능하게 하고 `kernel8.img`로부터 ARM의 특정 사용자 코드를 로드하게 합니다.
1. `kernel8.img`와 `config.txt` 파일을 복사합니다.
1.  USB-to-TTL 시리얼 케이블을 연결합니다.
1.  라즈베리파이에 전원을 ON합니다.
1.  터미널 에뮬레이터를 활용하여 RPi OS에 연결합니다.

불행하게도, 라즈베리파이 펌웨어 파일들을 모두 비공개이다. 라즈베리 파이 시작을 순서에 대한 많은 정보를 위해 [이것](https://raspberrypi.stackexchange.com/questions/10442/what-is-the-boot-sequence)와 [이것](https://github.com/DieterReuter/workshop-raspberrypi-64bit-os/blob/master/part1-bootloader.md)과 같은 비공식적 소스를 참고할 수도 있습니다.

#### 이전 페이지

[Prerequisites](../Prerequisites.md)

#### 다음 페이지

1.2 [Kernel Initialization: Linux project structure](/linux/project-structure.md)

##### 추가 사항

이 문서는 훌륭한 오픈소스 운영체제 학습 프로젝트인 Sergey Matyukevich의 문서를 영어에 능숙하지 않은 한국인들이 학습할 수 있도록 번역한 것입니다. 오타 및 오역이 있을 수 있습니다. Sergey Matyukevich는 한국어를 능숙하게 다루지 못합니다. 대신 저에게 elxm6123@gmail.com으로 연락해주세요.
