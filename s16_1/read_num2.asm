#!/usr/bin/sisa16_asm -run



//use a separately compiled libc.
..include"libc_pre.hasm"
..(LIBC_REGION):
..dinclude"libc_pre.bin"

//use the normal libc.
//..include"libc.hasm"

..(5):
length_of_input_string:
bytes %/0%;

..main(3):
	lrx0 %/what%
	proc_krenel
	halt;
	what:
	la '\n'; interrupt;
	lrx0 %0xc%,%0%;
	proc_gets
	lrx0 %0xc%,%0%;
	proc_prints
	la '\n'; putchar; la '\r'; putchar;
	lrx0 %0xc%,%0%;
	proc_stringlen;
	//we now have the length of the string stored, we have to get it into RX0.
	rxincr;
	farstrx0 %&length_of_input_string%;
	rx2_0;
	//The length of the string (plus one, for the null byte) is now in RX2, the src and dest arguments must be setup.
	lrx0 %/0xd0000%
	lrx1 %/0xc0000%
	proc_memcpy;
	lrx0 %0xd%,%0%;
	proc_prints;
	la '\n'; putchar; la '\r'; putchar;
	//We would now like to write to disk.
	lrx0 %/0x3001%						//location in file.
	llb %0xd00%						//the source in memory.
	proc_fwrite

	lrx0 %/0x3001%						//the source in file.
	llb %0xe00%						//the destination in memory.
	proc_fread
	
	lrx0 %/0xe0000%
	proc_atoi_dec
	proc_rxsqrt
	lrx1 %/0xc0000%;
	proc_itoa_dec
	lrx0 %0xc%,%0%;
	proc_prints
	la '\n'; putchar;
	la '\r'; putchar;


//Now, test rxpowi
	lrx0 %0xc%,%0%;
	proc_gets
	lrx0 0,%&0xc0000%
	proc_atoi_dec;
	lrx1 %/3%
	proc_rxpowi;
	lrx1 %/0xc0000%;
	proc_itoa_dec
	lrx0 0,%&0xc0000%
	proc_prints
	la '\n'; putchar;
	la '\r'; putchar;
	la '\n' 
	interrupt

	lrx0 %/0x1000%;
	proc_alloc;
	proc_alloc;
	rx2_0
	la '\n'; putchar; la '\r'; putchar;
		//print the number!
	proc_print32lchex
	rx0_2;
	//lrx1 %/256%;rxadd;
	proc_free;
		sc %the_end%; 
		nota; 
		jmpifeq;
	la '\n'; putchar; 
	la '\r'; putchar;



	lrx0 %/0xF00%;
		proc_alloc;
	rx2_0
	proc_print32lchex
	rx0_2
	la '\n'; putchar; 
	la '\r'; putchar;


	lrx0 %/0xBAFF00%;
		proc_alloc;
	rx2_0
	proc_print32lchex
	rx0_2
	la '\n'; putchar; 
	la '\r'; putchar;


	lrx0 %/0xFFE5%;
	proc_alloc;
	rx2_0
	proc_print32lchex
	rx0_2
	la '\n'; putchar; 
	la '\r'; putchar;

	lrx0 %/0xFFE5%;
	proc_alloc;
	rx2_0
	proc_print32lchex
	rx0_2
	la '\n'; putchar; 
	la '\r'; putchar;

	the_end:
	halt;
