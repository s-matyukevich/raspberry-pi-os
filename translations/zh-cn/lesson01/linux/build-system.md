## 1.3: 内核构建系统

Linux也使用`make`实用程序来构建内核，尽管`Linux makefile`要复杂得多。在看`makefile`之前，让我们学习一些有关Linux构建系统的重要概念，称为 `kbuild` 。

### 一些基本的kbuild概念

* 可以使用kbuild中的变量自定义构建过程。这些变量在 `Kconfig` 文件中定义。在这里，您可以定义变量本身及其默认值。变量可以具有不同的类型，包括字符串，布尔值和整数。在`Kconfig`文件中，您还可以定义变量之间的依赖关系（例如，如果选择了变量X，则可以隐式选择变量Y）。例如，您可以看一下 [arm64 Kconfig文件](https://github.com/torvalds/linux/tree/v4.14/arch/arm64/Kconfig)。该文件定义了所有变量，特定于`arm64`体系结构。 Kconfig功能不是标准`make`的一部分，而是在`Linux makefile`中实现的。在 `Kconfig` 中定义的变量公开给内核源代码以及嵌套的`makefile`。可以在内核配置步骤中设置变量值（例如，如果键入`make menuconfig`，则会显示一个控制台`GUI`。它允许自定义所有内核变量的值并将这些值存储在`.config`中 ,`help` 命令以查看所有可能的选项来配置内核）
* Linux使用递归构建。这意味着Linux内核的每个子文件夹都可以定义它自己的`Makefile`和`Kconfig`。大多数嵌套`Makefile`文件都非常简单，只是定义文件需要编译什么对象。

通常情况下，这样的定义具有以下格式。

  ```
  obj-$(SOME_CONFIG_VARIABLE) += some_file.o
  ```

  这个定义意味着`some_file.c`只会在设置`SOME_CONFIG_VARIABLE`的情况下编译并链接到内核。如果要无条件编译和链接文件，则需要更改以前的定义，如下所示。
  ```
  obj-y += some_file.o
  ```

  可以找到嵌套的Makefile的示例在[这](https://github.com/torvalds/linux/tree/v4.14/kernel/Makefile).

* 在继续前进之前，您需要了解基本make规则的结构并熟悉make术语。

下图说明了通用规则结构。

  ```
  targets : prerequisites
          recipe
          …
  ```
  
  * `targets` 是文件名，用空格分隔。执行规则后将生成目标文件。通常，每个规则只有一个目
  * `prerequisites` 是`make`跟踪的文件，以查看是否需要更新目标文件。
  * `recipe` 是一个`bash`脚本。在某些`prerequisites`更新后进行调用。`recipe`负责生成目标文件。
  * `targets` 和 `Prerequsites` 都可以包含通配符（％）。使用通配符时，将分别针对每个已实现的`prerequisites`执行`recipe`。在这种情况下，可以使用`$ <`和`$ @`变量来引用`recipe`中的`prerequisites`和`targets`。我们已经在 [RPi OS makefile](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/Makefile#L14) 中完成了此操作。
  有关制作规则的其他信息，请参考 [官方文档](https://www.gnu.org/software/make/manual/html_node/Rule-Syntax.html#Rule-Syntax)。
  * `make` 可以很好地检测是否已更改任何`prerequisites`，并且仅更新需要重建的目标文件。但是，如果`recipe`是动态更新的，则 `make` 将无法检测到此更改。怎么会这样? 非常简单。一个很好的例子是，当您更改一些配置变量时，这会导致在`recipe`中附加一个附加选项。默认情况下，`make`将不会重新编译以前生成的目标文件，因为它们的`prerequisites`没有改变，只有`recipe`已更新。为了应对此行为，Linux引入了[if_changed](https://github.com/torvalds/linux/blob/v4.14/scripts/Kbuild.include#L264) 函数。
  
  要查看其工作原理，请考虑以下示例。
  
  ```
  cmd_compile = gcc $(flags) -o $@ $<

  %.o: %.c FORCE
      $(call if_changed,compile)
  ```

  在这里对于每个`.c`文件，我们通过使用带有`compile`参数的`if_changed`函数来构建相应的`.o`文件。然后，`if_changed`查找`cmd_compile`变量（在第一个参数后添加`cmd_`前缀），并检查此变量自上次执行以来是否已更新，或者任何`prerequisites`已更改。如果是，则执行 `cmd_compile` 命令并重新生成目标文件。我们的示例规则有两个`prerequisites`源 `.c`文件和`.0` 文件。 `FORCE` 是一个特殊的`prerequisites`，它强制每次调用 `make` 命令时都调用`recipe`。没有它，只有在`.c`文件被更改的情况下才会调用配方。您可以阅读有关 `FORCE`目标的更多信息在[这](https://www.gnu.org/software/make/manual/html_node/Force-Targets.html).

### 构建内核

现在，当我们了解了有关Linux构建系统的一些重要概念时，让我们尝试弄清楚在键入`make`命令后到底发生了什么。这个过程非常复杂，并且包含很多细节，我们将跳过其中的大多数细节。我们的目标是回答2个问题。

1. 源文件如何精确地编译为目标文件？
2. 目标文件如何链接到OS映像中？

我们将首先解决第二个问题。

#### 链接阶段

* 您可能会从`make help`命令的输出中看到，负责构建内核的默认目标称为`vmlinux`。

* `vmlinux` 目标定义可以在 [这](https://github.com/torvalds/linux/blob/v4.14/Makefile#L1004) 找到，并且看起来像这样

  ```
  cmd_link-vmlinux =                                                 \
      $(CONFIG_SHELL) $< $(LD) $(LDFLAGS) $(LDFLAGS_vmlinux) ;    \
      $(if $(ARCH_POSTLINK), $(MAKE) -f $(ARCH_POSTLINK) $@, true)

  vmlinux: scripts/link-vmlinux.sh vmlinux_prereq $(vmlinux-deps) FORCE
      +$(call if_changed,link-vmlinux)
  ```

  这个目标使用我们已经熟悉的`if_changed`函数。 每当某些先决条件被更新时，将执行 `cmd_link-vmlinux` 命令。此命令执行 [scripts /link-vmlinux.sh](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh) 脚本（注意[$<](https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html)  为 `cmd_link-vmlinux` 命令中的自动变量）。它还执行特定于体系结构的 [postlink脚本](https://github.com/torvalds/linux/blob/v4.14/Documentation/kbuild/makefiles.txt#L1229)，但我们对此并不十分感兴趣。

* 当执行 [scripts /link-vmlinux.sh](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh) 时，它假定所有必需的目标文件均已构建并且它们的位置存储在3个变量中：`KBUILD_VMLINUX_INIT`，`KBUILD_VMLINUX_MAIN`和`KBUILD_VMLINUX_LIBS`。

* `link-vmlinux.sh` 脚本首先创建 `thin archive` 从所有可用的目标文件. `thin archive` 是一个特殊的对象，其中包含对一组目标文件及其组合符号表的引用. 这是在[archive_builtin](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L56) 函数内部完成的。为了创建`thin archive` 使用了 [ar](https://sourceware.org/binutils/docs/binutils/ar.html) 实现。 产生的 `thin archive` 被储存在 `built-in.o` 文件中并具有链接器可以理解的格式, 因此它可以用作任何其他普通目标文件。

* 下一个 [modpost_link](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L69) 被调用. 该函数调用链接器并生成`vmlinux.o`目标文件。我们需要这个目标文件来执行[断面失配分析](https://github.com/torvalds/linux/blob/v4.14/lib/Kconfig.debug#L308). 该分析由 [modpost](https://github.com/torvalds/linux/tree/v4.14/scripts/mod) 程序组成并被触发在 [这](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L260) 行.

* 下一个生成内核符号表。它包含有关所有函数和全局变量的信息，以及它们在`vmlinux`二进制文件中的位置。 主要工作在 [kallsyms](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L146) 函数内部完成. 该函数首先使用 [nm](https://sourceware.org/binutils/docs/binutils/nm.html) 从 `vmlinux` 二进制文件中提取所有符号. 然后，它使用 [scripts /kallsyms](https://github.com/torvalds/linux/blob/v4.14/scripts/kallsyms.c) 实用程序生成一个特殊的汇编器文件，其中包含所有格式特殊的符号，可以被理解 `Linux`内核。接下来，将编译此汇编器文件并将其与原始二进制文件链接在一起。因为可以更改某些符号的最终链接地址，所以此过程重复了几次。来自内核符号表的信息用于在运行时生成 `/proc /kallsyms` 文件。

* 最后，`vmlinux`二进制文件已经准备好，并且`System.map`已经构建。`System.map` 包含与 `/proc/kallsyms` 相同的信息，但这是静态文件与 `/proc/kallsyms` 的不同，它不是在运行时生成的。 `System.map`主要用于在[kernel oops](https://en.wikipedia.org/wiki/Linux_kernel_oops) 期间将地址解析为符号名称。 相同的`nm`实用程序用于构建`System.map`。 这个完成在 [这](https://github.com/torvalds/linux/blob/v4.14/scripts/mksysmap#L44).

#### 建立阶段 

* 现在让我们向后退一步，检查源代码文件如何编译为目标文件。您可能还记得，`vmlinux`目标的先决条件之一是 `$(vmlinux-deps)` 变量。现在，让我从Linux `主makefile` 中复制一些相关的行，以演示如何构建此变量。

  ```makefile
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

  所有这些都以变量 `init-y`，`core-y`等开头。 它们组合在一起，包含Linux内核的所有子文件夹，这些子文件夹包含可构建的源代码。然后，在所有子文件夹名称后附加 `built-in.o`，因此，例如， `drivers/` 成为 `drivers/built-in.o`。然后，`vmlinux-deps`会汇总所有结果值。这解释了`vmlinux`最终如何依赖于所有`build-in.o`文件。

* 下一个问题是如何创建所有`built-in.o`对象？再一次，让我复制所有相关的行，并说明其工作原理。

  ```makefile
  $(sort $(vmlinux-deps)): $(vmlinux-dirs) ;

  vmlinux-dirs    := $(patsubst %/,%,$(filter %/, $(init-y) $(init-m) \
               $(core-y) $(core-m) $(drivers-y) $(drivers-m) \
               $(net-y) $(net-m) $(libs-y) $(libs-m) $(virt-y)))

  build := -f $(srctree)/scripts/Makefile.build obj               #Copied from `scripts/Kbuild.include`

  $(vmlinux-dirs): prepare scripts
      $(Q)$(MAKE) $(build)=$@

  ```

  第一行告诉我们`vmlinux-deps`依赖于`vmlinux-dirs`。接下来，我们可以看到`vmlinux-dirs`是一个变量，它包含所有直接的根子文件夹，末尾没有`/`字符。最重要的一行是构建 `$(vmlinux-dirs)` 目标的方法。 替换所有变量后，此配方如下所示（我们以`drivers`文件夹为例，但是将对所有根子文件夹执行此规则）

  ```
  make -f scripts/Makefile.build obj=drivers
  ```

  这行仅调用另一个makefile ([scripts/Makefile.build](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build)) 并传递 `obj`变量，该变量包含要编译的文件夹。

* 接下来的逻辑步骤是查看 [scripts /Makefile.build](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build)。执行之后发生的第一件事是，包括当前目录中定义的`Makefile`或`Kbuild`文件中的所有变量。当前目录是指`obj`变量引用的目录。 包含在[以下3行](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L43-L45)中。

  ```makefile
  kbuild-dir := $(if $(filter /%,$(src)),$(src),$(srctree)/$(src))
  kbuild-file := $(if $(wildcard $(kbuild-dir)/Kbuild),$(kbuild-dir)/Kbuild,$(kbuild-dir)/Makefile)
  include $(kbuild-file)
  ```
  嵌套的makefile主要负责初始化诸如`obj-y`之类的变量。快速提醒：`obj-y`变量应包含位于当前目录中的所有源代码文件的列表。嵌套的`makefile`初始化的另一个重要变量是`subdir-y`。此变量包含在构建`curent`目录中的源代码之前需要访问的所有子文件夹的列表。 `subdir-y`用于实现递归递减到子文件夹。

* 在不指定目标的情况下调用 `make` 时（例如在执行 `scripts /Makefile.build` 的情况下），它将使用第一个目标。 `scripts /Makefile.build` 的第一个目标称为 `__build`，可以在[此处](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L96) 找到让我们来看一下。
  
  ```makefile
  __build: $(if $(KBUILD_BUILTIN),$(builtin-target) $(lib-target) $(extra-y)) \
       $(if $(KBUILD_MODULES),$(obj-m) $(modorder-target)) \
       $(subdir-ym) $(always)
      @:
  ```

  如您所见，`__build` 目标没有收据，但取决于其他目标。我们只对`$（builtin-target）`感兴趣-它负责创建`built-in.o`文件，而`$ {subdir-ym）`- 负责下降到嵌套目录。

* 让我们看一下`subdir-ym`。此变量在[此处](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.lib#L48) 进行了初始化，并且只是`subdir-y`和`subdir-m`个变量。 （`subdir-m`变量类似于`subdir-y`，但它定义了子文件夹需要包含在单独的[kernel模块](https://en.wikipedia.org/wiki/Loadable_kernel_module)中。我们跳过了讨论模块，以保持专注。）

*  `subdir-ym` 目标是在[此处](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L572) 定义的，应该看起来很熟悉。

  ```
  $(subdir-ym):
      $(Q)$(MAKE) $(build)=$@
  ```

  这个目标只是触发嵌套子文件夹之一中的 `scripts/Makefile.build` 的执行。

* 现在是时候检查[内置目标](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L467) 目标了。我再次在这里只复制相关行。
  
  ```makefile
  cmd_make_builtin = rm -f $@; $(AR) rcSTP$(KBUILD_ARFLAGS)
  cmd_make_empty_builtin = rm -f $@; $(AR) rcSTP$(KBUILD_ARFLAGS)

  cmd_link_o_target = $(if $(strip $(obj-y)),\
                $(cmd_make_builtin) $@ $(filter $(obj-y), $^) \
                $(cmd_secanalysis),\
                $(cmd_make_empty_builtin) $@)

  $(builtin-target): $(obj-y) FORCE
      $(call if_changed,link_o_target)
  ```

  该目标取决于 `$（obj-y）`目标，而`obj-y`是需要在当前文件夹中构建的所有目标文件的列表。这些文件准备就绪后，将执行 `cmd_link_o_target` 命令。如果 `obj-y` 变量为空，则调用 `cmd_make_empty_builtin` ，直到创建一个空的 `Built-in.o`。否则，执行 `cmd_make_builtin` 命令；它使用我们熟悉的 `ar` 工具创建 `built-in.o` 精简档案。

* 最终我们到达需要编译一些东西的地步。您还记得我们最后一个未开发的依赖项是 `$（obj-y）` 和 `obj-y` 只是目标文件列表。 [此处](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L313) 定义了从相应的`.c`文件编译所有目标文件的目标。让我们检查理解该目标所需的所有行。

  ```makefile
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

  在`recipe`内部，此`target`称为 `rule_cc_o_c`。 这个规则负责很多事情， 像检查源代码中的一些常见错误(`cmd_checksrc`), 为导出的模块符号启用版本控制 (`cmd_modversions_c`), 使用 [objtool](https://github.com/torvalds/linux/tree/v4.14/tools/objtool) 验证生成的目标文件的某些方面，并构建对`mcount`函数的调用列表以便 [ftrace](https://github.com/torvalds/linux/blob/v4.14/Documentation/trace/ftrace.txt) 可以很快找到他们. 但是最重​​要的是，它调用`cmd_cc_o_c`命令，该命令实际上将所有`.c`文件编译为目标文件。

### 结论

哇，在内核构建系统内部进行漫长的旅程！尽管如此，我们还是跳过了许多细节，对于那些想了解更多有关该主题的人，我建议阅读以下[document](https://github.com/torvalds/linux/blob/v4.14/Documentation/kbuild/makefiles.txt)，然后继续阅读`Makefiles`源代码。现在让我强调重要的几点，您应该将这些要点理解为本章的内容。

1. `.c` 文件如何被编译为目标文件。
2. 如何将目标文件合并到 `build-in.o` 文件中。
3. 递归构建如何拾取所有子`build-in.o`文件并将它们组合成一个文件。
4. `vmlinux` 是如何与所有顶级`build-in.o`文件链接的。

我的主要目标是，阅读本章后，您将对上述所有点有一个大致的了解。

##### 上一页

1.2 [Kernel 初始化: Linux 项目结构](./project-structure.md)

##### 下一页

1.4 [Kernel 初始化: Linux 启动顺序](./kernel-startup.md)
