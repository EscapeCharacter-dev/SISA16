#!/usr/bin/sisa16_asm -run

//This small program demonstrates a few essential features of Sisa16 and KRENEL
//Notably...
//1) usage of putchar and libc proc_wait
//2) the exec syscall
//3) Multitasking.

.ZERO_STACK_POINTER:		astp;popa;
..include"libc.hasm"

//..include"libc_pre.hasm"
//..(LIBC_REGION):
//..dinclude"libc_pre.bin"

.wait_time:				60

..main(3):
lrx0 %/krenel_boot%;
//proc_krenel;
fcall %&LIBC_KRENEL_BEGIN_REAL%;
halt;

..(4):
krenel_boot:
	lla %0xDE04%;//code for the exec region syscall.
	lb 5;		//What region to exec?
	syscall;	//Make the call
	sc %0%;
	halt;


..(5):
asm_begin_region_restriction;
sc %asciifun_start%; jmpc;
la '\r'; putchar; 
la '\n'; interrupt;
asciifun_start:
lla %0xDE0A%; lb 1; syscall; //Enable krenel shutdown syscall.
la 0;alpush;
	//our loop!
	asciifun_looptop: 
		//la ' '; putchar;putchar;putchar;
		alpop; 
			aincr; 
			lb %~ascii_spinny_len%;mod;
		alpush;
		lb %~ascii_spinny%;
		add; ca; ilda;
		ca
$|
			[for]{
				lrx0 %/0%;lrx1 %/50%;
			}{rxcmp;lt;}{
				ac
				putchar;
				rxincr;
			}
$|
		ac
		la '\r'; putchar;
		la '\n'; interrupt;
		la %~wait_time%;
		rx0a;sleep_wait;

		//alpush; proc_wait; alpop;
		jim %asciifun_looptop%;
		:ascii_spinny:
		..asciz:-\|/
		.ascii_spinny_len:4
asm_end_restriction;
