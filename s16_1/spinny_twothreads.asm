#!/usr/bin/sisa16_asm -run

//This small program demonstrates a few essential features of Sisa16 and KRENEL
//Notably...
//1) usage of putchar and libc proc_wait
//2) the exec syscall
//3) Multitasking.

.ZERO_STACK_POINTER:		astp;popa;
//..(2):
..include"libc.hasm"

//..include"libc_pre.hasm"
//..(LIBC_REGION):

//..dinclude"libc_pre.bin"

.wait_time:				60

..main(3):
lrx0 %/krenel_boot%;
proc_krenel;
halt;

..(4):
krenel_boot:
	lla %0xDE04%;//code for the exec region syscall.
	lb 5;		//What region to exec?
	syscall;	//Make the call
	sc %bruh%;
	bruh:
	la 'q'; putchar;
	la '\ '; putchar;
	la 5;
	rx0a
	sleep_wait;
	

	lla %0xDE02%; syscall;
	
	sc %bruh%;jmpc;


..(5):
asm_begin_region_restriction;
sc %asciifun_start%; jmpc;
la '\r'; putchar; 
la '\n'; interrupt;
nop;
asciifun_start:
la 0;alpush;
	//our loop!
	asciifun_looptop: 
		la ' '; putchar;putchar;putchar;
		alpop; 
			aincr; 
			lb %~ascii_spinny_len%;mod;
		alpush;
		lb %~ascii_spinny%;
		add; ca; ilda;
		putchar
		putchar
		putchar
		putchar
		putchar
		putchar
		putchar
		putchar
		la '\r'; putchar;
		la '\n'; interrupt;
		lrx0 %/wait_time%
		sleep_wait
		jim %asciifun_looptop%
		:ascii_spinny:
		..asciz:-\|/
		.ascii_spinny_len:4
asm_end_restriction;
