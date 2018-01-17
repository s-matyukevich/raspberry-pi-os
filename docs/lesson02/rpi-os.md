## Lesson 2: Processor initialization

In this lesson we are going to familiarizze yourself with some important concepts, relevant to ARM processors. Understanding of those concept is important in case you want to fully utilize ARM proessor capabilities. So now let's take a breef lool on the most important of them.

### Exception levels

Each ARM processor that suports ARM.v8 architecture has 4 exception levels. You can think about exception level as a processor execution mode in withc some operations are ether enabled  or disabled. The leastpriviliged exception level is level 0. When processor operates at this level it mostly uses only general purpose registers (X0 - X30) and stack pointer register (SP). Exception level 0 also allows us to use STR and LDR instructions to load and store data to and from memory. 

An operating system should deal with exception levels, because it needs to implement process isolation. Each user process should not be able to access any data that belongs to different user process. In order to achive such behaviour, an operating system always runs each user process at exception level 0. Operating at this exception level a process can only use it's own virtual memory and can't access any instruction that changes virtual memory settings. So in order to ensure process isolation, an OS just need to prepare separate virtual memory mapping for each process and put processor into exception level 0 before executing any user process.

Operating system itself usually operates at exception level 1. While running at this exception level processor gets access to the registers that allows to configure virtual memory settings as well as to some system registers. Raspberry Pi OS also will be using exception level 1.

We are not going to use exceptions levels 2 and 3 a lot, but I just wanted to brefely describe them so you can get an idea why they are needed. 

Exception level 2 is needed in a scenario when we are using a hypervisor. In this case host operating system runs on exception level 2 and guest operating systems can only use EL 1. This allows host OS to isolate guest OSes from each other in a similar way how OS isolates user proceses.

Exception level 3 is used for transitions from ARM "Secure World" to "Unsecure world"
