
## 3.3: 인터럽트 컨트롤러

이 장에서는 Linux 드라이버와 인터럽트를 처리하는 방법에 대해 많이 이야기 할 것입니다. 드라이버 초기화 코드부터 시작하여 [handle_arch_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L44) 함수 이후에 인터럽트가 어떻게 처리되는지 살펴 보겠습니다. 

### 디바이스 트리를 사용하여 필요한 장치 및 드라이버 찾기

RPi OS에서 인터럽트를 구현할 때 시스템 타이머와 인터럽트 컨트롤러의 두 가지 장치를 사용했습니다. 이제 우리의 목표는 Linux에서 동일한 장치가 어떻게 작동하는지 이해하는 것입니다. 가장 먼저해야 할 일은 언급 된 장치를 다루는 드라이버를 찾는 것입니다. 필요한 드라이버를 찾기 위해 [bcm2837-rpi-3-b.dts](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts) 장치 트리 파일을 사용할 수 있습니다. 이 파일은 Raspberry Pi 3 Model B에 고유한 최상위 장치 트리 파일이며, 다른 버전의 Raspberry Pi간에 공유되는 다른 일반적인 장치 트리 파일을 포함합니다. 포함 체인을 따라 `timer`및 `interrupt-controller`를 검색하면 4개의 장치를 찾을 수 있습니다.

1. [Local interrupt controller](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L11)
1. [Local timer](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L20)
1. Global interrupt controller. It is defined [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L109) and modified [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L72).
1. [System timer](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L57)

멈춰야하는데 왜 개가 아닌 4개의 장치가 있을까요? 여기에는 약간의 설명이 필요하며 다음 섹션에서이 질문을 다룰 것입니다.

### 로컬 vs 글로벌 인터럽트 컨트롤러

다중 프로세서 시스템의 인터럽트 처리에 대해 생각할 때 특정 인터럽트를 처리해야 할 코어는 무엇입니까? 인터럽트가 발생하면 4개의 코어가 모두 인터럽트됩니까, 아니면 하나의 코어만 중단됩니까? 특정 인터럽트를 특정 코어로 라우팅 할 수 있습니까? 궁금한 또 다른 질문은 정보를 전달해야 할 경우 한 프로세서가 다른 프로세서에게 어떻게 알릴 수 있습니까?

로컬 인터럽트 컨트롤러는 이러한 질문들을 대답하는 데 도움이되는 장치입니다. 다음 작업을 담당합니다.

* 특정 인터럽트를 수신 할 코어 구성.
* 코어 간 인터럽트 전송 이러한 인터럽트를 "mailbox"이라고하며 코어가 서로 통신 할 수 있도록합니다.
* 로컬 타이머 및 성능 모니터 인터럽트(PMU)의 인터럽트 처리.

로컬 타이머뿐만 아니라 로컬 인터럽트 컨트롤러의 동작은 [BCM2836 ARM 로컬 주변 장치](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) 메뉴얼에 설명되어 있습니다.

이미 로컬 타이머를 여러 번 언급했습니다. 이제 왜 시스템에 두 개의 독립 타이머가 필요한지 궁금 할 것입니다. 로컬 타이머를 사용하는 주요 사용 사례는 타이머 인터럽트를 동시에 받도록 4개의 코어를 모두 구성하려는 경우입니다. 시스템 타이머를 사용하면 인터럽트를 단일 코어로만 라우팅 할 수 있습니다.

RPi OS로 작업 할 때 로컬 인터럽트 컨트롤러 또는 로컬 타이머로 작동하지 않았습니다. 기본적으로 로컬 인터럽트 컨트롤러는 모든 외부 인터럽트가 첫 번째 코어로 전송되도록 구성되어 있기 때문에 정확히 필요한 것입니다. 시스템 타이머를 대신 사용하기 때문에 로컬 타이머를 사용하지 않았습니다.

### 로컬 인터럽트 컨트롤러

[bcm2837.dtsi](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L75)에 따르면 전역 인터럽트 컨트롤러는 로컬의 자식이다. 따라서 로컬 컨트롤러로 탐색을 시작하는 것이 좋습니다. 

특정 장치에서 작동하는 드라이버를 찾아야하는 경우 [호환 가능한](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L12) 속성을 사용해야합니다. 이 속성의 값을 검색하면 RPi 로컬 인터럽트 컨트롤러와 호환되는 단일 드라이버가 있음을 쉽게 찾을 수 있습니다. 여기에 해당 [정의](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L315)가 있습니다. 

```
IRQCHIP_DECLARE(bcm2836_arm_irqchip_l1_intc, "brcm,bcm2836-l1-intc",
        bcm2836_arm_irqchip_l1_intc_of_init);
```

이제 드라이버 초기화 절차가 무엇인지 짐작할 수 있습니다. 커널은 장치 트리의 모든 장치 정의와 "compatible"속성을 사용하여 일치하는 드라이버를 찾는 각 정의를 안내합니다. 드라이버가 발견되면 해당 초기화 함수가 호출됩니다. 초기화 함수는 장치 등록 중에 제공되며이 경우 이 함수는 [bcm2836_arm_irqchip_l1_intc_of_init](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L280)입니다.

```
static int __init bcm2836_arm_irqchip_l1_intc_of_init(struct device_node *node,
                              struct device_node *parent)
{
    intc.base = of_iomap(node, 0);
    if (!intc.base) {
        panic("%pOF: unable to map local interrupt registers\n", node);
    }

    bcm2835_init_local_timer_frequency();

    intc.domain = irq_domain_add_linear(node, LAST_IRQ + 1,
                        &bcm2836_arm_irqchip_intc_ops,
                        NULL);
    if (!intc.domain)
        panic("%pOF: unable to create IRQ domain\n", node);

    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTPSIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTPNSIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTHPIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTVIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_GPU_FAST,
                     &bcm2836_arm_irqchip_gpu);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_PMU_FAST,
                     &bcm2836_arm_irqchip_pmu);

    bcm2836_arm_irqchip_smp_init();

    set_handle_irq(bcm2836_arm_irqchip_handle_irq);
    return 0;
}
```

초기화 함수는 `node`및 `parent`의 두 매개 변수를 사용하며 둘 다 [struct device_node](https://github.com/torvalds/linux/blob/v4.14/include/linux/of.h#L49) 유형입니다. `node`는 장치 트리의 현재 노드를 나타내며 여기서는 [여기] (https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L11)를 가리 킵니다. `parent`는 장치 트리 계층 구조의 부모 노드이며 로컬 인터럽트 컨트롤러의 경우`soc` 요소를 가리킵니다 (`soc`는 "system on chip"을 나타내며 매핑 가능한 가장 간단한 버스입니다) 모든 장치는 주 메모리에 직접 등록됩니다.).

`node`는 현재 장치 트리 노드에서 다양한 속성을 읽는 데 사용될 수 있습니다. 예를 들어,`bcm2836_arm_irqchip_l1_intc_of_init` 함수의 첫 번째 줄은 [reg](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837dtsi # L13) 속성에서 장치 기본 주소를 읽습니다. 그러나 이 기능이 실행될 때 MMU가 이미 활성화되어 있고 일부 물리적 메모리 영역에 액세스하려면 먼저 이 영역을 가상 주소에 매핑해야하므로 프로세스가 그보다 더 복잡합니다. 이것은 정확히 [of_iomap](https://github.com/torvalds/linux/blob/v4.14/drivers/of/address.c#L759)의 기능입니다 : 제공된 노드의 `reg` 속성을 읽습니다. `reg` 속성으로 설명된 전체 메모리 영역을 일부 가상 메모리 영역에 매핑합니다.

다음 로컬 타이머 주파수는 [bcm2835_init_local_timer_frequency](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L264) 함수에서 초기화됩니다. 이 기능에 대한 구체적인 내용은 없습니다. [BCM2836 ARM-local 주변 장치](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) 매뉴얼에 설명 된 일부 레지스터 만 사용하여 로컬 타이머를 초기화합니다.

다음 줄에는 몇 가지 설명이 필요합니다.

```
    intc.domain = irq_domain_add_linear(node, LAST_IRQ + 1,
                        &bcm2836_arm_irqchip_intc_ops,
                        NULL);
```
Linux는 각 인터럽트에 고유한 정수를 할당하므로이 숫자를 고유한 인터럽트 ID로 생각할 수 있습니다. 이 ID는 인터럽트로 무언가를 수행하려고 할 때마다 사용됩니다(예 : 처리기 할당 또는 처리 할 CPU 할당). 각 인터럽트에는 하드웨어 인터럽트 번호도 있습니다. 일반적으로 어떤 인터럽트 라인이 트리거되었는지를 나타내는 숫자입니다. `BCM2837 ARM 주변 장치 매뉴얼`에는 113 페이지의 주변 장치 인터럽트 테이블이 있습니다. 이 테이블의 인덱스를 하드웨어 인터럽트 번호로 생각할 수 있습니다. 따라서 Linux irq 번호를 하드웨어 irq 번호에 매핑하거나 그 반대로 매핑하는 메커니즘이 필요합니다. 인터럽트 컨트롤러가 하나만 있으면 일대일 매핑을 사용할 수 있지만 일반적으로 더 정교한 메커니즘을 사용해야합니다. Linux에서 [struct irq_domain](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdomain.h#L152)은 이러한 매핑을 구현합니다. 각 인터럽트 컨트롤러 드라이버는 자체 irq 도메인을 작성하고이 도메인에서 처리 할 수있는 모든 인터럽트를 등록해야합니다. 등록 함수는 나중에 인터럽트 작업에 사용되는 Linux irq 번호를 반환합니다.

다음 6 줄은 irq 도메인에 지원되는 각 인터럽트를 등록합니다.

```
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTPSIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTPNSIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTHPIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTVIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_GPU_FAST,
                     &bcm2836_arm_irqchip_gpu);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_PMU_FAST,
                     &bcm2836_arm_irqchip_pmu);
```
[BCM2836 ARM-local 주변 장치](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) 메뉴얼에 따르면 로컬 인터럽트 컨트롤러는 10개의 서로 다른 인터럽트를 처리합니다. 0-3은 로컬 타이머의 인터럽트, 4-7은 프로세스 간 통신에 사용되는 mailbox 인터럽트, 8은 전역에 의해 생성 된 모든 인터럽트에 해당 인터럽트 컨트롤러 및 인터럽트9는 성능 모니터 인터럽트입니다. [여기](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L67) 드라이버가 각 인터럽트 당 하드웨어 irq 번호를 보유하는 상수 세트를 정의하고 있음을 알 수 있습니다. 위의 등록 코드는 mailbox 인터럽트를 제외하고 별도로 등록된 모든 인터럽트를 등록합니다. 등록 코드를 더 잘 이해하기 위해 [bcm2836_arm_irqchip_register_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L154) 함수을 검사 할 수 있습니다.

```
static void bcm2836_arm_irqchip_register_irq(int hwirq, struct irq_chip *chip)
{
    int irq = irq_create_mapping(intc.domain, hwirq);

    irq_set_percpu_devid(irq);
    irq_set_chip_and_handler(irq, chip, handle_percpu_devid_irq);
    irq_set_status_flags(irq, IRQ_NOAUTOEN);
}
```
첫 번째 줄은 실제 인터럽트 등록을 수행합니다. [irq_create_mapping](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/irqdomain.c#L632)은 하드웨어 인터럽트 번호를 입력으로 받아 Linux irq 번호를 반환합니다.

[irq_set_percpu_devid](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/irqdesc.c#L849)는 인터럽트를 "CPU 당"으로 구성하여 현재 CPU에서만 처리되도록합니다. 이것은 지금 논의하고 있는 모든 인터럽트가 로컬이고 모두 현재 CPU에서만 처리할 수 있기 때문에 쉽습니다.

[irq_set_chip_and_handler](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L608)는 이름에서 알 수 있듯이 irq 칩과 irq 핸들러를 설정합니다. Irq 칩은 드라이버에 의해 생성되어야하는 특정 구조체로, 특정 인터럽트를 마스킹 및 마스킹 해제하는 방법이 있습니다. 현재 검토중인 드라이버는 외부 타이머 장치에서 생성 된 모든 인터럽트를 제어하는 [timer](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L118) 칩, [PMU](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L134)  칩 및 [GPU](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L148) 칩의 세 가지 irq 칩을 정의합니다. 핸들러는 인터럽트 처리를 담당하는 함수입니다. 이 경우 핸들러는 일반 [handle_percpu_devid_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L859) 함수로 설정됩니다. 이 핸들러는 나중에 전역 인터럽트 컨트롤러 드라이버에 의해 다시 작성됩니다.

이 경우 [irq_set_status_flags](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L652)는 현재 인터럽트를 수동으로 활성화해야하며 기본적으로 활성화해서는 안됨을 나타내는 플래그를 설정합니다.

`bcm2836_arm_irqchip_l1_intc_of_init` 함수로 돌아가면 2 개의 호출만 남습니다. 첫 번째는 [bcm2836_arm_irqchip_smp_init](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L243)입니다. 여기에서 mailbox 인터럽트가 활성화되어 프로세서 코어가 서로 통신 할 수 있습니다.

마지막 함수 호출은 매우 중요합니다. 이것은 저수준 예외 처리 코드가 드라이버에 연결되는 곳입니다.

```
    set_handle_irq(bcm2836_arm_irqchip_handle_irq);
```

[set_handle_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L46)는 아키텍처 별 코드로 정의되어 있으며 이미이 기능을 만났습니다. 위의 줄에서 [bcm2836_arm_irqchip_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L164)가 하위 수준 예외 코드에 의해 호출됨을 이해할 수 있습니다. 함수 자체는 아래와 같습니다.

```
static void
__exception_irq_entry bcm2836_arm_irqchip_handle_irq(struct pt_regs *regs)
{
    int cpu = smp_processor_id();
    u32 stat;

    stat = readl_relaxed(intc.base + LOCAL_IRQ_PENDING0 + 4 * cpu);
    if (stat & BIT(LOCAL_IRQ_MAILBOX0)) {
#ifdef CONFIG_SMP
        void __iomem *mailbox0 = (intc.base +
                      LOCAL_MAILBOX0_CLR0 + 16 * cpu);
        u32 mbox_val = readl(mailbox0);
        u32 ipi = ffs(mbox_val) - 1;

        writel(1 << ipi, mailbox0);
        handle_IPI(ipi, regs);
#endif
    } else if (stat) {
        u32 hwirq = ffs(stat) - 1;

        handle_domain_irq(intc.domain, hwirq, regs);
    }
}
```

이 함수는 `LOCAL_IRQ_PENDING`레지스터를 읽어 현재 보류중인 인터럽트를 파악합니다. 각각 자체 프로세서 코어에 해당하는 4 개의 `LOCAL_IRQ_PENDING`레지스터가 있으므로 현재 프로세서 인덱스를 사용하여 올바른 것을 선택합니다. 메일 함 인터럽트 및 기타 모든 인터럽트는 if 문의 두 가지 다른 절로 처리됩니다. 멀티 프로세서 시스템의 서로 다른 코어 간의 상호 작용은 현재 논의 범위를 벗어나므로 mailbox 인터럽트 처리 부분을 건너 뛸 것입니다. 이제 다음 두 줄만 설명 할 수 없습니다.

```
        u32 hwirq = ffs(stat) - 1;

        handle_domain_irq(intc.domain, hwirq, regs);
```

인터럽트가 다음 핸들러로 전달되었습니다. 우선 모든 하드웨어 irq 번호가 계산됩니다. 이를 위해 [ffs](https://github.com/torvalds/linux/blob/v4.14/include/asm-generic/bitops/ffs.h#L13) (첫 번째 비트 찾기) 함수가 사용됩니다. 하드웨어 irq 번호가 계산 된 후 [handle_domain_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/irqdesc.c#L622) 기능이 호출됩니다. 이 함수는 irq 도메인을 사용하여 하드웨어 irq 번호를 Linux irq 번호로 변환 한 다음 irq 구성 ([irq_desc](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdesc.h#L55) 구조체에 저장 됨)을 확인하고 인터럽트 핸들러를 호출합니다. 핸들러가 [handle_percpu_devid_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L859)로 설정되어 있음을 확인했습니다. 그러나 이 핸들러는 나중에 하위 인터럽트 컨트롤러에서 덮어 씁니다. 이제 어떻게 이런 일이 일어나는지 살펴봅시다.

### 제네릭 인터럽트 컨트롤러

디바이스 트리와 `compatible`속성을 사용하여 일부 디바이스에 해당하는 드라이버를 찾는 방법을 이미 살펴 봤으므로 이 부분을 건너 뛰고 일반적인 인터럽트 컨트롤러 드라이버 소스 코드로 바로 넘어가겠습니다.

[irq-bcm2835.c](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c) 파일에서 찾을 수 있습니다. 평소와 같이 초기화 기능으로 탐색을 시작합니다. 이를 [armctrl_of_init](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L141)라고 합니다.

```
static int __init armctrl_of_init(struct device_node *node,
				  struct device_node *parent,
				  bool is_2836)
{
	void __iomem *base;
	int irq, b, i;

	base = of_iomap(node, 0);
	if (!base)
		panic("%pOF: unable to map IC registers\n", node);

	intc.domain = irq_domain_add_linear(node, MAKE_HWIRQ(NR_BANKS, 0),
			&armctrl_ops, NULL);
	if (!intc.domain)
		panic("%pOF: unable to create IRQ domain\n", node);

	for (b = 0; b < NR_BANKS; b++) {
		intc.pending[b] = base + reg_pending[b];
		intc.enable[b] = base + reg_enable[b];
		intc.disable[b] = base + reg_disable[b];

		for (i = 0; i < bank_irqs[b]; i++) {
			irq = irq_create_mapping(intc.domain, MAKE_HWIRQ(b, i));
			BUG_ON(irq <= 0);
			irq_set_chip_and_handler(irq, &armctrl_chip,
				handle_level_irq);
			irq_set_probe(irq);
		}
	}

	if (is_2836) {
		int parent_irq = irq_of_parse_and_map(node, 0);

		if (!parent_irq) {
			panic("%pOF: unable to get parent interrupt.\n",
			      node);
		}
		irq_set_chained_handler(parent_irq, bcm2836_chained_handle_irq);
	} else {
		set_handle_irq(bcm2835_handle_irq);
	}

	return 0;
}
```

이제, 이 함수를 살펴보겠습니다.

```
    void __iomem *base;
    int irq, b, i;

    base = of_iomap(node, 0);
    if (!base)
        panic("%pOF: unable to map IC registers\n", node);

    intc.domain = irq_domain_add_linear(node, MAKE_HWIRQ(NR_BANKS, 0),
            &armctrl_ops, NULL);
    if (!intc.domain)
        panic("%pOF: unable to create IRQ domain\n", node);

```

이 함수는 장치3에서 장치 기본 주소를 읽고 irq 도메인을 초기화하는 코드로 시작합니다. 로컬 irq 컨트롤러 드라이버에서 유사한 코드를 보았으므로이 부분은 이미 익숙 할 것입니다.

```
    for (b = 0; b < NR_BANKS; b++) {
        intc.pending[b] = base + reg_pending[b];
        intc.enable[b] = base + reg_enable[b];
        intc.disable[b] = base + reg_disable[b];
```
다음으로, 모든 irq 뱅크를 반복하는 루프가 있습니다. 우리는 이 강의의 첫 번째 장에서 이미 irq 뱅크에 대해 간단히 언급했습니다. 인터럽트 컨트롤러에는 3 개의 irq 뱅크가 있으며, 이는 `ENABLE_IRQS_1`, `ENABLE_IRQS_2` 및 `ENABLE_BASIC_IRQS` 레지스터에 의해 제어됩니다. 각 뱅크에는 자체 활성화, 비활성화 및 보류 레지스터가 있습니다. 활성화 및 비활성화 레지스터를 사용하여 특정 뱅크에 속하는 개별 인터럽트를 활성화 또는 비활성화 할 수 있습니다. 보류 레지스터는 처리 대기중인 인터럽트를 판별하는 데 사용됩니다.

```
        for (i = 0; i < bank_irqs[b]; i++) {
            irq = irq_create_mapping(intc.domain, MAKE_HWIRQ(b, i));
            BUG_ON(irq <= 0);
            irq_set_chip_and_handler(irq, &armctrl_chip,
                handle_level_irq);
            irq_set_probe(irq);
        }
```

다음으로, 지원되는 각 인터럽트를 등록하고 irq 칩과 핸들러를 설정하는 중첩 루프가 있습니다.

이미 로컬 인터럽트 컨트롤러 드라이버에서 동일한 함수가 어떻게 사용되는지 보았습니다. 그러나 몇 가지 중요한 사항을 강조하고 싶습니다.

* [MAKE_HWIRQ](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L57) 매크로는 하드웨어 irq 번호를 계산하는 데 사용됩니다. 뱅크 내 뱅크 지수 및 irq 지수를 기준으로 계산됩니다.
* [handle_level_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L603)는 레벨 유형의 인터럽트에 사용되는 공통 핸들러입니다. 이러한 유형의 인터럽트는 인터럽트가 승인 될 때까지 인터럽트 라인을 "HIGH"로 설정합니다. 다른 방식으로 작동하는 에지 유형 인터럽트도 있습니다.
* [irq_set_probe](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L667) 함수는 [IRQ_NOPROBE](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L64) 인터럽트 플래그를 설정 해제하여 인터럽트 자동 프로빙을 효과적으로 비활성화합니다. 인터럽트 자동 프로빙은 다른 드라이버가 장치에 연결된 인터럽트 라인을 발견 할 수 있도록하는 프로세스입니다. 이 정보는 장치 트리에서 인코딩되기 때문에 Raspberry Pi에는 필요하지 않지만 일부 장치의 경우 유용 할 수 있습니다. Linux 커널에서 자동 탐색이 작동하는 방식을 이해하려면 [여기](https://github.com/torvalds/linux/blob/v4.14/include/linux/interrupt.h#L662) 주석을 참조하십시오.

다음 코드는 BCM2836 및 BCM2835 인터럽트 컨트롤러와 다릅니다 (첫 번째 코드는 RPi 모델 2 및 3에 해당하고 두 번째 코드는 RPi 모델 1에 해당). BCM2836을 다루는 경우 다음 코드가 실행됩니다.

```
        int parent_irq = irq_of_parse_and_map(node, 0);

        if (!parent_irq) {
            panic("%pOF: unable to get parent interrupt.\n",
                  node);
        }
        irq_set_chained_handler(parent_irq, bcm2836_chained_handle_irq);
```

장치 트리는 로컬 인터럽트 컨트롤러가 글로벌 인터럽트 컨트롤러의 부모임을 [나타냅니다](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L75). 또 다른 장치 트리 [속성](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L76)은 전역 인터럽트 컨트롤러가 로컬 컨트롤러의 interupt line number 8에 연결되어 있음을 나타냅니다. 이는 부모 irq가 하드웨어 irq 번호 8을 가진 것을 의미합니다.이 두 속성은 Linux 커널이 부모를 찾을 수 있도록합니다. 인터럽트 번호 (하드웨어 번호가 아닌 Linux 인터럽트 번호) 마지막으로 [irq_set_chained_handler](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L636) 함수는 부모 irq의 핸들러를 [bcm2836_chained_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L246) 함수로 바꿉니다.

[bcm2836_chained_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L246) 매우 짧고, 아래에 코드가 있습니다.

```
static void bcm2836_chained_handle_irq(struct irq_desc *desc)
{
    u32 hwirq;

    while ((hwirq = get_next_armctrl_hwirq()) != ~0)
        generic_handle_irq(irq_linear_revmap(intc.domain, hwirq));
}
```
이 코드는 우리가 RPi OS에서 [여기에](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.c#L39) 수행 한 작업의 고급 버전으로 생각할 수 있습니다. [get_next_armctrl_hwirq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L217)는 대기중인 3 개의 레지스터를 모두 사용하여 어떤 인터럽트가 발생했는지 파악합니다. [irq_linear_revmap](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdomain.h#L377)은 irq 도메인을 사용하여 하드웨어 irq 번호를 Linux irq 번호로 변환하고 [generic_handle_irq](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdesc.h#L156)는 irq 핸들러를 실행합니다. Irq 핸들러는 초기화 함수에서 설정되었으며 [handle_level_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L603) 를 가리키며 결국 인터럽트와 관련된 모든 irq 조치를 실행합니다 (실제로 [여기서](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/handle.c#L135) 수행됨). 현재, irq 조치 목록은 지원되는 모든 인터럽트에 대해 비어 있습니다. 일부 인터럽트를 처리하려는 드라이버는 해당 목록에 조치를 추가해야합니다. 다음 장에서는 시스템 타이머를 예로 사용하여이 작업을 수행하는 방법을 살펴 보겠습니다.

##### 이전 페이지

3.2 [Interrupt handling: Low-level exception handling in Linux](./low_level-exception_handling.md)

##### 다음 페이지

3.4 [Interrupt handling: Timers](./timer.md)

##### 추가 사항

이 문서는 훌륭한 오픈소스 운영체제 학습 프로젝트인 Sergey Matyukevich의 문서를 영어에 능숙하지 않은 한국인들이 학습할 수 있도록 번역한 것입니다. 오타 및 오역이 있을 수 있습니다. Sergey Matyukevich는 한국어를 능숙하게 다루지 못합니다. 대신 저에게 elxm6123@gmail.com으로 연락해주세요.
