#!/usr/bin/sisa16_asm -run



//use a separately compiled libc.
//..include"libc_pre.hasm"
//..(LIBC_REGION):
//..dinclude"libc_pre.bin"

//use the normal libc.
..include"libc.hasm"

..(4):
..decl_farproc:proc_fib
	//if it is less than three...
	lb 3; rx1b;
	rxcmp;lt;
	sc %fib_return_one%; jmpifeq;
	//i is currently stored in rx0, it will be pushed 
	//fcall %&does_not_exist%
	
	
$|
	[for]//main loop
	{
		//we store our count value here...
		rx0push;  
		//these are the two values we add continually.
		lrx3 %/1%; 
		lrx2 %/1%; 
	}{
	 	rx0pop;
	 	rx0push;
	 	lrx1 %/2%;
	 	rxcmp;
	 	gt;
	}{
		rx0_2;
		rx1_3;
		rxadd;
		rx3_2;
		rx2_0;
		rx0pop;
		rxdecr;
		rx0push;
	}
$|
	rx0pop;
		rx0_2;farret;
	
	fib_return_one: :test: :test2:
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
	la '\r'; putchar;la '\n'; putchar; interrupt;
	la '\r'; putchar;la '\n'; putchar;
	lrx0 %/0xc0000%
	proc_atoi_dec
	rx0push
		proc_fib
		lrx1 %/0xd0000%;
		proc_utoa_dec
		lrx0 %/0xd0000%;
		proc_prints;
		la '\r'; putchar;la '\n'; putchar; interrupt;
	rx0pop;rx0push;
		lrx1 %/0xd0000%;
		proc_utoa_dec
		lrx0 %/0xd0000%;
		proc_prints;
		la '\r'; putchar;la '\n'; putchar; interrupt;
	rx0pop;

	farlda %&0xc0000%
	//test farsta.
	farsta 128,20,3;
	la 0
	sc 0,128;
	llb 20,3;
	//farilda;
	farldb 128,20,3;
	ab
	putchar;
	putchar;
	putchar;
	putchar;
	putchar;
	putchar;
	putchar;
	putchar;
	halt;

