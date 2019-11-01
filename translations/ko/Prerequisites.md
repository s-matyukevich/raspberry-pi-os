## 필수 구성 요소

### 1. [라즈베리 파이 3 모델 B](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/)

ARMv8 아키텍처를 지원하는 64비트 프로세서를 사용하도록 설계되었으며, 그러한 프로세서는 라즈베리 파이 3에서만 사용할 수 있기 때문에 이전 버전의 라즈베리 파이에서는 이 튜토리얼과 함께 작동하지 않을 것입니다. [라즈베리 파이 3 모델 B+](https://www.raspberrypi.org/products/raspberry-pi-3-model-b-plus/)를 포함한 최신 버전은 아직 테스트하지 않았지만 잘 작동할 것입니다.

### 2. [USB와 TTL 직렬 케이블 연결](https://www.amazon.com/s/ref=nb_sb_noss_2?url=search-alias%3Daps&field-keywords=usb+to+ttl+serial+cable&rh=i%3Aaps%2Ck%3Ausb+to+ttl+serial+cable) 

직렬 케이블을 연결한 후에는 연결을 테스트해야 합니다. 이 가이드에 따르라고 권고하기 전에 이 작업을 수행한 적이 없는 경우, 직렬 케이블을 통해 라즈베리 파이를 연결하는 과정을 자세히 설명합니다. 이 가이드에서는 직렬 케이블을 사용하여 라즈베리 파이에 전력을 공급하는 방법도 설명합니다. RPi OS는 그러한 종류의 설정과 잘 작동하지만, 이 경우에는 케이블을 꽂은 후 바로 단말 에뮬레이터를 실행해야합니다. 자세한 내용은 [이 문제](https://github.com/s-matyukevich/raspberry-pi-os/issues/2)를 확인하십시오.

### 3. [라즈비언 OS가 설치된 SD 카드](https://www.raspberrypi.org/documentation/installation/sd-cards.md) with installed [Raspbian OS](https://www.raspberrypi.org/downloads/raspbian/)

우리는 처음에 USB와 TTL 케이블의 연결을 테스트하기 위해 라즈비언이 필요합니다. 또 다른 이유는 설치 후 SD 카드를 올바른 방식으로 포맷한 상태로 방치하기 때문입니다.

### 4. 도커

엄밀히 말하면 도커는 필수 의존성이 아닙니다. Docker를 사용하여 소스 코드를 만드는 것이 편리할 뿐이며, 특히 Mac과 Windows 사용자에게는 더욱 그러합니다. 수업마다 틀이 잡혀 있습니다. sh 스크립트(또는 윈도우즈 사용자를 위한 build.bat) 이 스크립트는 Docker를 사용하여 수업의 소스 코드를 작성합니다. 플랫폼용 도커를 설치하는 방법은 [공식 도커 웹 사이트](https://docs.docker.com/engine/installation/)를 참조하십시오.

몇 가지 이유로 인해 Docker 사용을 피하고 싶다면 [make 유틸리티](http://www.math.tau.ac.il/~danha/courses/software1/make-intro.html)와 aarch64-linux-gnu 툴 체인을 설치할 수 있습니다. Ubuntu를 사용하는 경우 gcc-aarch64-linux-gnu 및 빌드-필수 패키지를 설치하기만 하면 됩니다.

##### 이전 페이지

[Introduction](../ko/Introduction.md)

##### 다음 페이지

1.1 [Kernel Initialization: Introducing RPi OS, or bare metal "Hello, world!"](../ko/lesson01/rpi-os.md)

##### 추가 사항

이 문서는 훌륭한 오픈소스 운영체제 학습 프로젝트인 Sergey Matyukevich의 문서를 영어에 능숙하지 않은 한국인들이 학습할 수 있도록 번역한 것입니다. 오타 및 오역이 있을 수 있습니다. Sergey Matyukevich는 한국어를 능숙하게 다루지 못합니다. 대신 저에게 elxm6123@gmail.com으로 연락해주세요.
