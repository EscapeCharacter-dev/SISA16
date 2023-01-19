#!/usr/bin/sisa16_asm -run



//use a separately compiled libc.
..include"libc_pre.hasm"
..(LIBC_REGION):
..dinclude"libc_pre.bin"

//use the normal libc.
//..include"libc.hasm"

..main:
	lrx0 %/0x70000%;
	proc_krenel;
	halt;
..(7):
	lrx0 %0xc%,%0%;
	proc_gets
	la '\n'; putchar; la '\r'; putchar;
	lrx0 %0xc%,%0%;
	proc_stringlen;
	rxincr;
	rx2_0;
	//The length of the string (plus one, for the null byte) is now in RX2, the src and dest arguments must be setup.
	lrx0 %/0xd0000%
	lrx1 %/0xc0000%
	proc_memcpy;
	lrx0 %0xd%,%0%;
	proc_prints;
	la '\n'; putchar; la '\r'; putchar;
	lrx0 %/0xd0000%
	proc_atof_dec
	rx2_0

	rx0_2;proc_print32hex;
	la '\n'; putchar;
	la '\n'; putchar;
	la '\n'; putchar;
	la '\n'; putchar;
	la '\r'; putchar;
	la '\r'; putchar;
	la '\r'; putchar;
	la '\r'; putchar;


	//Alloc testing.
	lrx0 %/0x1000%;
	proc_alloc;
	rx2_0
	la '\n'; putchar; la '\r'; putchar;
	la '\n'; putchar; la '\r'; putchar;
		//print the number! should be 0.
	rx0_2;proc_print32hex;rx0_2;
	proc_free;

	//this should also be 0.
	la '\n'; putchar; la '\r'; putchar;
	la '\n'; putchar; la '\r'; putchar;
	lrx0 %/0xF000%;
		proc_alloc;
		rx2_0
			//print the number!
		rx0_2;proc_print32hex;


	//this should return the number above rounded up.
	la '\n'; putchar; la '\r'; putchar;
	la '\n'; putchar; la '\r'; putchar;
	lrx0 %/0x400%;
		proc_alloc;rx2_0;
	rx2push;
		rx0_2;proc_print32hex;

		//this should return the some of the last two numbers above, rounded up.
	la '\n'; putchar; la '\r'; putchar;
	la '\n'; putchar; la '\r'; putchar;
	lrx0 %/0x400%;
		proc_alloc;rx2_0;
		rx0_2;proc_print32hex;

	rx0pop; proc_free;

			//this should return the number from test #3
	la '\n'; putchar; la '\r'; putchar;
	la '\n'; putchar; la '\r'; putchar;
	//lrx0 %/0x401%;//enabling this line will cause this allocation the after the previous one instead of in the spot of test#3.
	lrx0 %/0x400%;
		proc_alloc;rx2_0;
		rx0_2;proc_print32hex;
			
halt;
