## Lesson 1: Kernel build system.

After you take a look into project structure it worth spending some time invetigating how we can build and run the kernel. Linux also uses `make` utility to build the kernel, though `Makefile` the is uses is much more complicated. Before we will take a look on the Makefile, let's learn some important concepts about linux build system.

### Some important kbuild concepts

* Build process can be customized by using variables. Those variables are defined in `Kconfig` files. In such files you can define what variables can be used, default values for those variables. Variables can have different types, including boolean and integer. In Kconfig file you can also define dependencies between variales (for example, you can say that is variable X is selected then variable Y will be selected implicily). As an example you can take a look on [arm64 Kconfig file](https://github.com/torvalds/linux/tree/v4.14/arch/arm64/Kconfig). This file defines all variables, specific for `arm64` architecture. `Kconfig` functionality is not part of standard `make` and defined in the linux `Makefile`. Variables, defined in `Kconfig` files are availabe in kernel source code as well as in nested makefiles. Values for all configuration variables can be provided during kernel configuration step (for example, if you type `make menuconfig` a console GUI will be shown that allows you to customize values for all kernel variables and stores those values in `.config` file. Use `make help` command to view all posible options to configure the kernel.)

* Linux uses recursive build. This mean that each subfolder of the linux kernel can define it's own `Makefile` and `Kconfig. Most of the nested Makefiles are very simple and just defines what object files need to be compiled. Usually such definitionns have the following format.

```
obj-$(SOME_CONFIG_VARIABLE) += some_file.o
```

This definition means that file `some_file.c` will be compiled and linked to the kernel only if `SOME_CONFIG_VARIABLE` is set. If you want to compile and link some file unconditionaly you need to change previous definition to look like this.

```
obj-y += some_file.o
```

An example of the nested Makefile can be found at [/kernel/Makefile](https://github.com/torvalds/linux/tree/v4.14/kernel/Makefile)

* `make` is very good in detecting whether any of prerequisitie files have been changed and updating only targets that need to be updated. However if recipe that builds a target is dynamicalyy generated and has been updated as weel `make` is unable to detect this change. How this can happen? Very easily. One good example is when you change some configuration parameter that results in adding aditional parameter to the compiler. By default, in this case `make` will not recompile previously generated object files, because prerequsities for those files were not changed, only the recipe was updated. In order to overcome this behaviour linux introduces the function [if_changed](https://github.com/torvalds/linux/blob/v4.14/scripts/Kbuild.include#L264) To see how it works let's consider the following example.

```
cmd_compile = gcc $(flags) -o $@ $<

%.o: %.c FORCE
	$(call if_changed,compile)
```

Here for each of the `.c` file we are building coresponding `.o` file by calling `if_changed` function with the argument `compile`. `if_changed` function then looks for `cmd_compile` variable (it adds `cmd_` prefix to the provided argument) and checks whether this variable has been updated since the last execution or any of the prerequsities have been updated. If yes - `cmd_compile` command is executed and object file is regenerated. When `if_changed` function is called it can see that the rule have 2 prerequsities: source `.c` file and `FORCE`. `FORCE` is a special prerequsitie that forces recipe for this rule to be executed each time when `make` command is executed, without it the recipe would be called only if `.c` file was changed. You can read more about `FORCE` target [here](https://www.gnu.org/software/make/manual/html_node/Force-Targets.html) 

### Building the kernel

Now, when we learned a few important concepts about linux build system, let's try to figure out what exactly is going on after you type `make` command.

#### Link stage 

* As you might see from the output of `make help` command default target, that is responsible for building the kernel, is called `vmlinux`

* `vmlinux` target definition can be found [here](https://github.com/torvalds/linux/blob/v4.14/Makefile#L1004)

```
cmd_link-vmlinux =                                                 \
	$(CONFIG_SHELL) $< $(LD) $(LDFLAGS) $(LDFLAGS_vmlinux) ;    \
	$(if $(ARCH_POSTLINK), $(MAKE) -f $(ARCH_POSTLINK) $@, true)

vmlinux: scripts/link-vmlinux.sh vmlinux_prereq $(vmlinux-deps) FORCE
	+$(call if_changed,link-vmlinux)
```

This target uses already familiar `if_changed` function. Whenever some of the prerequsities are updated `cmd_link-vmlinux` command is executed. This command executes [scripts/link-vmlinux.sh](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh) (Note ussage of [$<](https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html) automatic variable). It also executes architecture spcefic [postlink script](https://github.com/torvalds/linux/blob/v4.14/Documentation/kbuild/makefiles.txt#L1229)

* Whenever [scripts/link-vmlinux.sh](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh) is executed it assumes that all required object files are already buit and there locations are stored in 3 variables: `KBUILD_VMLINUX_INIT`, `KBUILD_VMLINUX_MAIN`, `KBUILD_VMLINUX_LIBS`.  

  1.`link-vmlinux.sh` script first creates `thin archive` from all available object files. `thin archive` is a special object that contains references to all object files as well as combined symbol table. This is done inside [archive_builtin](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L56) function. In order to create `thin archive` this function uses [ar](https://sourceware.org/binutils/docs/binutils/ar.html) utility. Generated `thin archive` is stored in `built-in.o` file and has format that is undestandable by linker, so it can be used as any other normal object file.

  2. Next [modpost_link](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L69) is called. This function calls linker and generates `vmlinux.o` object file. We need this object file to perform [Section mismatch analysis](https://github.com/torvalds/linux/blob/v4.14/lib/Kconfig.debug#L308) This analysis is performed by [modpost](https://github.com/torvalds/linux/tree/v4.14/scripts/mod) program and is triggered at [this](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L260) line.

  3. Next kernel symbol table is generated. It contains information about all functions and global variables as well as their location in the `vmlinux` binary. The main work is done inside [kallsyms](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L146) function. This function first uses [nm](https://sourceware.org/binutils/docs/binutils/nm.html)to extract all symbols from `vmlinux` binary. Then it uses [scripts/kallsyms](https://github.com/torvalds/linux/blob/v4.14/scripts/kallsyms.c)  utility to generate a special assembler file containing all symbols in a cpecial format, understandable by linux kernel. Next this assembler file is compiled and linked together with the original binary.  This process is repeted severla times, because after final link addresses of some symbols can be changed.  Information from  kernel symobol table is used to generate '/proc/kallsyms' file at runtime.

  4. Finally `vmlinux` binary is ready and `System.map`  is build. `System.map` contains the same information as `/proc/kallsyms` but this is static file and unlike `/proc/kallsyms` it is not generated at runtime. `System.map` is mostly used to resolve addresses to symbol names during [kernel oops](https://en.wikipedia.org/wiki/Linux_kernel_oops). The same `nm` utility is used to build `System.map`. This is done [here](https://github.com/torvalds/linux/blob/v4.14/scripts/mksysmap#L44)

#### Build stage 

* Now let's take one step backward and examine how source code files are compiled to object files. As you might remember one of the prerequsities of `vmlinux` target is `$(vmlinux-deps)` variable. Let me now copy a few relevant lines from the main linux Makefile to demonstrate how this variable is built. 

```
init-y		:= init/
drivers-y	:= drivers/ sound/ firmware/
net-y		:= net/
libs-y		:= lib/
core-y		:= usr/

core-y		+= kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/

init-y		:= $(patsubst %/, %/built-in.o, $(init-y))
core-y		:= $(patsubst %/, %/built-in.o, $(core-y))
drivers-y	:= $(patsubst %/, %/built-in.o, $(drivers-y))
net-y		:= $(patsubst %/, %/built-in.o, $(net-y))
...

export KBUILD_VMLINUX_INIT := $(head-y) $(init-y)
export KBUILD_VMLINUX_MAIN := $(core-y) $(libs-y2) $(drivers-y) $(net-y) $(virt-y)
export KBUILD_VMLINUX_LIBS := $(libs-y1)
export KBUILD_LDS          := arch/$(SRCARCH)/kernel/vmlinux.lds

vmlinux-deps := $(KBUILD_LDS) $(KBUILD_VMLINUX_INIT) $(KBUILD_VMLINUX_MAIN) $(KBUILD_VMLINUX_LIBS)
```

It all starts with variables like `init-y`, `core-y`, etc. that combined contains all subfolders of the linux kernel that contains buildable source code. Then to each of the folder `built-in.o` is appendes, so `drivers/` becomes `drivers/built-in.o` for example. `vmlinux-deps` then just collects all those variables together.

* Next question is how all `built-in` objects are created? One again, let me copy all relevant lines and explain how it all works.

```
$(sort $(vmlinux-deps)): $(vmlinux-dirs) ;

vmlinux-dirs	:= $(patsubst %/,%,$(filter %/, $(init-y) $(init-m) \
		     $(core-y) $(core-m) $(drivers-y) $(drivers-m) \
		     $(net-y) $(net-m) $(libs-y) $(libs-m) $(virt-y)))

build := -f $(srctree)/scripts/Makefile.build obj               #Copied from `scripts/Kbuild.include`

$(vmlinux-dirs): prepare scripts
	$(Q)$(MAKE) $(build)=$@

```

The first line tells us that `vmlinux-deps` depends on `vmlinux-dirs`. Next we can see that `vmlinux-dirs` is a variable that contains all root direct subfolders without `/` character at the end of each folder name. And the most important line here is the recipe to build `$(vmlinux-dirs)` target. After substitution of all variables this recipe will look like the following (we use `drivers` folder as an example, but this rule will be executed for all folders)

```
make -f scripts/Makefile.build obj=drivers
```

This line just calls another Makefile  ([scripts/Makefile.build](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build)) and passes `obj` variable to it.

* Next logical step is to take a look at [scripts/Makefile.build](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build). The first importand thing that happends after it is executed is that  all variables from `Makefile` or `Kbuild` files, difind in the current directory are included. By current directoy I mean the directory referenced by the `obj` variable. This is done in the [following 3 lines](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L43-L45)

```
kbuild-dir := $(if $(filter /%,$(src)),$(src),$(srctree)/$(src))
kbuild-file := $(if $(wildcard $(kbuild-dir)/Kbuild),$(kbuild-dir)/Kbuild,$(kbuild-dir)/Makefile)
include $(kbuild-file)
```
Nexted makefiles are mostly responsible for initializing variables like `obj-y` that we discussed previously. As a quick reminder: `obj-y` variable should contain list of all source code files, locaated in the current directory. Another important variable that is initialized by the nested makefile is `subdir-y`. This variable contains list of all subfolders, that need to be visited before source code in the curent directory can be buit. `subdir-y` is used to implement recursive descending into subfolders.

* When `make` is executed without arguments (as it is in case of `scripts/Makefile.build`) it uses the first agument as default. We can easily figure out that default target for `scripts/Makefile.build` is called `__build` and it can be found [here](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L96) Let's take a look on it.

```
__build: $(if $(KBUILD_BUILTIN),$(builtin-target) $(lib-target) $(extra-y)) \
	 $(if $(KBUILD_MODULES),$(obj-m) $(modorder-target)) \
	 $(subdir-ym) $(always)
	@:
```

As you can see `__build` target don't have a recipt, but it depends on a bunch of other targets. We are only interested in `$(builtin-target)` - it is responsible for creating `built-in.o` file, and `$(subdir-ym)` - it is responsible for decending into nested directories and building there source code.

* Let's take a look on `subdir-ym`. This variable is initialized [here](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.lib#L48) and it is just a concatenation of `subdir-y` and `subdir-m` variables.  (`subdir-m` variable is similar to `subdir-y`, but it defines subfolders need to be included in a separate [modules](https://en.wikipedia.org/wiki/Loadable_kernel_module). We skip the discussion of modules for now to keep focused.) 

*  `subdir-ym` target is defined [here](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L572) and should look very familiar to you.

```
$(subdir-ym):
	$(Q)$(MAKE) $(build)=$@
```

This target just trigeres execution of `scripts/Makefile.build` in one of the nested subfolders.

* Now it is time to examine [builtin-target](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L467) target. One again I am copying only lines, need to understand this target.

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

This target depends on `$(obj-y)` target and `obj-y` is a list of all object files, that need to be buit in the current folder. After those files are ready `cmd_link_o_target` command is executed. In case if `obj-y` variable is emty is just creates empty `built-in.o` using `cmd_make_empty_builtin` command. Otherwise  `cmd_make_builtin` command is executed that uses familiar to us `ar` tool to create `built-in.o` thin archive.

* Finally we got to the point where we are going to compile something. You remember that our last unexplored dependency is `$(obj-y)` and `obj-y` is just a list of object files. The target that compiles all object files from coresponding `.c` files is defined [here](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L313) Let's examine all lines, need to understand this target.

```
cmd_cc_o_c = $(CC) $(c_flags) -c -o $@ $<

define rule_cc_o_c
	$(call echo-cmd,checksrc) $(cmd_checksrc)			  \
	$(call cmd_and_fixdep,cc_o_c)					  \
	$(cmd_modversions_c)						  \
	$(call echo-cmd,objtool) $(cmd_objtool)				  \
	$(call echo-cmd,record_mcount) $(cmd_record_mcount)
endef

$(obj)/%.o: $(src)/%.c $(recordmcount_source) $(objtool_dep) FORCE
	$(call cmd,force_checksrc)
	$(call if_changed_rule,cc_o_c)
```

Inside it's recipe this target calls `rule_cc_o_c`. This rule is responsible for a lot of things, like checking source code for some common errors (`cmd_checksrc`), enable versioning for exported module symbols (`cmd_modversions_c`), uses [objtool](https://github.com/torvalds/linux/tree/v4.14/tools/objtool) to validate some aspects of generated object files and construct a table of the locations of calls to 'mcount` function so that [ftrace](https://github.com/torvalds/linux/blob/v4.14/Documentation/trace/ftrace.txt) can find them quickly. But most importantly it calls `cmd_cc_o_c` command that actually compiles `.c` file to an object file. 

Wow, it was a long journy inside kernel build system internals! We still skipped a lot of details and for those people who want to learn more about the subject I can recommed to read the following [document](https://github.com/torvalds/linux/blob/v4.14/Documentation/kbuild/makefiles.txt) and yes, continue reading Makefiles source code. But what I want you to grasp from this lesson are major concepts and architectural deceigions that were used to design kernel build system.



