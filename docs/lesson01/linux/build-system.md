## 1.3: Kernel build system.

After we examined Linux project structure, it worth spending some time investigating how we can build and run the kernel. Linux also uses `make` utility to build the kernel, though Linux makefile is much more complicated. Before we will take a look at the makefile, let's learn some important concepts about Linux build system, wich is called "kbuid".

### A few essential kbuild concepts

* Build process can be customized by using kbuild variables. Those variables are defined in `Kconfig` files. In such files you can define the variables themselves and their default values. Variables can have different types, including boolean and integer. In Kconfig file you can also define dependencies between variables (for example, you can say that if variable X is selected then variable Y will be selected implicitly). As an example, you can take a look at [arm64 Kconfig file](https://github.com/torvalds/linux/tree/v4.14/arch/arm64/Kconfig). This file defines all variables, specific for `arm64` architecture. `Kconfig` functionality is not part of the standard `make` and is implemented in the Linux makefile. Variables, defined in `Kconfig` files are exposed to the kernel source code as well as to nested makefiles. Variable values can be set during kernel configuration step (for example, if you type `make menuconfig` a console GUI will be shown. It allows you to customize values for all kernel variables and stores those values in `.config` file. Use `make help` command to view all possible options to configure the kernel)

* Linux uses recursive build. This means that each subfolder of the Linux kernel can define it's own `Makefile` and `Kconfig. Most of the nested Makefiles are very simple and just defines what object files need to be compiled. Usually, such definitions have the following format.

  ```
  obj-$(SOME_CONFIG_VARIABLE) += some_file.o
  ```

  This definition means that file `some_file.c` will be compiled and linked to the kernel only if `SOME_CONFIG_VARIABLE` is set. If you want to compile and link some file unconditionally, you need to change the previous definition to look like this.

  ```
  obj-y += some_file.o
  ```

  An example of the nested Makefile can be found at [/kernel/Makefile](https://github.com/torvalds/linux/tree/v4.14/kernel/Makefile)

* Before we move forward, you need to understand the basic make rule structure and be comfortable with make terminology. The rule structure is illustrated in the following diagram. 

  ```
  targets : prerequisites
          recipe
          …
  ```
    * `targets` are file names, separated by spaces. Targets are generated after the rule is executed. Usually, there is only one target per rule.
    * `prerequisites` are files that make monitors in order to see whether it needs to update the targets.
    * `recipe` is a bash script. Make calls it when some of the prerequisites have been updated. The recipe is responsible for generating the targets.
  For additional information about make rules, please refer to the [official documentation](https://www.gnu.org/software/make/manual/html_node/Rule-Syntax.html#Rule-Syntax)

* `make` is very good in detecting whether any of the prerequisites have been changed and updating only targets that need to be updated. However, if a recipe is dynamically generated, `make` is unable to detect this change. How can this happen? Very easily. One good example is when you change some configuration variable that results in appending an additional option to the recipe. By default, in this case, `make` will not recompile previously generated object files, because their prerequisites haven't been changed, only the recipe was updated. To overcome this behavior Linux introduces [if_changed](https://github.com/torvalds/linux/blob/v4.14/scripts/Kbuild.include#L264) function. To see how it works let's consider the following example.

  ```
  cmd_compile = gcc $(flags) -o $@ $<

  %.o: %.c FORCE
      $(call if_changed,compile)
  ```

  Here for each of the `.c` files we are building corresponding `.o` file by calling `if_changed` function with the argument `compile`. `if_changed` function then looks for `cmd_compile` variable (it adds `cmd_` prefix to the first argument) and checks whether this variable has been updated since the last execution or any of the prerequisites have been changed. If yes - `cmd_compile` command is executed and object file is regenerated. Our sample rule has 2 prerequisites: source `.c` file and `FORCE`. `FORCE` is a special prerequisite that forces the recipe to be called each time when `make` command is called, without it, the recipe would be called only if `.c` file is changed. You can read more about `FORCE` target [here](https://www.gnu.org/software/make/manual/html_node/Force-Targets.html) 

### Building the kernel

Now, when we learned some important concepts about the Linux build system, let's try to figure out what exactly is going on after you type `make` command. This process is very complicated and includes a lot of details, most of which we will skip. Our goal will be to answer 2 questions.

1. How exactly are source files compiled into object files?
1. How are object files linked into the OS image?

We are going to tackle the second question first.

#### Link stage 

* As you might see from the output of `make help` command, the default target, which is responsible for building the kernel, is called `vmlinux`

* `vmlinux` target definition can be found [here](https://github.com/torvalds/linux/blob/v4.14/Makefile#L1004) and it looks like this.

  ```
  cmd_link-vmlinux =                                                 \
      $(CONFIG_SHELL) $< $(LD) $(LDFLAGS) $(LDFLAGS_vmlinux) ;    \
      $(if $(ARCH_POSTLINK), $(MAKE) -f $(ARCH_POSTLINK) $@, true)

  vmlinux: scripts/link-vmlinux.sh vmlinux_prereq $(vmlinux-deps) FORCE
      +$(call if_changed,link-vmlinux)
  ```

  This target uses already familiar to us `if_changed` function. Whenever some of the prerequsities are updated `cmd_link-vmlinux` command is executed. This command executes [scripts/link-vmlinux.sh](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh) script(Note ussage of [$<](https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html) automatic variable). It also executes architecture spcefic [postlink script](https://github.com/torvalds/linux/blob/v4.14/Documentation/kbuild/makefiles.txt#L1229), but we are not very interested in it.

* Whenever [scripts/link-vmlinux.sh](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh) is executed it assumes that all required object files are already buit and there locations are stored in 3 variables: `KBUILD_VMLINUX_INIT`, `KBUILD_VMLINUX_MAIN`, `KBUILD_VMLINUX_LIBS`.  

* `link-vmlinux.sh` script first creates `thin archive` from all available object files. `thin archive` is a special object that contains references to all object files as well as a combined symbol table. This is done inside [archive_builtin](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L56) function. In order to create `thin archive` this function uses [ar](https://sourceware.org/binutils/docs/binutils/ar.html) utility. Generated `thin archive` is stored as `built-in.o` file and has the format that is understandable by the linker, so it can be used as any other normal object file.

* Next [modpost_link](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L69) is called. This function calls linker and generates `vmlinux.o` object file. We need this object file to perform [Section mismatch analysis](https://github.com/torvalds/linux/blob/v4.14/lib/Kconfig.debug#L308) This analysis is performed by [modpost](https://github.com/torvalds/linux/tree/v4.14/scripts/mod) program and is triggered at [this](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L260) line.

* Next kernel symbol table is generated. It contains information about all functions and global variables as well as their location in the `vmlinux` binary. The main work is done inside [kallsyms](https://github.com/torvalds/linux/blob/v4.14/scripts/link-vmlinux.sh#L146) function. This function first uses [nm](https://sourceware.org/binutils/docs/binutils/nm.html) to extract all symbols from `vmlinux` binary. Then it uses [scripts/kallsyms](https://github.com/torvalds/linux/blob/v4.14/scripts/kallsyms.c)  utility to generate a special assembler file containing all symbols in a special format, understandable by the Linux kernel. Next, this assembler file is compiled and linked together with the original binary.  This process is repeated several times because after final link addresses of some symbols can be changed.  Information from kernel symbol table is used to generate '/proc/kallsyms' file at runtime.

* Finally `vmlinux` binary is ready and `System.map`  is build. `System.map` contains the same information as `/proc/kallsyms` but this is static file and unlike `/proc/kallsyms` it is not generated at runtime. `System.map` is mostly used to resolve addresses to symbol names during [kernel oops](https://en.wikipedia.org/wiki/Linux_kernel_oops). The same `nm` utility is used to build `System.map`. This is done [here](https://github.com/torvalds/linux/blob/v4.14/scripts/mksysmap#L44)

#### Build stage 

* Now let's take one step backward and examine how source code files are compiled to object files. As you might remember one of the prerequisites of the `vmlinux` target is `$(vmlinux-deps)` variable. Let me now copy a few relevant lines from the main Linux Makefile to demonstrate how this variable is built. 

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

  It all starts with variables like `init-y`, `core-y`, etc., which combined contains all subfolders of the Linux kernel that contains buildable source code. Then `built-in.o` is appended to all the subfolders, so, for example, `drivers/` becomes `drivers/built-in.o`. `vmlinux-deps` then just aggregates all resulting values. This illustrates how `vmlinux` eventually becomes dependent on all `build-in.o` files.

* Next question is how all `built-in` objects are created? Once again, let me copy all relevant lines and explain how it all works.

  ```
  $(sort $(vmlinux-deps)): $(vmlinux-dirs) ;

  vmlinux-dirs    := $(patsubst %/,%,$(filter %/, $(init-y) $(init-m) \
               $(core-y) $(core-m) $(drivers-y) $(drivers-m) \
               $(net-y) $(net-m) $(libs-y) $(libs-m) $(virt-y)))

  build := -f $(srctree)/scripts/Makefile.build obj               #Copied from `scripts/Kbuild.include`

  $(vmlinux-dirs): prepare scripts
      $(Q)$(MAKE) $(build)=$@

  ```

  The first line tells us that `vmlinux-deps` depends on `vmlinux-dirs`. Next, we can see that `vmlinux-dirs` is a variable that contains all direct root subfolders without `/` character at the end. And the most important line here is the recipe to build `$(vmlinux-dirs)` target. After substitution of all variables, this recipe will look like the following (we use `drivers` folder as an example, but this rule will be executed for all root subfolders)

  ```
  make -f scripts/Makefile.build obj=drivers
  ```

  This line just calls another Makefile  ([scripts/Makefile.build](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build)) and passes `obj` variable, which contains a folder to be compiled. 

* Next logical step is to take a look at [scripts/Makefile.build](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build). The first important thing that happens after it is executed is that all variables from `Makefile` or `Kbuild` files, defined in the current directory are included. By current directory I mean the directory referenced by the `obj` variable. This is done in the [following 3 lines](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L43-L45)

  ```
  kbuild-dir := $(if $(filter /%,$(src)),$(src),$(srctree)/$(src))
  kbuild-file := $(if $(wildcard $(kbuild-dir)/Kbuild),$(kbuild-dir)/Kbuild,$(kbuild-dir)/Makefile)
  include $(kbuild-file)
  ```
  Nexted makefiles are mostly responsible for initializing variables like `obj-y`. As a quick reminder: `obj-y` variable should contain list of all source code files, locaated in the current directory. Another important variable that is initialized by the nested makefile is `subdir-y`. This variable contains list of all subfolders, that need to be visited before source code in the curent directory can be buit. `subdir-y` is used to implement recursive descending into subfolders.

* When `make` is executed without arguments (as it is in the case when `scripts/Makefile.build` is called) it uses the first target as default. We can easily figure out that default target for `scripts/Makefile.build` is called `__build` and it can be found [here](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L96) Let's take a look at it.

  ```
  __build: $(if $(KBUILD_BUILTIN),$(builtin-target) $(lib-target) $(extra-y)) \
       $(if $(KBUILD_MODULES),$(obj-m) $(modorder-target)) \
       $(subdir-ym) $(always)
      @:
  ```

  As you can see `__build` target doesn't have a receipt, but it depends on a bunch of other targets. We are only interested in `$(builtin-target)` - it is responsible for creating `built-in.o` file, and `$(subdir-ym)` - it is responsible for descending into nested directories and building their source code.

* Let's take a look at `subdir-ym`. This variable is initialized [here](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.lib#L48) and it is just a concatenation of `subdir-y` and `subdir-m` variables.  (`subdir-m` variable is similar to `subdir-y`, but it defines subfolders need to be included in a separate [module](https://en.wikipedia.org/wiki/Loadable_kernel_module). We skip the discussion of modules, for now, to keep focused.) 

*  `subdir-ym` target is defined [here](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L572) and should look very familiar to you.

  ```
  $(subdir-ym):
      $(Q)$(MAKE) $(build)=$@
  ```

  This target just triggers execution of `scripts/Makefile.build` in one of the nested subfolders.

* Now it is time to examine [builtin-target](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L467) target. Once again I am copying only lines, needed to understand this target.

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

  This target depends on `$(obj-y)` target and `obj-y` is a list of all object files, that need to be buit in the current folder. After those files are ready `cmd_link_o_target` command is executed. In case if `obj-y` variable is empty `cmd_make_empty_builtin` is called, wich just creates an empty `built-in.o`. Otherwise, `cmd_make_builtin` command is executed; it uses familiar to us `ar` tool to create `built-in.o` thin archive.

* Finally we got to the point where we are going to compile something. You remember that our last unexplored dependency is `$(obj-y)` and `obj-y` is just a list of object files. The target that compiles all object files from corresponding `.c` files is defined [here](https://github.com/torvalds/linux/blob/v4.14/scripts/Makefile.build#L313) Let's examine all lines, needed to understand this target.

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

  Inside it's recipe this target calls `rule_cc_o_c`. This rule is responsible for a lot of things, like checking source code for some common errors (`cmd_checksrc`), enabling versioning for exported module symbols (`cmd_modversions_c`), using [objtool](https://github.com/torvalds/linux/tree/v4.14/tools/objtool) to validate some aspects of generated object files and constructing a table of the locations of calls to 'mcount` function so that [ftrace](https://github.com/torvalds/linux/blob/v4.14/Documentation/trace/ftrace.txt) can find them quickly. But most importantly it calls `cmd_cc_o_c` command that actually compiles a `.c` file to an object file. 

### Conclusion

Wow, it was a long journey inside kernel build system internals! Still, we skipped a lot of details and, for those people who want to learn more about the subject, I can recommend to read the following [document](https://github.com/torvalds/linux/blob/v4.14/Documentation/kbuild/makefiles.txt) and continue reading Makefiles source code. Let me know summarize what we've just learned.

1. How `.c` files are compiled into object files.
1. How object files are combined into `build-in.o` files.
1. How  recursive build pick up all child `build-in.o` files and combines them into a single one.
1. How `vmlinux` is linked from all top-level `build-in.o` files.

This summary should be the take-home message of this chapter.

