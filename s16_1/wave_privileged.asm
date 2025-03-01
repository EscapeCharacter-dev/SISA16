#!/usr/bin/sisa16_asm -run

.ZERO_STACK_POINTER:		astp;popa;
.POP_FARPTR_VARIABLE:		blpop;apop;ca;
.PUSH_FARPTR_VARIABLE:		ac;apush;blpush;

//global variables.
.ld_iteration_count:	farllda %&0x30000%;
.st_iteration_count:	farstla %&0x30000%;

.ld_line_count:			farllda %&0x30002%;
.st_line_count:			farstla %&0x30002%;

.ld_page_count:	farllda %&0x30004%;
.st_page_count:	farstla %&0x30004%;


section 0x40000;
	:ascii_greyscale:
	..asciz: .:-=+*#%@%#*+=-:.
	.ascii_greyscale_len:18

.ITER_REGION:3
..(ITER_REGION):
	bytes 0,0,0,0;

..include"libc.hasm"


.line_length:			90
.line_length_plus_1:	91
.num_lines:				20
.wait_time:				16

..main:
asm_begin_region_restriction;
la line_length;
alpush;
asm_print @&
st_iteration_count;
st_page_count;
st_line_count;
nop;
nop;
lla %0xE000%; interrupt;
asciifun_looptop: //Comment.
	//increment the counter.
		alpop;
			aincr;
			llb %line_length_plus_1%; 
			mod;
		alpush;
		sc %iter_is_endval%;llb %line_length%;cmp;jmpifeq;
		sc %iter_is_not_endval%;jmpc;

	iter_is_endval:
		la '\n';putchar;
		la '\r';putchar;
		ld_iteration_count; 
			adecr;
		st_iteration_count;
		ld_line_count;aincr;st_line_count;
		lb num_lines;cmp;lb 2;cmp;sc %reached_num_lines%; jmpifeq;
	sc %asciifun_looptop%;jmpc;

	reached_num_lines:
		la '\n'
		interrupt;
		clear_terminal;
		la 0;
		st_line_count;
		ld_page_count;
		adecr;
		st_page_count;
		st_iteration_count;
		la %~wait_time%;
		alpush;
			proc_wait;
		alpop;
		sc %asciifun_looptop%;jmpc;
	
	iter_is_not_endval:
		ld_iteration_count;
		blpop;blpush;
		add;
		lb %~ascii_greyscale_len%;
		mod;
		//we now have the offset calculated in register a.
		sc %0x4%; 
		lb %~ascii_greyscale%;
		add;
		ba;
		farilda;
		putchar;
	sc %asciifun_looptop%;jmpc;
asciifun_loopout:
	la 7; putchar;
	halt;
	asm_end_restriction;
