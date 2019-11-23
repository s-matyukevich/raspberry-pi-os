## 3.5: 연습

1. 시스템 타이머 대신 로컬 타이머를 사용하여 프로세서 인터럽트를 생성하십시오. 자세한 내용은 [이 문제](https://github.com/s-matyukevich/raspberry-pi-os/issues/70) 를 참조하십시오.
1. MiniUART 인터럽트를 처리하십시오. `kernel_main` 함수의 마지막 루프를 아무것도하지 않는 루프로 교체하십시오. 사용자가 새 문자를 입력하자마자 인터럽트를 생성하도록 MiniUART 장치를 설정하십시오. 새로 도착한 각 문자를 화면에 인쇄하는 인터럽트 처리기를 구현하십시오.
1. qemu에서 실행되도록 Lesson 03을 적용하십시오. [이 문제](https://github.com/s-matyukevich/raspberry-pi-os/issues/8) 를 참조하십시오.

##### 다음 페이지

3.4 [Interrupt handling: Timers](../../docs/lesson03/linux/timer.md)

##### 이전 페이지

4.1 [Process scheduler: RPi OS Scheduler](../../docs/lesson04/rpi-os.md)
