## 1.2: 리눅스 프로젝트 구조

리눅스에 대해 말하겠습니다. 우선 우리의 커널이 쓰는 작은 단계를 완료하고 나서 리눅스에서도 같은 단계들이 어떻게 작동하는지 살펴볼 것입니다. 지금까지 우리는 조금 밖에 진행하지 않았습니다. 단지 우리의 첫 번째 베어 메탈 헬로우 월드 프로그램을 구현했습니다. 하지만 RPi OS와 Linux 사이의 유사점을 찾을 수 있을 것입니다. 그리고 이제부터 그들 중 일부를 탐험할 것이다.

### 프로젝트 구조

대형 소프트웨어 프로젝트를 조사할 때마다 프로젝트 구조를 잠깐 살펴볼 가치가 있습니다. 이것은 프로젝트를 구성하는 모듈들과 높은 수준의 아키텍처를 이해할 수 있게 해주기 때문에 매우 중요합니다. 이제 Linux 커널의 프로젝트 구조를 살펴봅시다.

우선 리눅스 레포지토리를 복제해야 합니다.

```
git clone https://github.com/torvalds/linux.git 
cd linux
git checkout v4.14
```

"v4.14 버전은 작성 당시 최신 버전이었기 때문에 이를 사용하겠습니다. Linux 소스 코드에 대한 모든 참조는 이 특정 버전을 사용하여 이루어질 것입니다. 다음으로 리눅스 리포지토리 안에서 찾을 수 있는 폴더를 살펴보도록 합시다. 우리는 그 모든 것들을 볼 것이 아니라, 가장 중요하게 생각하는 것들로 시작할 것입니다. 

* [arch](https://github.com/torvalds/linux/tree/v4.14/arch) 이 폴더 각각 특정 프로세서 아키텍처에 대해 하위 폴더를 포함합니다. 대게 우리는 [arm64](https://github.com/torvalds/linux/tree/v4.14/arch/arm64)를 사용하여 작업할 것입니다. - 이건 ARM.v8 프로세서들과 호환됩니다.
* [init](https://github.com/torvalds/linux/tree/v4.14/init) 커널은 항상 구체적인 코드 구조에 의해 부팅됩니다. 그러나 실행은 커널 초기화와 독립적인 커널 출발점 아키텍처를 책임하고 있는 [start_kernel](https://github.com/torvalds/linux/blob/v4.14/init/main.c#L509) 함수로 전달됩니다. `start_kernel` 함수는 다른 초기화 기능과 함께 `init`폴더에 정의되어 있습니다.
* [kernel](https://github.com/torvalds/linux/tree/v4.14/kernel) 이것이 리눅스 커널의 핵심입니다. 거의 모든 주요 커널 서브시스템이 이곳에서 구현됩니다.
* [mm](https://github.com/torvalds/linux/tree/v4.14/mm) 메모리 관리와 관련된 모든 데이터 구조와 방법이 여기서 정의됩니다.
* [drivers](https://github.com/torvalds/linux/tree/v4.14/drivers) 이것은 리눅스 커널에서 가장 큰 폴더입니다. 여기에는 모든 장치 드라이버의 구현이 포함되어 있습니다.
* [fs](https://github.com/torvalds/linux/tree/v4.14/fs) 다른 파일 시스템 구현을 찾으려면 여기를 클릭하십시오.

이 설명은 매우 어려운 설명이지만, 현재 이것으로 충분합니다. 다음 장에서는 리눅스 빌드 시스템을 자세히 살펴보겠습니다.

##### 이전 페이지

1.1 [Kernel Initialization: Introducing RPi OS, or bare metal "Hello, world!"](./lesson01/rpi-os.md)

##### 다음 페이지

1.3 [Kernel Initialization: Kernel build system](./build-system.md)
