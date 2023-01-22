# Notice: Development on hold

Development of the emulator is on hold until I receive affirming prayer that this is what I should be working on.
I will, however, note changes i'd like to make here:

```
1. Add 256 bytes to the end of every single process memory and change memory reads/writes to mask 0xffFFff only for the start address, 
	then do the multi-byte read/write. This will mean that there will be buggy behavior when trying to execute instructions
	whose arguments straddle the end of the 16 megabyte image, and there will be 'secretly' 3 extra accessible bytes in memory.
	I consider this fine.
2. (implementing 1) Implement multi-byte reads/writes in such a way that they will compile to a bswap instruction on x86_64. This should improve performance signifncantly, might even bring the emulator's speed within the same order as native C. This should be done with inline functions, not macros, to clean up the emulator's code and improve maintainability. Hundreds of instructions need to be changed to use the new functions.

This stack overflow post: https://stackoverflow.com/questions/36497605/how-to-make-gcc-generate-bswap-instruction-for-big-endian-store-without-builtins
shows how to do the implementation in a portable way that doesn't use __builtin_bswap.

```

________________________________________________
# Replace the CPU!

SISA16 is a lightweight high-performance minimal virtual machine designed for the replacement of the host CPU.

In this repository i have provided the assembler, emulator, disassembler, debugger, and an example kernel and standard library.

The assembler has the emulator built-in so that programs can be executed as scripts.

Platforms Tested and confirmed for 100% compliance:

```
	Windows 10 x86_64 (i7-6700) on MSYS2
	Windows 10 x86_64 (i7-6700) on WSL2
	Debian 11 x86_64 (i7-6700)
	Raspbian ArmV8 (BCM2837B0)
	Debian 11 IA32 (i7-6700)
	Alpine Linux IA32 (JSlinux)
	Buildroot Linux Riscv64 (JSLinux)
```

Platforms planned to be tested:

```
	Debian ppc32
	(any) ppc64
	(any) ppc64le
	(any) Mips64el
	(any) Mips64be
	(any) Mips64le
	Sega Naomi Hitachi Sh-4
```

## Why is SISA16 special?

* Minimal dependence on the host operating system. A very minimal C standard library will build the assembler
	and emulator just fine.

* CPU architecture independence- The behavior of the VM is host-independent. There are only two behaviors which may vary by architecture:
	* Floating point numbers. Their layout in memory may vary, although IEEE-754 compliance is nearly ubiquitous...

	* Signed integers. Non-twos-complement architectures are not supported, and the sign bit must be the highest bit.

	(Luckily, virtually every architecture around today guarantees both of these things.)

	* Execution time. Slower processors run code slower.

	The emulator and assembler are confirmed to work on literally dozens of architectures and operating systems.

* Unique. SISA16 is no ordinary virtual machine language. It has full preemption.

* Trivially embeddable. Implement five small and easy-to-understand functions in a single header file
	to add SISA16 scripting to any system.

* Ready for deployment. Assembler, Debugger, Disassembler, Kernel, manuals. Everything is here.

* Documented. Manpages and developer manual are provided in this repository along with examples.

* Public domain. 


Build Statuses:

C-Chads/Simple_ISA:

Ubuntu (x86_64):
![build cchads](https://github.com/C-Chads/Simple_ISA/actions/workflows/c-cpp.yml/badge.svg)

This repository contains the SISA16 standalone emulator source, 
the macro assembler/disassembler which doubles as an emulator,
and a functioning debugger.

Note that the debugger is not as well maintained as the rest of the project,

notably the debugger currently does not have the ability

to view the values in the "gp" (general purpose) registers.

# What does the Assembly language look like?

See the included programs in s16_1. they are named ".asm"

header libraries are postfixed with ".hasm"

precompiled libraries and binaries are postfixed ".bin"

The disassembly looks like this:

![disassembly output](disassembly.png)


# Notable Features and Limitations

* Two privilege rings 

* the registers a, b, c, and the program counter are 16 bit. the program counter region is 8 bit.

* RX0-3 are 32 bit.

* gp 0 - 64 are 32 bit.

* No MOV instruction, SISA16 is a load/store RISC.

* 16 bit segmented memory model

* Single-threaded

* Roughly similar performance in the virtual machine with unoptimized or optimized builds of the VM.

* Uses computed goto on supported compilers, define USE_COMPUTED_GOTO

# Why?

Code written for Sisa16 is not only infinitely portable and fairly fast, 
but can easily be translated into any programming language.

Read the documentation for more information.

sisa16_asm.1

Manual.odt

```
Written by
~~~DMHSW~~~
for the public domain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~Let all that you do be done with love~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

```



## How fast is it?
______________________________
READER'S NOTE: I was not super serious about making sure that nothing else was happening on the computer

while benchmarks were running.

Notably, I was editting this document in my terminal editor, and sometimes looking at chrome.

I did eventually decide though to turn off the music that was playing on youtube.
______________________________

I wrote a simple program called "rxincrmark" which calculates how long it takes in milliseconds
for a busy loop to make 2.1 billion iterations.

The repository for that code is [here](https://github.com/gek169/rxincrmark).

(It has been seperated out due to potential licensing issues in the other scripting languages.)

it's similar to the Linux kernel's idea of "Bogomips".

I wrote this program both in sisa16 assembly language, to run on top of krenel,
and on WSL2 on my main development machine.

Running native code with the optimizer on, on my skylake machine, 
I ran the test 6 times and got these numbers:

### Native C
_________________
```
	rxincrmark.c, gcc, -O3 -march=native, volatile variable:
	
	5712
	5795
	5861
	5660
	5640
	5645
```

the average number of milliseconds for the native code was 5718.8333333333

this will serve as a baseline.

_________________________________________________________
### Sisa16

Using a user-mode program written in SISA16 which runs on KRENEL,
mimicking the C code having a variable in memory (not in registers)
I got these results:

```
	rxincrmark.asm,
	sisa16 userland code, variable is in memory, not in registers
	
	47411
	47284
	47433
	47258
	48435
	47283
```

an average of 47517.333333 ms.

47517.3333 / 5718.8333 = 8.3089 

so it runs about 1/8.3 times the speed.

I also decided to run SISA16 without KRENEL to get a more direct comparison of the actual

interpreter's raw performance without the overhead of KRENEL, the operating system I wrote for SISA16.

```
	rxincrmark_privilieged.asm,
	assembled into a .dsk image,
	sisa16 privileged code

	47206
	47212
	47212
	47538
	47539
	47433
```

The results were nearly identical, which is surprising and seems to indicate

that KRENEL's context switches are very lightweight.

Running an Operating System in SISA16 seems to introduce very little overhead! wow!



For reference, here is how other emulators/interpreters hold up given the same test:

_________________________________________________________
### Lua 5.4 (granularity is seconds, getting milliseconds was too complicated):

```
	rxincrmark.lua
	
	106000
	103000
	 91000
	 93000
```

Rather than using the average score, I decided to give lua the best chance and use the best score.

91000 / 5718.8333 = 15.912365

so about 1/15.912 times the speed.

_________________________________________________________
### Python 3.9.2

Python is typically mocked and scorned for being incredibly slow, however,

I would guess that this comes from the excess use of overly complex data structures and garbage collecting.

As expected, it was the slowest thing I have tested thus far, and thus I only did three iterations.

```
	rxincrmark.py

	142055.93286
	141836.07348 (would round up to 9)
	144037.05126 (would round up to 7)
```

141836.07348 / 5718.8333 = 24.801575

so python 3 was about 1/24.8 times the speed.


________________________________________________________
### NodeJS

The first time I tried to run the test on nodejs, I got something very strange...
the result was actually around 3.6-3.7 seconds.

I was stunned, but then I thought about it, and realized it was probably

optimizing the local variable "i" that I was using for the loop down to a single register being incremented,

which I happen to know from previous (invalid) versions of the same benchmark for sisa16, 
takes about 11-14 seconds.

In order to do a "real" comparison, like the volatile variable in C or SISA16, I needed

to use a global variable.

```
	node rxincrmark.js

	45573
	45157
	45602
	44895
```

the best result was 44895,

44895 / 5718.8333 = 7.85

so about 1/7.85 the speed of the native C program.

Sisa16_emu does not use JIT, yet, even with KRENEL running, it is comparable to

a JIT javascript implementation.

A 'proper' implementation of SISA16 with JIT acceleration would be awesome...

__________________________________
### QuickJS

Fabrice Bellard made a javascript engine, and I'm a big fan of his work, so I decided to use the same

javascript program in his interpreter.

I was hopeful that his interpreter might be faster than lua, but sadly, it is not (in this specific instance)

These took a long time, so I only did 3.

```
	qjs rxincrmark.js

	127717
	125914
	127243	
```

The best time was 125914 milliseconds (125.9 seconds, over two minutes)

125914 / 5718.8333 = 22.017

so about 1/22.017 of the native C program.

To give his interpreter a better chance, i also tried running it with "i" as a local variable.

```
	qjs rxincrmark_local.js

	118546
	118923
```
It did run slightly faster, but, as you can see, it's not a huge improvement.

So I decided not to run a third or fourth test.

__________________________________

### cc65 targetting sim65

I had to modify my C program to compile under CC65, but I was able to.

the new program uses an unsigned long rather than unsigned int, which is needed

to get a 4 byte integer.

This took way too long going all the way to 0xFFffFFff so I decided

to reduce the number of iterations by 256, making it 

0xFFffFF

Due to the nature of sim65, I couldn't use a builtin clock, so I had to use the "time" command.

```
	cl65 rxincrmark_6502.c --target sim6502 -o rxincrmark_6502;
	sim65 rxincrmark_6502

	4482
	4330
	4361
	4349
	4215
	4231
```

To correct for the division by 256, I decided to multiply each of these by 256

```

	4482 = 1147392		|	19.1232 minutes.
	4330 = 1108480		|	18.4747 minutes. (ends in 6 repeating, rounded up to 7)
	4361 = 1116416		|	18.6069 minutes.
	4349 = 1113344		|	18.5557 minutes.
	4215 = 1079040		|	17.984  minutes.
	4231 = 1083136		|	18.0523 minutes. (ends in 6 repeating, rounded up to 3)

```

Clearly, 6502s are not very efficient at doing 32 bit integer incrementing.


The source code for all tests is provided in [this repo](https://github.com/gek169/rxincrmark)





