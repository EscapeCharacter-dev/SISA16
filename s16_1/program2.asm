#!/usr/bin/sisa16_asm -run


//Program to demonstrate usage of FOR from structure.hasm


//use a separately compiled libc.
//..include"libc_pre.hasm"
//..(LIBC_REGION):
//..dinclude"libc_pre.bin"

//use the normal libc.
..include"libc.hasm"

..main(3):
	lrx0 %/0x770000%;
	proc_krenel;
	halt;

..(0x78):
:buf1:
fill 1024,0;
:end_of_program_string:
..asciz:You win!
:request:
..asciz:Enter the secret code:
:secret_code:
..asciz:panini

..decl_farproc:no_runme
${	(byte f)\//what a nice byte
	uint[500] q;\//what a nice array
	uint[20] q2;//an even nicer array

$|
	nop;$<f;nop;
	arx0;putchar;
	la '\n'; putchar;
	$return;//expands to farret
$|

$}

..decl_farproc:add_two_numbers
${(int a, int b, int[1] secret_array) byte before; int[500] test; int beyond;

$|

#notice how these comments are just fine?

	$<a;rx1_0;
	
	$<b;rxadd;

	lrx1 %/4%;
		
	$>secret_array;#Hi!

	lrx1 %/0%;//hello!

	$<test;
	
	$>b;	$return
$|

$}

:store_a_string:
fill 256,0;

..(0x77):
	//[for]{init}{condition eval}{step}

.innerfor:[for]{lrx0 %/0%; rx0push; }{rx0pop;rx0push;lrx1 %/20%;rxcmp;lt;}{la ' ';putchar;rx0pop;rxincr;rx0push;};rx0pop;


	[for]{lrx0 %/0%; rx0push;}\
		{rx0pop;rx0push;lrx1 %/3000%;rxcmp;lb 0;cmp;}\
		{innerfor;innerfor;la 'q';putchar;rx0pop;rxincr;rx0push;}
	rx0pop;
	innerfor;
	astp;
	rx0a;
	proc_printbytehex;
	la '\n';

	apush;
	no_runme;
	no_runme;
	apop;



	
	la '=';putchar;
	
	lrx0 %-81%;
	lrx1 %/17%;
	rx0push;
	rx1push;
	add_two_numbers;
	rx0pop;
	rx0pop;
	
	//result is in rx0
	//print the result of add_two_numbers
	lrx1 %/store_a_string%
	proc_itoa_dec
	lrx0 %/store_a_string%;
	proc_prints;

		la '\n'; putchar;
	la '\n'; putchar;
	la '\n'; putchar;
	
	lrx0 %/request%; proc_prints; la '\n'; putchar;

	//[GOTOIF]{label}{condition code}
$|
	[GOTOIF]{end_of_program}
	{
		lrx0 %/buf1%; 
		proc_gets;
	 	lrx0 %/buf1%; 
	 	lrx1 %/secret_code%; 
	 	proc_strcmp;
	 	nota;
	}
$|
	
	halt;

	:end_of_program:
	lrx0 %/end_of_program_string%; proc_prints;
	halt;
