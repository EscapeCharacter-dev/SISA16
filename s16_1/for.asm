#!/usr/bin/sisa16_asm -run

//This small program demonstrates a few essential features of Sisa16 and KRENEL
//Notably...
//1) usage of putchar and libc proc_wait
//2) the exec syscall
//3) Multitasking.


..include"libc.hasm"
.ZERO_STACK_POINTER:		astp;popa;
//..include"libc_pre.hasm"
//..(LIBC_REGION):
//..dinclude"libc_pre.bin"

.wait_time:				33

..main(3):
lrx0 %/krenel_boot%;
lrx0 %/top_of_5%;
proc_krenel;
halt;

..(4):
krenel_boot:
	//halt;
	la 's'; putchar;
	la '\r'; putchar;
	la '\n'; putchar; interrupt;
	lla %0xDE04%;//code for the exec region syscall.
	lb 5;		//What region to exec?
	syscall;	//Make the call
	halt;


..(5):
asm_begin_region_restriction;
top_of_5:


astp; popa;

sc %fun_start%; 
jmpc;

//Space for variables.
fun_i: // location of the i variable
bytes 0,0; 
fun_end_i: // location of the end
bytes 255,255; 




fun_start:
//initialize variables
la 0 
stla %fun_i%
lla %0xC000% 
stla %fun_end_i%


fun_loop:
la '\r'; putchar;
la '\n'; putchar;
la ' '; putchar;
la '\:'; putchar;

//la 10; interrupt;
[if]{llda %fun_i%; llb %0x100%;mod;lb 0; cmp;}\
	{la '\n'; interrupt;}


llda %fun_i%;
lldb %fun_end_i%;
cmp;
sc %fun_exit_loop%;jmpifeq;




llda %fun_i%;
lb 8;rsh;rx0a;
proc_printbytelchex;


llda %fun_i%;
rx0a;
proc_printbytelchex;


la '\t';putchar;putchar;


//i++
llda %fun_i%;
aincr;
stla %fun_i%;

//lla %1%; alpush; proc_wait;alpop;
sc %fun_loop%; jmpc;

fun_exit_loop:
la '\n'; interrupt;
lla %500%; alpush; proc_wait;alpop;

la '\r'; putchar;
la '\n'; putchar;
la 'I'; putchar;
la '='; putchar;
la '\n'; interrupt;

llda %fun_i%;
lb 8;rsh;rx0a;
proc_printbytelchex;
llda %fun_i%;
rx0a;
proc_printbytelchex;

la ';'; putchar;
la '\n'; interrupt;

halt;

asm_end_restriction;
