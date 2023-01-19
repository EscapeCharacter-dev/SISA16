#!/usr/bin/sisa16_asm -run
//create a whole-memory image.

.ZERO_STACK_POINTER:		astp;popa;
.POP_FARPTR_VARIABLE:	blpop;apop;ca;
.PUSH_FARPTR_VARIABLE:	ac;apush;blpush;



..include"libc.hasm"
//..include"libc_pre.hasm"
//..(LIBC_REGION):
//..dinclude"libc_pre.bin"

.BENCH_SECONDS:32768

//Variable Section.
section 0xAE0000;
.ld_iter:farldrx0 %&0xAE0000%;
.st_iter:farstrx0 %&0xAE0000%;
bytes %/0%;
.ld_secs:farllda %&0xAE0004%;
.st_secs:farstla %&0xAE0004%;
.st_secs_b:farstlb %&0xAE0004%;
bytes %0%;

section 0xAF0000;
..asciz:~~~~~Bench finished.~~~~~~~~
:seconds_strings:
bytes 0xd, 0xa;
..asciz://Seconds:
:iterations_strings:
..asciz: Iterations:
:STP_STRING:
..asciz: Stack Pointer:
:LENGTH_OF_STRING_STRING:
..asciz:	length of this string:

//main routine.

.myMacFunc:asm_arg0;asm_arg1
.myMacFunc:asm_arg0;asm_arg1

//asm_print;
//[myMacFunc]{asm_arg0;}{wj};bytes 3;asm_stop;
bytes '['; [myMacFunc]{[myMacFunc]{la 0}{bytes 1}}{[myMacFunc]{bytes 3}{bytes 4}};asm_print
//asm_print;[myMacFunc]{[myMacFunc]{bytes 0}{bytes 1}}{[myMacFunc]{bytes 2}{bytes 3}}

asm_print ASM_GEN_NAME
asm_print ASM_GEN_NAME
asm_print ASM_GEN_NAME
asm_print ASM_GEN_NAME
asm_print ASM_GEN_NAME
asm_print '1';
//rx2 holds our time in seconds.
..main(5):
	//containerize execution of the clock.
	lrx0 %/Lbl_clock_start%;
	proc_krenel;
	la '\n';putchar;
	la '\r';putchar;
	nop;|lrx0 %/0xaf0000%;
		proc_prints;
	halt;

..decl_farproc:legacy_printbytehex
	pop %3%;apop;push %4%;//grab our argument.
	rx0push; //code is expecting none of the RX registers to be clobbered.
	rx0a;
	proc_printbytehex;
	rx0pop;
	farret;

region 22;	
	:Lbl_clock_start:

	//halt;
	lla %0xDE0A%; lb 0; syscall; //Disable shutdownprint.
	la '\n';putchar; interrupt;
	la 0xc; interrupt;

nop//llda %[Lbl_clock_start]%; la 0xc;


	push %5%;
	clock;
	st_secs_b;
	rx3b;
	:main_looptop:
		//during iteration counting, use
		rxincr;
		clock;
		arx3;
		cmp;
		//jump to the top if the number of seconds elapsed is the same.
		jeq %main_looptop%;
	rx2_0;
	st_iter;
	//the time elapsed is different! it is still in b.
	st_secs_b
//print seconds string.
	lrx0 %0xAF%, %seconds_strings%
	proc_prints;
	ld_secs
	apush
	//testing 8 bit split syntax
	lb %~8%;
	rsh;
	apush;
	//print the number of seconds.
	legacy_printbytehex;
	apop;
	legacy_printbytehex;
	apop;
	//print the iterations string.
	lrx0 %0xAF%, %iterations_strings%
	proc_prints;
	//print the number of iterations.
	ld_iter;arx0;apush;
			lb 8; rx1b;rxrsh;arx0;apush;
	ld_iter;lb 16;rx1b;rxrsh;arx0;apush;
	ld_iter;lb 24;rx1b;rxrsh;arx0;apush;
	legacy_printbytehex;
		apop;
	legacy_printbytehex;
		apop;
	legacy_printbytehex;
		apop;
	legacy_printbytehex;
		apop;

	
//print the stack pointer.
	lrx0 %0xAF%, %STP_STRING%;
	proc_prints;
	astp;apush;legacy_printbytehex;apop;
	//print the length of string string.
	//print the length of that very same string.
	lrx0 %0xAF%, %LENGTH_OF_STRING_STRING%
	proc_prints;
	lrx0 %0xAF%, %LENGTH_OF_STRING_STRING%
	proc_strlen;
	rx0push;
	legacy_printbytehex;
	pop %4%;
		//Reset iteration counter.
	la 0
	rx0a
	st_iter
	//exit the loop if we have have passed the limit.
	sc %main_loopout%;
	ld_secs;rx3a;llb %BENCH_SECONDS%;cmp;lb 0;cmp;jmpifneq;
	la '\n'; interrupt;
	la 0xc; interrupt;
	sc %main_looptop%;jmpc;
:main_loopout:
lb 0;mod;
halt;

