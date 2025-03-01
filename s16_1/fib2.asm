#!/usr/bin/sisa16_asm -run



//use a separately compiled libc.
..include"libc_pre.hasm"
..(LIBC_REGION):
..dinclude"libc_pre.bin"

//use the normal libc.
//..include"libc.hasm"

..(4):
..decl_farproc:proc_fib
	//if it is less than two...
	lb 2; rx1b;
	rxcmp;
	lb 0; cmp;
	sc %fib_return_one%; jmpifeq;
	//Sum fib(i-1) and fib(i-2)
	rx0push;
	rxdecr;
	proc_fib;
	rx1pop;
	rx0push;
	rx0_1;rxdecr;rxdecr;
	proc_fib;
	rx1pop;
	rxadd;
	farret;
	fib_return_one:
		lb 1; 
		rx0b;
		farret

..main(3):
	lrx0 %/0x770000%;
	proc_krenel;
	halt;
..(0x77):
	lrx0 %/0xc0000%
	proc_gets
	la '\r'; putchar;la '\n'; putchar;
	lrx0 %/0xc0000%
	proc_atoi_dec
	proc_fib
	lrx1 %/0xd0000%;
	proc_utoa_dec
	lrx0 %/0xd0000%;
	proc_prints;
	la '\r'; putchar;la '\n'; putchar;
	la '\n'; interrupt; 
	halt;

