## 3.4: 타이머

우리는 글로벌 인터럽트 컨트롤러를 검사하여 마지막 장을 마쳤습니다. [bcm2836_chained_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L246)  함수까지 타이머 인터럽트의 경로를 추적 할 수있었습니다. 다음 논리적 단계는 타이머 드라이버가 이 인터럽트를 처리하는 방법을 보는 것입니다. 그러나 이를 수행하기 전에 타이머 함수와 관련된 몇 가지 중요한 개념을 숙지해야합니다. 이들 모두는 [공식 커널 문서](https://github.com/torvalds/linux/blob/v4.14/Documentation/timers/timekeeping.txt)에 설명되어 있으며 이 문서를 읽는 것이 좋습니다. 그러나 너무 바빠서 읽을 수 없다면 언급 된 개념에 대한 간단한 설명을 제공 할 수 있습니다.

1. **Clock sources** 시간을 정확히 알아야 할 때마다 클럭 소스 프레임워크를 사용합니다. 일반적으로 클럭 소스는 0에서 2 ^ (n-1)까지 카운트 한 다음 0으로 감싼 후 다시 시작되는 단조 원자 n 비트 카운터로 구현됩니다. 클럭 소스는 카운터를 나노초 값으로 변환하는 수단도 제공합니다.
1. **Clock events** 이 추상화는 누구나 타이머 인터럽트를 구독 할 수 있도록하기 위해 도입되었습니다. 클록 이벤트 프레임 워크는 다음 이벤트의 설계 시간을 입력으로 사용하고이를 기반으로 타이머 하드웨어 레지스터의 적절한 값을 계산합니다.
1. **sched_clock()** 이 함수는 시스템이 시작된 후 나노초 수를 반환합니다. 일반적으로 타이머 레지스터를 직접 읽습니다. 이 기능은 매우 자주 호출되며 성능에 최적화되어야합니다.

다음 섹션에서는 시스템 타이머를 사용하여 클럭 소스, 클럭 이벤트 및 sched_clock 기능을 구현하는 방법을 살펴 보겠습니다.

### BCM2835 System Timer.

평소와 같이 장치 트리에서 위치를 찾아 특정 장치 탐색을 시작합니다. 시스템 타이머 노드는 [여기](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L57)에 정의되어 있습니다. 이 정의를 여러 번 참조하기 때문에 이 정의를 잠시 동안 열어 둘 수 있습니다.

다음으로, 호환되는 속성을 사용하여 해당 드라이버의 위치를 알아 내야합니다. 드라이버는 [여기](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c)에서 찾을 수 있습니다. 가장 먼저 살펴볼 것은 [bcm2835_timer](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L42) 구조입니다.

```
struct bcm2835_timer {
    void __iomem *control;
    void __iomem *compare;
    int match_mask;
    struct clock_event_device evt;
    struct irqaction act;
};
```
이 구조에는 드라이버가 작동하는 데 필요한 모든 상태가 포함됩니다. `control`과`compare` 필드는 대응하는 메모리 매핑 레지스터의 주소를 보유하고,`match_mask`는 우리가 사용할 4 개의 타이머 인터럽트 중 어떤 것을 결정하는데 사용되며`evt` 필드는 클럭 이벤트 프레임 워크으로 전달되는 구조를 포함하고 `act`는 현재 드라이버를 인터럽트 컨트롤러와 연결하는 데 사용되는 irq 액션입니다.
 
다음으로 드라이버 초기화 기능인 [bcm2835_timer_init](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L83)를 살펴 보겠습니다. 크기는 크지 만 처음부터 생각하는 것만 큼 어렵지는 않습니다.

```
static int __init bcm2835_timer_init(struct device_node *node)
{
    void __iomem *base;
    u32 freq;
    int irq, ret;
    struct bcm2835_timer *timer;

    base = of_iomap(node, 0);
    if (!base) {
        pr_err("Can't remap registers\n");
        return -ENXIO;
    }

    ret = of_property_read_u32(node, "clock-frequency", &freq);
    if (ret) {
        pr_err("Can't read clock-frequency\n");
        goto err_iounmap;
    }

    system_clock = base + REG_COUNTER_LO;
    sched_clock_register(bcm2835_sched_read, 32, freq);

    clocksource_mmio_init(base + REG_COUNTER_LO, node->name,
        freq, 300, 32, clocksource_mmio_readl_up);

    irq = irq_of_parse_and_map(node, DEFAULT_TIMER);
    if (irq <= 0) {
        pr_err("Can't parse IRQ\n");
        ret = -EINVAL;
        goto err_iounmap;
    }

    timer = kzalloc(sizeof(*timer), GFP_KERNEL);
    if (!timer) {
        ret = -ENOMEM;
        goto err_iounmap;
    }

    timer->control = base + REG_CONTROL;
    timer->compare = base + REG_COMPARE(DEFAULT_TIMER);
    timer->match_mask = BIT(DEFAULT_TIMER);
    timer->evt.name = node->name;
    timer->evt.rating = 300;
    timer->evt.features = CLOCK_EVT_FEAT_ONESHOT;
    timer->evt.set_next_event = bcm2835_time_set_next_event;
    timer->evt.cpumask = cpumask_of(0);
    timer->act.name = node->name;
    timer->act.flags = IRQF_TIMER | IRQF_SHARED;
    timer->act.dev_id = timer;
    timer->act.handler = bcm2835_time_interrupt;

    ret = setup_irq(irq, &timer->act);
    if (ret) {
        pr_err("Can't set up timer IRQ\n");
        goto err_iounmap;
    }

    clockevents_config_and_register(&timer->evt, freq, 0xf, 0xffffffff);

    pr_info("bcm2835: system timer (irq = %d)\n", irq);

    return 0;

err_iounmap:
    iounmap(base);
    return ret;
}
```
이제이 함수를 자세히 살펴 보겠습니다.

```
    base = of_iomap(node, 0);
    if (!base) {
        pr_err("Can't remap registers\n");
        return -ENXIO;
    }
```

메모리 레지스터를 매핑하고 레지스터 기본 주소를 얻는 것으로 시작합니다. 이 부분에 이미 익숙해야합니다.

```
    ret = of_property_read_u32(node, "clock-frequency", &freq);
    if (ret) {
        pr_err("Can't read clock-frequency\n");
        goto err_iounmap;
    }

    system_clock = base + REG_COUNTER_LO;
    sched_clock_register(bcm2835_sched_read, 32, freq);
```

다음으로,`sched_clock` 서브 시스템이 초기화됩니다. `sched_clock`은 실행될 때마다 타이머 카운터 레지스터에 액세스해야하며이 작업을 지원하기 위해 [bcm2835_sched_read](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L52)가 첫 번째 인수로 전달됩니다. 두 번째 인수는 타이머 카운터의 비트 수 (이 경우 32)에 해당합니다. 비트 수는 카운터가 0으로 줄 바꿈되는 시간을 계산하는 데 사용됩니다. 마지막 인수는 타이머 주파수를 지정합니다. 타이머 카운터의 값을 나노초로 변환하는 데 사용됩니다.

타이머 주파수는 [여기](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L65) 라인의 장치 트리에서 정의됩니다.

```
    clocksource_mmio_init(base + REG_COUNTER_LO, node->name,
        freq, 300, 32, clocksource_mmio_readl_up);
```

다음 줄은 클럭 소스 프레임 워크를 초기화합니다. [clocksource_mmio_init](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/mmio.c#L52)는 메모리 매핑 된 레지스터를 기반으로 간단한 클럭 소스를 초기화합니다. 클록 소스 프레임 워크는 일부 측면에서`sched_clock`의 기능을 복제하며 동일한 3 가지 기본 파라미터에 액세스해야합니다.

* 타이머 카운터 레지스터의 위치입니다.
* 카운터에서 유효한 비트 수입니다.
* 타이머 주파수.

또 다른 3 개의 매개 변수에는 클럭 소스 이름, 클럭 소스 장치를 평가하는 데 사용되는 정격 및 타이머 카운터 레지스터를 읽을 수있는 함수가 포함됩니다.

```
    irq = irq_of_parse_and_map(node, DEFAULT_TIMER);
    if (irq <= 0) {
        pr_err("Can't parse IRQ\n");
        ret = -EINVAL;
        goto err_iounmap;
    }
```

이 코드는 세 번째 타이머 인터럽트에 해당하는 Linux irq 번호를 찾는 데 사용됩니다 (번호 3은 [DEFAULT_TIMER](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L108) 상수). 간단한 알림 : Raspberry Pi 시스템 타이머에는 4개의 독립적인 타이머 레지스터 세트가 있으며 여기에서 세 번째 타이머 레지스터가 사용됩니다. 장치 트리로 돌아 가면 [interrupts](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L60) 속성을 찾을 수 있습니다. 이 속성은 장치에서 지원하는 모든 인터럽트와 이러한 인터럽트가 인터럽트 컨트롤러 라인에 매핑되는 방법을 설명합니다. 각 항목이 하나의 인터럽트를 나타내는 배열입니다. 항목의 형식은 인터럽트 컨트롤러에 따라 다릅니다. 이 경우 각 항목은 2개의 숫자로 구성됩니다. 첫 번째 항목은 인터럽트 뱅크를 지정하고 두 번째 항목은 뱅크 내의 인터럽트 번호를 지정합니다. [irq_of_parse_and_map](https://github.com/torvalds/linux/blob/v4.14/drivers/of/irq.c#L41)는 `interrupts` 속성의 값을 읽은 다음, 두 번째 인수를 사용하여 관심있는 지원되는 인터럽트를 찾고 요청 된 인터럽트에 대한 Linux irq 번호를 반환합니다.

```
    timer = kzalloc(sizeof(*timer), GFP_KERNEL);
    if (!timer) {
        ret = -ENOMEM;
        goto err_iounmap;
    }
```

여기에`bcm2835_timer` 구조를위한 메모리가 할당됩니다.

```
    timer->control = base + REG_CONTROL;
    timer->compare = base + REG_COMPARE(DEFAULT_TIMER);
    timer->match_mask = BIT(DEFAULT_TIMER);
```

다음으로, 제어 및 비교 레지스터의 주소가 계산되고 `match_mask`는 `DEFAULT_TIMER` 상수로 설정됩니다.

```
    timer->evt.name = node->name;
    timer->evt.rating = 300;
    timer->evt.features = CLOCK_EVT_FEAT_ONESHOT;
    timer->evt.set_next_event = bcm2835_time_set_next_event;
    timer->evt.cpumask = cpumask_of(0);
```

이 코드 스 니펫 [clock_event_device](https://github.com/torvalds/linux/blob/v4.14/include/linux/clockchips.h#L100) 구조체가 초기화됩니다. 여기서 가장 중요한 속성은 'bcm2835_time_set_next_event] (https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L57) 함수를 가리키는`set_next_event`입니다. 여기서 가장 중요한 속성은 `set_next_event`인데 이것은 [bcm2835_time_set_next_event](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L57) 함수를 가르킨다. 이 함수는 클럭 이벤트 프레임 워크에 의해 호출되어 다음 인터럽트를 예약합니다. `bcm2835_time_set_next_event`는 매우 간단합니다. 비교 레지스터를 업데이트하여 원하는 간격 후에 인터럽트를 예약합니다. 이것은 우리가 RPi OS를 위해 여기에서 한 것과 유사합니다.

```
    timer->act.flags = IRQF_TIMER | IRQF_SHARED;
    timer->act.dev_id = timer;
    timer->act.handler = bcm2835_time_interrupt;
```
다음으로, irq 조치가 초기화됩니다. 여기서 가장 중요한 속성은 'handler'인데, 이는 [bcm2835_time_interrupt](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L67)를 가리킵니다. 이것은 인터럽트가 발생한 후 호출되는 함수입니다. 살펴보면 모든 이벤트가 시계 이벤트 프레임 워크에 의해 등록된 이벤트 핸들러로 경로 재 지정됨을 알 수 있습니다. 이 이벤트 핸들러를 잠시 후에 살펴 보겠습니다.

```
    ret = setup_irq(irq, &timer->act);
    if (ret) {
        pr_err("Can't set up timer IRQ\n");
        goto err_iounmap;
    }
```

irq 조치가 구성된 후 타이머 인터럽트의 irq 조치 목록에 추가됩니다.

```
    clockevents_config_and_register(&timer->evt, freq, 0xf, 0xffffffff);
```
마지막으로 시계 이벤트 프레임 워크는 [clockevents_config_and_register](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L504)를 호출하여 초기화됩니다. `evt` 구조와 타이머 주파수는 처음 2 개의 인수로 전달됩니다. 마지막 2 개의 인수는 "원샷" 타이머 모드에서만 사용되며 현재 논의와 관련이 없습니다.

이제 우리는`bcm2835_time_interrupt` 함수까지 타이머 인터럽트의 경로를 추적했지만 실제 작업이 완료된 곳을 찾지 못했습니다. 다음 섹션에서는 클럭 이벤트 프레임 워크에 들어갈 때 인터럽트가 어떻게 처리되는지 자세히 살펴볼 것입니다.

### 클록 이벤트 프레임 워크에서 인터럽트가 처리되는 방법

이전 섹션에서 우리는 타이머 인터럽트를 처리하는 실제 작업이 시계 이벤트 프레임 워크로 아웃소싱되는 것을 보았습니다. 이것은 [따름의](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L74) 몇 줄로 이루어집니다.

```
        event_handler = ACCESS_ONCE(timer->evt.event_handler);
        if (event_handler)
            event_handler(&timer->evt);
```

이제 우리의 목표는 정확히 'event_handler'가 설정되었고 그것이 호출 된 후 무슨 일이 발생했는지 알아내는 것입니다.

[clockevents_config_and_register](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L504) 함수는 시계 이벤트 프레임 워크가 구성되는 위치이기 때문에 탐색을 시작하기에 좋은 장소이며, 이 함수의 논리를 따르면 결국 `event_handler`가 어떻게 설정되는지 찾아야합니다.

이제 필요한 장소로 연결되는 함수 호출 체인을 보여 드리겠습니다.

1. [clockevents_config_and_register](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L504) 이것이 최상위 초기화 함수입니다.
1. [clockevents_register_device](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L449) 이 함수에서 타이머는 전체 시계 이벤트 장치 목록에 추가됩니다.
1. [tick_check_new_device](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L300) 이 함수는 현재 장치가 "틱 장치"로 사용하기에 적합한지 여부를 확인합니다. 그렇다면, 그러한 장치는 주기적으로 틱을 생성하여 나머지 커널이 정기적으로 수행해야하는 모든 작업을 수행하는 데 사용됩니다.
1. [tick_setup_device](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L177) 이 함수는 장치 구성을 시작합니다.
1. [tick_setup_periodic](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L144) 이것은 장치가 주기적 틱을 위해 구성된 장소입니다.
1. [tick_set_periodic_handler](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-broadcast.c#L432)  마지막으로 핸들러가 할당 된 위치에 도달했습니다!

콜 체인의 마지막 기능을 살펴보면 브로드 캐스트가 활성화되어 있는지 여부에 따라 Linux가 다른 처리기를 사용한다는 것을 알 수 있습니다. 틱 브로드캐스트는 아이들 CPU를 깨우는 데 사용됩니다. 여기서 더 자세히 읽을 수는 있지만 이를 무시하고보다 일반적인 틱 핸들러에 집중할 것입니다.

일반적으로 [tick_handle_periodic](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L99) 및 [tick_periodic](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L79) 함수가 호출됩니다. 후자는 정확히 우리가 관심을 갖는 함수입니다. 여기에 내용을 복사하겠습니다.

```
/*
 * Periodic tick
 */
static void tick_periodic(int cpu)
{
    if (tick_do_timer_cpu == cpu) {
        write_seqlock(&jiffies_lock);

        /* Keep track of the next tick event */
        tick_next_period = ktime_add(tick_next_period, tick_period);

        do_timer(1);
        write_sequnlock(&jiffies_lock);
        update_wall_time();
    }

    update_process_times(user_mode(get_irq_regs()));
    profile_tick(CPU_PROFILING);
}
```

이 기능에서 몇 가지 중요한 작업이 수행됩니다:

1. `tick_next_period`는 다음 틱 이벤트를 예약 할 수 있도록 계산됩니다.
1.  [do_timer](https://github.com/torvalds/linux/blob/v4.14/kernel/time/timekeeping.c#L2200)이 호출되면 `jiffies` 설정을 담당합니다. `jiffies`는 마지막 시스템 재부팅 이후 많은 틱입니다. `jiffies`는 나노초 정밀도가 필요하지 않은 경우`sched_clock` 함수와 같은 방식으로 사용될 수 있습니다.
1. [update_process_times](https://github.com/torvalds/linux/blob/v4.14/kernel/time/timer.c#L1583)이 호출됩니다. 현재 실행중인 프로세스에 주기적으로 수행해야하는 모든 작업을 수행 할 수있는 기회가 있습니다. 이 작업에는 예를 들어 로컬 프로세스 타이머 실행 또는 가장 중요한 것은 스케줄러에 틱 이벤트에 대해 알리는 작업이 포함됩니다.

### Conclusion

이제 일반적인 타이머 인터럽트의 길이는 얼마나되는지 알지만 처음부터 끝까지 따라갔습니다. 가장 중요한 것 중 하나는 마침내 스케줄러가 호출되는 위치에 도달했다는 것입니다. 스케줄러는 모든 운영 체제에서 가장 중요한 부분 중 하나이며 타이머 인터럽트에 크게 의존합니다. 이제 스케줄러 기능이 트리거되는 위치를 보았을 때 구현에 대해 논의 할 시간입니다. 이는 다음 학습에서 수행 할 작업입니다.

##### 이전 페이지

3.3 [Interrupt handling: Interrupt controllers](../../../docs/lesson03/linux/interrupt_controllers.md)

##### 다음 페이지

3.5 [Interrupt handling: Exercises](../../../docs/lesson03/exercises.md)
