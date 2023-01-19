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
		
		sc %0xffFF%;		
		llb %0xffFF%;		
		lrx0 %/0xffFFffFF%;
		faristrx0;			
							//test 2- try to write 2 bytes
		ab;					
		faristla;			
							//test 3- use literal address
		farstrx0 %&0xffFFff%;
		lla %0xDE02%;syscall;
		sc %harden_whiletop%; jmpc;

		lb 0; div;


