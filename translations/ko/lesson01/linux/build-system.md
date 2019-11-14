
## 1.3: 커널 빌드 시스템

리눅스 커널 구조를 조사한 후, 어떻게 그것을 만들고 실행할 수 있는지 조사하는 데 시간을 들일 가치가 있습니다. 리눅스 makefile은 복잡하지만 리눅스 makefile은 커널을 만들기 위해 `make` 유틸리티를 사용합니다. 우리가 makefile을 살펴보기 전에, "kbuild"라고 불리는 Linux 빌드 시스템에 대한 몇 가지 중요한 개념을 배워봅시다.

### 몇 가지 필수적인 kbuild 개념

* kbuild 변수를 사용하여 빌드 프로세스를 사용자 정의할 수 있습니다. 이 변수들은 `Kconfig` 파일에 정의돼 있습니다. 여기서 변수 자체와 변수 기본값을 정의할 수 있습니다. 변수는 문자열, 부울 및 정수를 포함하여 다른 유형을 가질 수 있습니다. Kconfig 파일에서 변수 간의 종속성을 정의할 수도 있습니다(예를 들어 변수 X를 선택하면 변수 Y가 암묵적으로 선택된다고 말할 수 있습니다). [arm64 Kconfig file](https://github.com/torvalds/linux/tree/v4.14/arch/arm64/Kconfig)를 예로 볼 수 있습니다. 이 파일은 'arm64' 아키텍처에 특정한 모든 변수를 정의합니다. `Kconfig` 기능은 표준 make의 일부가 아니며 리눅스 makefile에서 구현됩니다. `Kconfig`에서 정의한 변수는 중첩된 파일뿐만 아니라 커널 소스 코드에도 노출됩니다. 커널 구성 단계에서 변수 값을 설정할 수 있습니다.

* 리눅스는 재귀적인 빌딩을 합니다. 리눅스 커널의 각 하위 폴더는 자체 `Makefile`과 `Kconfig`를 하위 폴더가 정의할 수 있다는 뜻입니다. 중첩된 대부분의 Makefile들은 매우 단순하며 어떤 객체 파일을 컴파일해야 하는지 정의하기만 하면 됩니다. 일반적으로 그러한 정의는 다음과 같은 형식을 가지고 있습니다.

  ```
  obj-$(SOME_CONFIG_VARIABLE) += some_file.o
  ```
 
  이 정의는 `SOME_CONFIG_VARIABLE`을 설정한 경우에만 `some_file.c`를 컴파일하고 커널에 링크한다는 것을 의미합니다. 파일을 무조건 컴파일하고 연결하려면 이전 정의를 이렇게 변경해야 합니다.

  ```
  obj-y += some_file.o
  ```

  중첩 Makefile의 예는 [여기](https://github.com/torvalds/linux/tree/v4.14/kernel/Makefile)에서 찾을 수 있습니다.An example of the nested Makefile can be found [here](https://github.com/torvalds/linux/tree/v4.14/kernel/Makefile).

* 더 나아가기 전에, 기본적인 빌딩 규칙의 구조를 이해하고 make 용어에 익숙해져야 합니다. 공통 규칙 구조는 다음 도표에 설명되어 있습니다.

  ```
  targets : prerequisites
          recipe
          …
  ```
    * `targets` 스페이스들에 의해서 분리된 파일 이름들입니다. 타겟은 규칙이 수행된 이후 생성됩니다. 보통 타겟 당 하나의 규칙이 존재합니다.
    * `prerequisites` 은 타겟들이 업데이트가 필요한지 트래킹 할 수 있게 해주는 파일들입니다.
    * `recipe` 는 bash 스크립트입니다.  prerequisites 중 일부가 업데이트되었을 때 호출됩니다. recipe은 목표물을 생성하는데 책임이 있습니다.
    * 타겟과 prerequisites는 모두 와일드 카드("%")를 포함할 수 있습니다. 와일드 카드를 사용할 경우, 각각의 캐시된 prerequisites에 대해 별도의 recipe를 실행합니다. 이 경우, `$<`와 `$@` 변수를 사용하여 recipe 내부의 prerequisites와 타겟을 참조할 수 있습니다. 우리는 이미 [RPi OS makefile](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/Makefile#L14)을 완성했습니다. 좀 더 추가적인 make 규칙을 알고 싶다면 [공식 문서](https://www.gnu.org/software/make/manual/html_node/Rule-Syntax.html#Rule-Syntax)를 참조하세요.

* `make` 는 prerequisites가 변경되었는지 탐지하고 재구축해야 하는 대상만을 업데이트할 때 유용합니다. 그러나 recipe이 동적으로 업데이트되면 `make` 는 이 변화를 감지할 수 없습니다. 어떻게 이런 일이 일어날 수 있을까요? 메우 쉽게 한 가지 좋은 예는 일부 구성 변수를 변경할 때 레시피에 추가 옵션을 추가하는 것이다. 기본적으로, 이 경우,  `make`는 이전에 생성된 객체 파일을 재구성하지 않습니다. 그 prerequisites은 변경되지 않았고, recipe만 업데이트되었기 때문입니다. 이 동작을 해결하기 위해서 리눅스는 [if_changed](https://github.com/torvalds/linux/blob/v4.14/scripts/Kbuild.include#L264) 함수를 도입했습니다. 어떻게 작동하는지 보기 위해서 다음의 예를 생각해봅시다.

  ```
  cmd_compile = gcc $(flags) -o $@ $<

  %.o: %.c FORCE
      $(call if_changed,compile)
  ```

  여기서 각 `.c` 파일에 대해 compile이라는 인수로 `if_changed"`함수를 호출하여 해당 `.o` 파일을 작성합니다. 만약 'cmd_compile' 명령이 실행되고 객체 파일이 재생성되는 경우에 `if_changed`는 `cmd_compile` 변수(첫 번째 인수에 `cmd_` 접두사를 추가함)를 찾아 이 변수가 마지막 실행 이후 업데이트 되었는지 또는 prerequisites이 변경되었는지 확인합니다. 샘플 규칙은 소스 .c 파일과 FORCE라는 두 가지 필수조건이 있습니다. `FORCE`는 `make` 명령을 부를 때마다 recipe를 불러야 하는 특별한 prerequiste입니다. 만약 없다면 레시피는 .c 파일을 바꿔야 합니다. `FORCE` 타겟에 대해서 [여기](https://www.gnu.org/software/make/manual/html_node/Force-Targets.html)를 통해 좀 더 읽을 수 있습니다.

### 커널 빌딩하기

이제 Linux 빌드 시스템에 대한 몇 가지 중요한 개념을 배웠을 때 make 명령을 입력한 후 정확히 어떤 일이 벌어지고 있는지 알아봅시다. 이 과정은 매우 복잡하고 많은 세부사항들을 포함하고 있는데, 우리는 대부분 생략할 것입니다. 우리의 목표는 2개의 질문에 답하는 것입니다.

1. 소스 파일이 객체 파일로 정확히 어떻게 컴파일되는가?
1. 오브젝트 파일이 OS 이미지에 어떻게 연결되어 있는가?

우리는 두 번째 문제를 먼저 다룰 것이다.

#### 링크 스테이지

* `make help`명령의 결과로 부터 너가 보려고 할때, 그 커널 빌딩의 책임이 있는 기본 결과는 `vmlinx`라고 부릅니다.
* `vmlinux` 타겟 정의는 [여기](https://github.com/torvalds/linux/blob/v4.14/Makefile#L1004)에서 찾을 수 있고 이것은 아래와 같습니다.

  ```
  cmd_link-vmlinux =                                                 \
      $(CONFIG_SHELL) $< $(LD) $(LDFLAGS) $(LDFLAGS_vmlinux) ;    \
      $(if $(ARCH_POSTLINK), $(MAKE) -f $(ARCH_POSTLINK) $@, true)

  vmlinux: scripts/link-vmlinux.sh vmlinux_prereq $(vmlinux-deps) FORCE
      +$(call if_changed,link-vmlinux)
  ```

  이 타겟은 이미 친숙한  `if_changed` 함수를 사용합니다. 일부 prerequsities를 업데이트될 때마다 `cmd_link-vmlinux` 커맨드는 실행됩니다. 이 커맨드는 [scripts/link-vmlinux.sh](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh) 스크르립트를 실행합니다. ( [$<](https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html)의 사용은 `cmd_link-vmlinux` 명령어 안에 자동적인 변수임을 강조합니다.). 이것은 또한 [postlink script](https://github.com/torvalds/linux/blob/v4.14/Documentation/kbuild/makefiles.txt#L1229)라는 구체적인 아키텍처를 실행합니다, 그러나 이것까지 신경쓸 필요는 없습니다.

* [scripts/link-vmlinux.sh](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh)을 실행할 때 3가지 오브젝트 파일이 이미 작되어 있고 해당 위치가 세 가지 변수(`KBUILD_VMLINUX_INIT`, `KBUILD_VMLINUX_MAIN`, `KBUILD_VMLINUX_LIBS`)로 작성되어 있음을 가정합니다.  

* `link-vmlinux.sh` 스크립트는 먼저 `thin archive`을 모든 개체 파일로부터 만듭니다. `thin archive`는 개체 파일 집합과 결합된 기호 표를 참조하는 특별한 오브젝트 파일입니다. 이것은 [archive_builtin](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L56)함수 내에서 이루어집니다.  `thin archive`함수를 만들기 위해서 이 함수는 [ar](https://sourceware.org/binutils/docs/binutils/ar.html) 유틸리를 사용합니다. 생성된 `thin archive`은 `built-in.o` 파일 안에 저장되고 링커에 의해서 이해될만한 포맷을 가지고 있고 그래서 이것은 다른 일반적인 오브젝트 파일로 사용할 수 있도록 합니다.

* 다음은 [modpost_link](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L69) 라고 부릅니다 . 이 함수는 링커를 호출하고  `vmlinux.o` 오브젝트 파일을 생성합니다. 우리는 [Section mismatch analysis](https://github.com/torvalds/linux/blob/v4.14/lib/Kconfig.debug#L308) 이 오브젝트 파일이 필요합니다. 이 분석은 [modpost](https://github.com/torvalds/linux/tree/v4.14/scripts/mod)에 의해서 수행되는 프로그램이고 [이 라인](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L260)에서 실행됩니다.

* 다음 커널 기호 테이블이 생성됩니다. 이것은 `vmlinux` 바이너리 안에서의 위치 뿐만 아니라 모든 함수와 전역 변수에 대한 정보를 포함하고 있습니다. 주요한 작업은 [kallsyms](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L146) 함수 내에서 이뤄집니다. 이 함수는 첫번째로 `vmlinux`바이너리에서 모든 기호를 추출하기 위해서 [nm](https://sourceware.org/binutils/docs/binutils/nm.html)을 사용합니다. 그때에 이것은 리눅스 커널에서 이해할 수 있는 모든 기호를 포함하는 특수 어셈블러 파일을 발생시키기 위해 [scripts/kallsyms](https://github.com/torvalds/linux/blob/v4.14/scripts/kallsyms.c) 유틸리티를 사용합니다. 다음으로, 이 어셈블러 파일을 컴파일하여 원래의 바이너리로 연결합니다. 이 프로세스는 일부 기호의 최종 링크 주소를 변경할 수 있기 때문에 여러 번 반복됩니다. 커널 기호 테이블의 정보는 런타임에 '/proc/kallsyms' 파일을 생성하는 데 사용됩니다.

* 마지막으로 `vmlinux` 바이너리가 준비되었고 `System.map`이 제작되었습니다. `System.map`는 [kernel oops](https://en.wikipedia.org/wiki/Linux_kernel_oops)와 같은 정보가 들어 있지만, 정적 파일이며 `/proc/kallsyms`와 달리 런타임에 생성되지 않습니다. 같은 `nm` 유틸리티는 `System.map`를 빌드하기 위해서 사용됩니다. 이것은 [here](https://github.com/torvalds/linux/blob/v4.14/scripts/mksysmap#L44)에서 수행됩니다.

#### 빌드 스테이지

* 이제 한 걸음 뒤로 물러나 소스 코드 파일이 오브젝트 파일로 어떻게 컴파일되는지 살펴보자. 'vmlinux' 타겟의 prerequisites 중 하나는 '$(vmlinux-deps)' 변수라는 것을 기억할 것입니다. 이제 메인 Linux makefile에서 몇 개의 관련 라인을 복사하여 이 변수가 작성되는 방법을 시연해 보이도록 하겠습니다.

  ```
  init-y        := init/
  drivers-y    := drivers/ sound/ firmware/
  net-y        := net/
  libs-y        := lib/
  core-y        := usr/

  core-y        += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/

  init-y        := $(patsubst %/, %/built-in.o, $(init-y))
  core-y        := $(patsubst %/, %/built-in.o, $(core-y))
  drivers-y    := $(patsubst %/, %/built-in.o, $(drivers-y))
  net-y        := $(patsubst %/, %/built-in.o, $(net-y))

  export KBUILD_VMLINUX_INIT := $(head-y) $(init-y)
  export KBUILD_VMLINUX_MAIN := $(core-y) $(libs-y2) $(drivers-y) $(net-y) $(virt-y)
  export KBUILD_VMLINUX_LIBS := $(libs-y1)
  export KBUILD_LDS          := arch/$(SRCARCH)/kernel/vmlinux.lds

  vmlinux-deps := $(KBUILD_LDS) $(KBUILD_VMLINUX_INIT) $(KBUILD_VMLINUX_MAIN) $(KBUILD_VMLINUX_LIBS)
  ```

  이 모든 것은 빌드 가능한 소스 코드를 포함하는 리눅스 커널의 모든 하위 폴더를 포함하는 `init-y`, `core-y` 등과 같은 변수들로 시작합니다. 모든 하위 폴더 이름에 `Built-in.o`가 추가되므로, 예를 들어 `drivers/`는 `drivers/built-in.o`가 됩니다. `vmlinux-deps`는 모든 결과 값을 집계합니다. 결국 모든 `build-in.o` 파일에 의존하게 된 것도 이 때문입니다.

* 다음 질문은 모든 `built-in.o` 객체들이 어떻게 만들어 지는가?입니다. 다시 한번 모든 관련된 라인들을 복사하여 그것이 어떻게 작동하는지 설명하겠습니다.

  ```
  $(sort $(vmlinux-deps)): $(vmlinux-dirs) ;

  vmlinux-dirs    := $(patsubst %/,%,$(filter %/, $(init-y) $(init-m) \
               $(core-y) $(core-m) $(drivers-y) $(drivers-m) \
               $(net-y) $(net-m) $(libs-y) $(libs-m) $(virt-y)))

  build := -f $(srctree)/scripts/Makefile.build obj               #Copied from `scripts/Kbuild.include`

  $(vmlinux-dirs): prepare scripts
      $(Q)$(MAKE) $(build)=$@

  ```

  첫번째 줄은 `vmlinux-deps`가 `vmlinux-dirs`에 달려 있다고 말합니다.  다음 `/` 문자에 상관 없이 직접적인 루트 하위 폴더를 포함한다는 `vmlinux-dirs` 변수를 볼 수 있습니다. 그리고 여기서 가장 중요한 라인은 `$(vmlinux-dirs)`  타겟을 빌드하는 recipe입니다. 모든 변수들의 대체 후 이 recipe은 다음과 같습니다. (`drivers` 폴더를 예로 들지만, 이 규칙은 모든 루트 하위 폴더에 대해 실행될 것입니다.)

  ```
  make -f scripts/Makefile.build obj=drivers
  ```


  이 라인은 다른 makefile  ([scripts/Makefile.build](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build))릏 호출하고 컴파일할 폴더가 들어 있는 `obj` 변수를 통과시킵니다. 

* 다음 논리적인 단계는 [scripts/Makefile.build](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build)을 한번 살펴 보는 것입니다. 실행 후 가장 중요한 것은 현재 디렉토리에 정의된 `Makefile`이나 `Kbuild` 파일의 모든 변수들이 수행된 이후에 포함되는 것입니다. 현재 디렉토리로 나는 `obj` 변수에 의해 참조되는 디렉토리를 의미합니다. 이는 [뒤의 3라인](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L43-L45) 안에 있습니다.

  ```
  kbuild-dir := $(if $(filter /%,$(src)),$(src),$(srctree)/$(src))
  kbuild-file := $(if $(wildcard $(kbuild-dir)/Kbuild),$(kbuild-dir)/Kbuild,$(kbuild-dir)/Makefile)
  include $(kbuild-file)
  ```
  
  중첩된 makefiles 대부분 `obj-y` 같은 변수를 초기화하기 위한 책임이 있습니다. 빠르게 상기시키는 것으로서: `obj-y` 변수 모든 소스 코드 파일, 현재 디렉토리에 위치한 목록이 포함되어야 합니다. 중첩 makefile들에 의해 초기화되는 또 다른 중요한 변수는 `subdir-y`입니다. 이 변수에는 cheptnt 디렉토리의 소스 코드를 작성하기 전에 방문해야 하는 모든 하위 폴더 목록이 포함되어 있습니다. `subdir-y` 는 하위폴더에 재귀적으로 방문하는 것을 구현하기 위해 사용됩니다.

* `make`는 타겟을 구체화하는 것 없이 호출될 때 첫번째 타겟을 사용합니다. `scripts/Makefile.build`을 위한 첫번째 타겟은  `__build`에 의해 호출되고 이것은 [여기](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L96)에서 확인 할 수 있습니다. 한번 살펴 봅시다.

  ```
  __build: $(if $(KBUILD_BUILTIN),$(builtin-target) $(lib-target) $(extra-y)) \
       $(if $(KBUILD_MODULES),$(obj-m) $(modorder-target)) \
       $(subdir-ym) $(always)
      @:
  ```

  보다시피 `__build` 타겟은은 recipe은 없지만 다른 타겟에 따라 다릅니다. `built-in.o`파일 작성을 담당하는 `$(builtin-target)`와 중첩된 디렉토리에 내림하는 책임을 맡고 있는 `$(subdir-ym)`에만 관심이 있습니다.

*  `subdir-ym`을 한법 봅시다. 이 변수는 [여기](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.lib#L48)에서 초기화되고 이것은 `subdir-y`와 `subdir-m` 변수의 합성입니다. 

*  `subdir-ym` 타겟은 [여기](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L572)에서 정의되고 우리에게 매우 익숙합니다.

  ```
  $(subdir-ym):
      $(Q)$(MAKE) $(build)=$@
  ```

  이 타겟은 중첩된 서브 폴더 중에 하나에 있는 `scripts/Makefile.build`의 실행을 트리거할 뿐입니다.

* 이제 [builtin-target](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L467) 타겟에 대해서 설명하겠습니다. 일단 여기에 관련된 라인만 복사하겠습니다.

  ```
  cmd_make_builtin = rm -f $@; $(AR) rcSTP$(KBUILD_ARFLAGS)
  cmd_make_empty_builtin = rm -f $@; $(AR) rcSTP$(KBUILD_ARFLAGS)

  cmd_link_o_target = $(if $(strip $(obj-y)),\
                $(cmd_make_builtin) $@ $(filter $(obj-y), $^) \
                $(cmd_secanalysis),\
                $(cmd_make_empty_builtin) $@)

  $(builtin-target): $(obj-y) FORCE
      $(call if_changed,link_o_target)
  ```

  이 타겟은 `$(obj-y)` 타겟에 따라 달라지고 `obj-y`는 현재 폴더에 빌드해야하는 모든 객체 파일의 목록입니다. 해당 파일이 준비되면  `cmd_link_o_target` 명령이 실행됩니다. 이 `obj-y` 변수가 비어 있는경우 `cmd_make_empty_builtin`이 호출되면 빈 `built-in.o`를 생성합니다. 반면에 `cmd_make_builtin` 명령이 실행해서 익숙한 `ar` 툴을 사용해서 `built-in.o` 아카이브를 만듭니다.

* 마침내 우리는 무언가를 편집해야할 단계입니다. 마지막 미개척 종속성은 `$(obj-y)`이고 `obj-y`는 객체 리스트에 불과한다는 것을 기억할 겁니다. 사응하는 .c 파일들로부터 컴파일하는 그 타켓은 [여기에서](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L313) 정의됩니다. 이 타겟을 이해하는데 필요한 모든 라인을 살펴봅시다.

  ```
  cmd_cc_o_c = $(CC) $(c_flags) -c -o $@ $<

  define rule_cc_o_c
      $(call echo-cmd,checksrc) $(cmd_checksrc)              \
      $(call cmd_and_fixdep,cc_o_c)                      \
      $(cmd_modversions_c)                          \
      $(call echo-cmd,objtool) $(cmd_objtool)                  \
      $(call echo-cmd,record_mcount) $(cmd_record_mcount)
  endef

  $(obj)/%.o: $(src)/%.c $(recordmcount_source) $(objtool_dep) FORCE
      $(call cmd,force_checksrc)
      $(call if_changed_rule,cc_o_c)
  ```

  내부에서는 이 타겟을 `rule_cc_o_c`라고 부릅니다. 이 규칙은 보통 에러를 위한 소스 코드 체크(`cmd_checksrc`) 또는 외부 모듈 심볼(`cmd_modversions_c`)을 위한 버전을 가능하게하는 것 또는 [objtool](https://github.com/torvalds/linux/tree/v4.14/tools/objtool)을 사용하여 생성된 개체 파일의 일부 측면을 검증하고 [ftrace](https://github.com/torvalds/linux/blob/v4.14/Documentation/trace/ftrace.txt)가 신속하게 찾을 수 잇도록 `mcount`함수에 대한 통화 목록을 구성하는 등같은 많은 것들을 담당합니다. 그러나 가장 중요한 것은 모든 `.c` 파일을 객체 파일에 컴파일하는 `cmd_cc_o_c` 명령어입니다.
  
### 결론

와우, 커널 빌드 시스템 내부로 가는 긴 여정이었어요! 그럼에도 불구하고, 우리는 많은 세부사항을 생략했고, 이 주제에 대해 더 알고 싶어하는 사람들을 위해 나는 다음의 [문서](https://github.com/torvalds/linux/blob/v4.14/Documentation/kbuild/makefiles.txt)들을 읽는 것을 추천하고 Makefiles 코드 읽는 것을 계속하겠습니다. 지금 중요한 점을 강조하는 당신이 이 장에서 가져갈 중요한 메시지입니다.

1. How `.c` files are compiled into object files.
1. How object files are combined into `build-in.o` files.
1. How  recursive build pick up all child `build-in.o` files and combines them into a single one.
1. How `vmlinux` is linked from all top-level `build-in.o` files.

1. 어떻게 `.c` 파일들이 오브젝트 파일들로 컴파일되는지.
1. 어떻게 오브젝트파일들이 `build-in.o` 파일들로 합쳐지는지.
1. 어떻게 재귀적인 빌드가 모든 하위  `build-in.o` 파일들을 선택하고 하나로 결합 시키는지.
1. 어떻게 `vmlinux`가 톱레벨의 `build-in.o` 파일들로부터 링크되는지.

나의 주된 목표는 이 장을 읽고 나면 위의 모든 사항에 대한 일반적인 이해를 얻는 것이었습니다.
##### Previous Page

1.2 [Kernel Initialization: Linux project structure](./project-structure.md)

##### Next Page

1.4 [Kernel Initialization: Linux startup sequence](./kernel-startup.md)

