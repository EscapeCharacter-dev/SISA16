#!/usr/bin/sisa16_asm -run



//use a separately compiled libc.
..include"libc_pre.hasm"
..(LIBC_REGION):
..dinclude"libc_pre.bin"

//use the normal libc.
//..include"libc.hasm"

..(4):
..decl_farproc:proc_fib
${int a; int b; int c; int i;

	$>i;
	$|
		[if]{$<i;lrx1 %/2%;rxcmp;lte;}{
			lrx0 %/1%; $return;
		}
	$|

	la 1; rx0a; $>a; $>b;
	//while loop to compute fibonacci numbers.
	$|
		[while]{
			$<i;
			lrx1 %/2%;
			rxcmp;
			gt;
		}{
			$<a;rx1_0;
			$<b;
			rxadd;
			$>c;
			//move b into a
			$<b;
			$>a;
			//move c into b
			$<c;
			$>b;
			//i--;
			$<i;rxdecr;$>i;
		}
	$|;
	
	$<c;
	$return;
$}

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

	//test gp registers
	lrx0 %/0%;
	lgp 17, %/0xABCDEF98%;
	rx0gp 17;
	proc_print32hex
	
	halt;

