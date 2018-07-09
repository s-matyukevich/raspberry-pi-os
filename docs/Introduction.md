## Raspberry Pi OS project introduction or how to efficiently learn operating system development?

A few years ago, I opened the source code of the Linux kernel for the first time. At that time, I considered myself more or less a skillful software developer: I knew a little bit of assembler and C programming, and had a high-level understanding of major operating system concepts, such as process scheduling and virtual memory management. However, my first attempt was a complete failure - I understood almost nothing.

For other software projects that I have to deal with, I have a simple approach that usually works very well: I find the entry point of the program and then start reading the source code, going as deep as necessary to understand all the details that I am interested in. This approach works well, but not for something as sophisticated as an operating system. It was not just that it took me more than a week just to find an entry point - the main problem was that I quickly found myself in a situation where I was looking at a few lines of code,  and I had no idea how to find any clues about what those lines were doing. This was especially true for the low-level assembler source code, but it worked no better for any other part of the system that I tried to investigate. 

I don't like the idea of dismissing a problem just because it looks complex from the beginning. Furthermore, I believe that there are no complex problems. Instead, there are a lot of problems we simply don't know how to address efficiently, so I started to look for an effective way to learn OS development in general and Linux in particular.

### Challenges in learning OS development

I know that there are tons of books and documentation written about Linux kernel development, but neither of them provides me with the learning experience that I want. Half of the material are so superficial that I already know it. With the other half I have a very similar problem that I have with exploring the kernel source code: as soon as a book goes deep enough, 90% of the details appear to be irrelevant to the core concepts, but related to some security, performance or legacy considerations as well as to millions of features that the Linux kernel supports. As a result, instead of learning core operating system concepts, you always end up digging into the implementation details of those features.

You may be wondering why I need to learn operating system development in the first place. For me, the main reason is that I was always interested in how things work under the hood. It is not just curiosity: the more difficult the task you are working on, frequently things begin to trace down to the operating system level. You just can't make fully informed technical decisions if you don't understand how everything works at a lower level. Another thing is that if you really like a technical challenge, working with OS development can be an exciting task for you.

The next question you may ask is, why Linux? Other operating systems would probably be easier to approach. The answer is that I want my knowledge to be, at least in some way, relevant to what I am currently doing and to something I expect to be doing in the future. Linux is perfect in this regard because nowadays everything from small IoT devices to large servers tend to run Linux.

When I said that most of the books about Linux kernel development didn't work well for me - I wasn't being quite honest. There was one book that explained some essential concepts using the actual source code that I was capable of fully understanding even though I am a novice in OS development. This book is "Linux Device Drivers", and it's no wonder that it is one of the most famous technical books about the Linux kernel. It starts by introducing source code of a simple driver that you can compile and play around with. Then it begins to introduce new driver related concepts one by one and explains how to modify the source code of the driver to use these new concepts. That is exactly what I refer to as a "good learning experience". The only problem with this book is that it focuses explicitly on driver development and says very little about core kernel implementation details.

But why has nobody created a similar book for kernel developers? I think this is because if you use the current Linux kernel source code as a base for your book, then it's just not possible. There is no function, structure, or module that can be used as a simple starting point because there is nothing simple about the Linux source. You also can't introduce new concepts one at a time because in the source code, everything is very closely related to one another. After I realized this, an idea came to me: if the Linux kernel is too vast and too complicated to be used as a starting point for learning OS development, why don't I implement my own OS that will be explicitly designed for learning purposes? In this way, I can make the OS simple enough to provide a good learning experience. Also, if this OS will be implemented mostly by copying and simplifying different parts of the Linux kernel source, it would be straightforward to use it as a starting point to learn the Linux kernel as well. In addition to the OS, I decided to write a series of lectures that teaches major OS development concepts and fully explains the OS source code.

### OS requirements

I started working on the project, which later became the [RPi OS](https://github.com/s-matyukevich/raspberry-pi-os). The first thing I had to do was to determine what parts of kernel development I considered to be "basic", and what components I considered to be not so essential and can be skipped (at least in the beginning). In my understanding, each operating system has 2 fundamental goals:

1. Run user processes in isolation.
1. Provide each user process with a unified view of the machine hardware.

To satisfy the first requirement, the RPi OS needs to have its own scheduler. If I want to implement a scheduler, I also have to handle timer interrupts. The second requirement implies that the OS should support some drivers and provide system calls to expose them to user applications. Since this is for beginners, I don't want to work with complicated hardware, so the only drivers I care about are drivers that can write something to screen and read user input from a keyboard. Also, the OS needs to be able to load and execute user programs, so naturally it needs to support some sort of file system and be capable of understanding some sort of executable file format. It would be nice if the OS can support basic networking, but I don't want to focus on that in a text for beginners. So those are basically the things that I can identify as "core concepts of any operating system".

Now let's take a look at the things that I want to ignore:
1. **Performance** I don't want to use any sophisticated algorithms in the OS. I am also going to disable all caches and other performance optimization techniques for simplicity.
1. **Security** It is true that the RPi OS has at least one security feature: virtual memory. Everything else can be safely ignored.
1. **Multiprocessing and synchronization** I am quite happy with my OS being executed on a single processor core. In particular, this allows me to get rid of a vast source of complexity - synchronization. 
1. **Support for multiple architectures and different types of devices** More on this in the next section.
1. **Millions of other features that any production-ready operating system supports**

### How Raspberry Pi comes into play

I already mentioned that I don't want the RPi OS to support multiple computer architectures or a lot of different devices. I felt even stronger about this after I dug into the Linux kernel driver model. It appears that even devices with similar purposes can largely vary in implementation details. This makes it very difficult to come up with simple abstractions around different driver types, and to reuse driver source code. To me, this seems like one of the primary sources of complexity in the Linux kernel, and I definitely want to avoid it in the RPi OS. Ok, but what kind of computer should I use then? I clearly don't want to test my bare metal programs using my working laptop, because I'm honestly not sure that it is going to survive. More importantly, I don't want people to buy an expensive laptop just to follow my OS development exercises (I don't think anybody would do this anyway). Emulators look like more or less a good choice, but I want to work with a real device because it gives me the feeling that I am doing something real rather than playing with bare metal programming.

I ended up using the Raspberry Pi, in particular, the [Raspberry Pi 3 Model B](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/). Using this device looks like the ideal choice for a number of reasons:

1. It costs something around $35. I think that should be an affordable price.
1. This device is specially designed for learning. Its inner architecture is as simple as possible, and that perfectly suits my needs.
1. This device uses ARM v8 architecture. This is a simple RISC architecture, is very well adapted to OS authors' needs, and doesn't have so many legacy requirements as, for example, the popular x86 architecture. If you don't believe me, you can compare the amount of source code in the `/arch/arm64` and `/arch/x86` folders in the Linux kernel.

The OS is not compatible with the older versions of the Raspberry Pi, because neither of them support 64 bit ARM v8 architecture, though I think that support for all future devices should be trivial.

### Working with community

One major drawback of any technical book is that very soon after release each book becomes obsolete. Technology nowadays is evolving so fast that it is almost impossible for book writers to keep up with it. That's why I like the idea of an "open source book" - a book that is freely available on the internet and encourages its readers to participate in content creation and validation. If the book content is available on Github, it is very easy for any reader to fix and develop new code samples, update the book content, and participate in writing new chapters. I understand that right now the project is not perfect, and at the time of writing it is even not finished. But I still want to publish it now, because I hope that with the help of the community I will be able to not only complete the project faster but also to make it much better and much more useful than it was in the beginning. 

##### Previous Page

[Main Page](https://github.com/s-matyukevich/raspberry-pi-os#learning-operating-system-development-using-linux-kernel-and-raspberry-pi)

##### Next Page

[Contributing to the Raspberry PI OS](../docs/Contributions.md)
