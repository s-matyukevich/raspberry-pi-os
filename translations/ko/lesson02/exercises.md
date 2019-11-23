## 2.3: 연습

1. EL3에서 EL1로 바로 점프하는 대신 먼저 EL2로 이동 한 다음 EL1로 전환하십시오.
1. 이 레슨에서 작업할 때 우연히 발견한 한 가지 문제는 FP/SIMD 레지스터를 사용할 경우 모든 것이 EL3에서는 잘 작동하지만 EL1의 인쇄 기능에 도달하는 즉시 작동이 중지된다는 것이었습니다.  이것이 내가 컴파일러 옵션에 [-mgeneral-regs-only](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/Makefile#L3) 매개 변수를 추가한 이유였습니다. 이제 이 파라미터를 제거하고 이 행동을 재현해 주길 바랍니다. 다음으로 `objdump` 도구를 사용하여 gcc가 `-meneral-regs only` 플래그가 없을 때 FP/SIMD 레지스터를 정확히 사용하는 방법을 확인할 수 있습니다. 마지막으로 FP/SIMD 레지스터를 사용할 수 있도록 'cpak_el1'을 사용하십시오. 
1. qemu에서 실행되도록 레슨 2에 적용하십시오. 이 issue를 [확인](https://github.com/s-matyukevich/raspberry-pi-os/issues/8)하십시오.

##### Previous Page

2.2 [Processor initialization: Linux](../lesson02/linux.md)

##### Next Page

3.1 [Interrupt handling: RPi OS](../lesson03/rpi-os.md)

##### 추가 사항

이 문서는 훌륭한 오픈소스 운영체제 학습 프로젝트인 Sergey Matyukevich의 문서를 영어에 능숙하지 않은 한국인들이 학습할 수 있도록 번역한 것입니다. 오타 및 오역이 있을 수 있습니다. Sergey Matyukevich는 한국어를 능숙하게 다루지 못합니다. 대신 저에게 elxm6123@gmail.com으로 연락해주세요.
