
## 1.4: 리눅스 스타트업 순서

### 진입점 검색

리눅스 프로젝트 구조를 살펴보고 어떻게 구축할 수 있는지 검토한 후, 다음 논리적 단계는 프로그램 진입점을 찾는 것입니다. 이 단계는 많은 프로그램에서는 사소한 것일 수 있지만 Linux 커널에서는 그렇지 않습니다.

우리가 할 첫 번째 일은 [arm64 linker 스크립트](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/vmlinux.lds.S)를 살펴보는 것입니다. 우리는 이미 링커 스크립트가 [기본 makefile 안에서](https://github.com/torvalds/linux/blob/v4.14/Makefile#L970) 어떻게 사용되는지 이미 살펴보았습니다. 이 라인으로부터, 우리는 쉽게 유추할 수 있는데, 어디에서 특정한 아키텍처에 대한 링커 스크립트를 찾을 수 있습니다.

우리가 검토할 파일은 실제 링크 스크립트가 아니라 템플릿이며, 실제 링크 스크립트는 일부 매크로를 실제 값으로 대체하여 작성됩니다. 그러나 정확히 말해서 이 파일은 대부분 매크로로 구성되어 있기 때문에 다른 아키텍처들 사이에서 읽기와 포팅이 훨씬 쉬워집니다.

우리가 링커 스크립트안에서 발견할 수 있는 첫번째 섹션은 [.head.text](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/vmlinux.lds.S#L96)라고 불립니다. 진입점은 이 섹션에서 정의되어야 하기 때문에 이것은 우리에게 매우 중요합니다. 조금만 생각해 보면, 그것은 완벽하게 이해가 됩니다: 커널이 로드된 후, 이진 이미지의 내용이 어떤 메모리 영역으로 복사되고 그 영역의 시작부터 실행이 시작됩니다. 누가 .head.text를 사용하고 있는지 검색하는 것만으로도 진입점을 찾을 수 있어야 한다는 뜻입니다. arm64 아키텍처는 .section ".head.text"ax"로 확장된 [__HEAD](https://github.com/torvalds/linux/blob/v4.14/include/linux/init.h#L90) 매크로를 사용하는 단일 파일을 가지고 있는데, 이 파일은 [head.S]입니다.  

`head.S` 파일 안에서 우리가 찾을 수 있는 첫 실행 파일 라인은 [이것](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L85)입니다.
여기서 ARM 어셈블러의 `b`와 `branch` 명령어를 [stext](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L116) 함수로 점프하기 위해서 사용합니다. 그리고 이것이 커널을 부팅한 후에 실행되는 첫 번째 함수입니다.

다음 논리적 단계에서 어떻게 그 `stext` 함수에서 일어나는 일을 탐험하기 위해 아직 준비가 없다는 것입니다. 첫번째로, RPi는 OS에서 비슷한 기능을 구현하는 데, 그리고 2,3수업에 적용될 것입니다. 우리가 지금 할 것은 커널과 연관된 몇가지 중요한 개념을 설명하는 것입니다.

### 리눅스 부트로더 및 부트 프로토콜

리눅스 커널이 부팅될 때 기계 하드웨어가 어떤 "알려진 상태"로 준비되었다고 가정한다. 이 상태를 정의하는 규칙 집합을 "부팅 프로토콜"이라고 하며, `arm64` 아키텍처에 대해서는 [여기](https://github.com/torvalds/linux/blob/v4.14/Documentation/arm64/booting.txt)에 문서화합니다. 그 중에서도, 예를 들면, 주 CPU에서만 실행을 시작해야 하고, 메모리 매핑 유닛을 꺼야하며, 모든 인터럽트를 비활성화해야 한다고 규정합니다.

좋아요, 하지만 누가 기계를 그 알려진 상태로 들여올 책임이 있나요? 보통, 커널보다 먼저 실행되어 모든 초기화를 수행하는 특별한 프로그램이 있습니다. 그 프로그램은 부르토더라고 불립니다. 부트로더 코드는 기계마다 매우 다를 수 있으며, 라즈베리 파이가 이에 해당 됩니다. 라스베리 파이는 보드에 내장되어 있는 부트로더를 가지고 있습니다. 우리는 오직 이 행동을 커스텀마이즈 하기 위해서 [config.txt](https://www.raspberrypi.org/documentation/configuration/config-txt/) 파일을 사용할 수 있습니다.

### UEFI boot

그러나 커널 이미지 자체에 내장할 수 있는 부트 로더가 하나 있습니다. 이 bootloader는 [Unified Extensible Firmware Interface (UEFI)](https://en.wikipedia.org/wiki/Unified_Extensible_Firmware_Interface)가 지원하는 플랫폼에만 사용될 수 있습니다. UEFI를 지원하는 기기는 실행 중인 소프트웨어에 표준화된 서비스 세트를 제공하며 그러한 서비스는 기계 자체와 그 기능에 대한 모든 필요한 정보를 파악하는 데 사용될 수 있습니다. UEFI는 또한 컴퓨터 펌웨어가 [Portable Executable(PE)](https://en.wikipedia.org/wiki/Portable_Executable) 포맷 안에 실행 파일을 실행할 수 있어야 한다고 요구합니다. 리눅스 커널 UEFI 부트로더는 이 기능을 이용합니다. 리눅스 커널 이미지 시작 부분에 `PE` 헤더를 주입하여 컴퓨터 펌웨어가 이미지가 일반적인 `PE` 파일이라고 생각하도록 합니다. 이것은 [efi-header.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-header.S) 파일 안에서 수행됩니다. 이 파일은 [head.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L98) 안에 사용되는 [__EFI_PE_HEADER](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-header.S#L13) 매크로를 정의합니다. which is used inside [head.S]

`__EFI_PE_HEADER` 안에 정의된 한가지 중요한 속성은 [location of the UEFI entry point](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-header.S#L33)에 대해 말합니다. 그리고 이 진입점은 스스로 [efi-entry.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-entry.S#L32) 안에서 발견될 수 있습니다. ]이 위치부터 소스 코드를 따라 UEFI 부트로더가 정확히 무엇을 하고 있는지 검사할 수 있습니다. 그러나 본 섹션의 목적은 UEFI 부트로더를 자세히 조사하는 것이 아니라 UEFI가 무엇인지, 리눅스 커널이 어떻게 사용하는지를 대략적으로 알려주기 때문에 여기서 멈추려고 합니다.

### Device Tree

리눅스 커널의 스타트업 코드를 조사하기 시작했을 때 `Device Tree`에 대한 언급이 많았습니다. 그것은 본질적인 개념으로 보이며, 나는 그것을 논의할 필요가 있다고 생각합니다.

`Raspberry PI OS` 커널을 작업했을 때, 특정 메모리 매핑 레지스터가 위치한 정확한 오프셋을 파악하려면 [BCM2837 ARM Peripherals manual](https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf)을 사용했었습니다. 이 정보는 분명히 각 게시판마다 다르며, 그 중 한 곳만 지원하면 되는 것이 다행입니다. 하지만 우리가 수백 개의 다른 보드를 지원해야 한다면? 우리가 커널 코드에 있는 각각의 보드에 대한 정보를 하드코딩하려고 하면 완전히 엉망진창이 될 것입니다. 그리고 설사 우리가 가까스로 그렇게 한다고 해도, 지금 어떤 보드를 사용하고 있는지 어떻게 알 수 있을까? 예를 들어 `BCM2837`은 실행 중인 커널에 그러한 정보를 전달하는 수단을 제공하지 않습니다.

장치 트리는 위에 언급된 문제에 대한 해결책을 우리에게 제공합니다. 컴퓨터 하드웨어를 설명하는 데 사용할 수 있는 특수한 형식입니다. 장치 트리는 [여기](https://www.devicetree.org/)에서 볼 수 있습니다. 커널이 실행되기 전에 부트로더는 적절한 장치 트리 파일을 선택하여 커널에 인수로 전달합니다. 라즈베리 PI SD카드의 부트 파티션에 있는 파일을 보면 여기서 `.dtb` 파일을 많이 찾을 수 있습니다. `.dtb`는 장치 트리 파일을 컴파일한 것입니다. `config.txt` 에서 그 중 일부를 선택하여 일부 라즈베리 파이 하드웨어를 활성화하거나 비활성화 할수 있습니다. 커널이 실행되기 전에 부트로더는 적절한 장치 트리 파일을 선택하여 커널에 인수로 전달합니다. Raspberry PI SD 카드의 부팅 파티션에 있는 파일을 보면, 여기서 `.dtb` 파일을 많이 찾을 수 있습니다. `.dtb`는 장치 트리 파일을 컴파일한 것입니다. "config.txt"에서 일부를 선택하여 일부 Raspberry PI 하드웨어를 활성화하거나 비활성화할 수 있습니다. 이 프로세스는 [Raspberry PI 공식 설명서](https://www.raspberrypi.org/documentation/configuration/device-tree.md)에 자세히 설명되어 있습니.

자, 이제 실제 장치 트리가 어떻게 생겼는지 살펴봅시다. 간단한 연습으로 [Rasperry PI 3 Model B](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/)의 장치 트리를 찾아보자. [문서](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2837/README.md)로부터  우리는 `BCM2837`를 사용하는 `Raspberry PI 3 Model B`를 이해할 수 있습니다. 이 파일 [/arch/arm64/boot/dts/broadcom/bcm2837-rpi-3-b.dts](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/boot/dts/broadcom/bcm2837-rpi-3-b.dts)을 검색하면 찾을 수 있습니다. 여러분이 볼 수 있듯이, 그것은 단지 `arm` 구조의 동일한 파일을 포함하고 있을 뿐이다. `ARM.v8` 프로세서도 32비트 모드를 지원한다는 점에서 일리가 있다.

다음에, [arm](https://github.com/torvalds/linux/tree/v4.14/arch/arm) 아키텍처를 포함하고 있는 [bcm2837-rpi-3-b.dts](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts)를 찾을 수 있습니다. 우리는 이미 장치 트리 파일이 다른 파일에 포함될 수 있다는 것을 보았습니다. 이는 `bcm2837-rpi-3-b.dts`의 경우로, `BCM2837`에 특정한 정의만을 포함하고 있으며, 그 밖의 모든 것을 재사용하고 있습니다. 예를 들어, `bcm2837-rpi-3-b.dts`는 [기기는 현재 1GB 메모리를 가지고 있다](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts#L18)라고 명시하고 있습니다. 

앞서 언급했듯이 `BCM2837`과 `BCM2835`는 주변 하드웨어가 동일하며, 이 하드웨어의 대부분을 실제로 정의하는 `bcm283x.dtsi`를 찾을 수 있습니다.

장치 트리 정의는 각각 중첩된 블록으로 구성됩니다. 탑레벨에서 우리는 보통 [cpus](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L30)나 [memory](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts#L17)와 같은 블록들을 찾을 수 있습니다. 그 블록들의 의미는 꽤 자기 스스로 설명 가능해야합니다.  `bcm283x.dtsi` 안에서 발견할 수 있는 다른 흥미로운 탑 레벨 요소는 [SoC](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L52)입니다. 이것은 그것은 모든 주변 장치가 메모리 맵 레지스터를 통해 어떤 메모리 영역에 직접 매핑된다는 것을 알려주는 [System on a chip](https://en.wikipedia.org/wiki/System_on_a_chip)을 의미합니다 . `soc` 요소는 모든 주변 장치의 상위 요소 역할을 합니다. 하위 요소들의 하나는 [gpio](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L147)입니다. 이 요소는 [reg = <0x7e200000 0xb4>](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L149) 속성을 GPIO 메모리 매핑 레지스터가 `[0x7e200000 : 0x7e2000b4]` 에 위치하고 있다는 것을 정의합니다. 어느`gpio` 요소의들의 [따른 정의](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L474을 가지고 있습니다.

```
uart1_gpio14: uart1_gpio14 {
        brcm,pins = <14 15>;
        brcm,function = <BCM2835_FSEL_ALT5>;
};
``` 

이 정의는 핀 14와 핀 15에 대해 대체 함수 5를 선택하면 해당 핀이 `uart1` 장치에 연결된다는 것을 알려줍니다. 이미 사용했던 `uart1` 장치가 미니 UART라는 것을 쉽게 짐작할 수 있습니다.

장치 트리에 대해 알아야 할 중요한 한 가지는 형식이 확장 가능하다는 것입니다. 각 장치는 자체 특성과 내포된 블록을 정의할 수 있습니다. 이러한 속성은 기기 드라이버로 투명하게 전달되며, 이를 해석하는 것은 드라이버의 책임입니다. 그러나 어떻게 커널이 장치 트리의 블록과 올바른 드라이버 사이의 통신을 알아낼 수 있을까요? 이를 위해 `compatible` 속성을 사용합니다. 예를 들어 `uart1` 장치의 경우 `compatible` 속성이 다음과 같이 지정됩니다.
```
compatible = "brcm,bcm2835-aux-uart";
```

그리고 실제로 리눅스 소스 코드에서 `bcm2835-aux-uart`를 검색하면 일치하는 드라이버를 찾을 수 있으며, 이는 [8250_bcm2835aux.c](https://github.com/torvalds/linux/blob/v4.14/drivers/tty/serial/8250/8250_bcm2835aux.c)에 정의되어 있습니다.

### Conclusion

여러분은 이 장에 대해 우리가 방금 탐구한 개념들을 이해하지 못한다면 `arm64` 부트 코드를 읽기 위한 준비로 생각할 수 있습니다. 다음 수업에서는 `stex`' 함수로 돌아가서 어떻게 작동하는지 자세히 살펴볼 것입니다.

##### Previous Page

1.3 [Kernel Initialization: Kernel build system](../../../docs/lesson01/linux/build-system.md)

##### Next Page

1.5 [Kernel Initialization: Exercises](../../../docs/lesson01/exercises.md)

