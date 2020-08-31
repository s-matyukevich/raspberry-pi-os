## 5.3: Exercises

1.  task가 사용자 모드에서 실행될 때, 일부 시스템 레지스터에 접근해보아라. 이 경우 동기적 예외가 발생하는지 확인해라. 이 예외를 처리하려면 `esr_el1` 레지스터를 사용해 시스템 콜과 구분지어라.
1.  현재 task의 우선순위를 설정하기 위한 새로운 시스템 콜을 구현해보아라. task가 실행되는 동안 동적으로 우선 순위를 바꾸는 방법을 보여라.
1.  lesson05의 소스코드가 qemu에서 실행되도록 수정하라. 다음 링크를 참고하라.

	[https://github.com/s-matyukevich/raspberry-pi-os/issues/8](https://github.com/s-matyukevich/raspberry-pi-os/issues/8)
##### Previous Page

5.2 [User processes and system calls: Linux](../../docs/lesson05/linux.md)

##### Next Page

6.1 [Virtual memory management: RPi OS](../../docs/lesson06/rpi-os.md)