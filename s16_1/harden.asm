#!/usr/bin/sisa16_asm -run

//program to test security of memory segmentation on SISA16.

//..include"libc_pre.hasm"
//..(LIBC_REGION):
//..dinclude"libc_pre.bin"

..include"libc.hasm"

.wait_time:				60

..main(3):
lrx0 %/krenel_boot%;
proc_krenel;
halt;




//Krenel boot
..(7):
krenel_boot:
	lla %0xDE04%;//code for the exec region syscall.
	lb 5;		//What region to exec?
	syscall;	//Make the call
	halt;




..(4):
	//first, set up our stack.
	//get some zeroes in there.
	astp; popa;
	la 0; rx0a; rx0push;rx0push;rx0push;
	astp;popa;
		//Make the call
listener_loop:
	//la 3; apush;
	lrx0 %/0%;			
	ildrx0_0;			
						//compare with 0
	//la 0; rx1a; rxmod;
						
	lrx1 %/0%;			
	rxcmp;				
	sc %failure_print%; jmpifneq;
	lla %0xDE02%;syscall;
	sc %listener_loop%; jmpc;

failure_print:
	lrx0 %/fail_msg%;
	proc_prints;
	la 10; interrupt;
	la 0; rx1a; rxmod;

:fail_msg:
..asciz: FAILURE I was able to read something other than zero!
bytes 0


//Region 5


..(5):
harden_main:
	la 'q'; putchar;
	la 10; interrupt;
	harden_whiletop:		

		lla %0xDE04%;//code for the exec region syscall.
		lb 4;		//What region to exec?
		syscall;	//Make the call

		//first, set up our stack.
		//get some zeroes in there.
		astp; popa;
		la 0; rx0a; rx0push;rx0push;rx0push;
		astp;popa;

		//Do a far write.
		sc %0xffFF%;		
		lla %0xffFF%;		
		lrx0 %/0xAABBCCDD%;
		faristrx0;			

		//this should have written 4 FF bytes, one to the last byte of memory,
		//and three at the start of memory.

		//test if they are where they should be.


		farlda %&0xffFFff%
		lb 0xAA; cmp;
		jne %harden_fail%

		farlda %&0%
		lb 0xBB
		cmp
		jne %harden_fail%

		farlda %&1%
		lb 0xCC; cmp;
		jne %harden_fail%

		farlda %&2%
		lb 0xDD; cmp;
		jne %harden_fail%

		//store rx3.

		lrx3 %/0%
		farstrx3 %&0xffFFffFF%;

		//Test for zero.


		farlda %&0xffFFff%
		lb 0; cmp;
		jne %harden_fail%
		
		farlda %&0%;
		lb 0; cmp;
		jne %harden_fail%

		farlda %&1%
		lb 0; cmp;
		jne %harden_fail%

		farlda %&2%
		lb 0; cmp;
		jne %harden_fail%

		//test 2- write 2 bytes
		//test 2- try to write 2 bytes
		lla %0xCADF%;
		sc %0xffFF%
		llb %0xffFF%					
		faristla;

		farlda %&0xffFFff%
		lb 0xCA; cmp;
		jne %harden_fail%
		
		farlda %&0%;
		lb 0xDF; cmp;
		jne %harden_fail%

		lrx0 %/0xBDCDEDFD%
							//test 3- use literal address
		farstrx0 %&0xffFFffFF%;

		farlda %&0xffFFff%
		lb 0xBD; cmp;
		jne %harden_fail%
		
		farlda %&0%;
		lb 0xCD; cmp;
		jne %harden_fail%

		farlda %&1%
		lb 0xED; cmp;
		jne %harden_fail%

		farlda %&2%
		lb 0xFD; cmp;
		jne %harden_fail%
		
		lla %0xDE02%;syscall;
		sc %harden_whiletop%; jmpc;
	harden_fail:
		lb 0; div;


