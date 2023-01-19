#!/usr/bin/sisa16_asm -run

..include"libc_pre.hasm"
..zero:
..dinclude"libc_pre.bin"


..(0x25):
	my_page_o_useless_garbage:
	fill 256, 'Q'
	bytes 0;
..(7):
:STR_my_string:
..ascii:Enter some text:
bytes '\r' ,'\n', 0;

bytes 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0;
bytes 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0;

:STR_my_other_string:
..ascii:Krenel shut down...
bytes '\r' ,'\n', 0;
:STR_yet_other_string:
..ascii:End of Program.
bytes '\r' ,'\n', 0;
:goodbye_string_loc:
..asciz:Goodbye!


//ENTER: exec 2048 on the command line!!!
..(55):

	
	// Overwrite the krenel. There is no way 
	//it would work if this was being done to krenel memory!
	lrx0 %LIBC_REGION%, %LIBC_KRENEL_BEGIN%;
	lrx1 %LIBC_REGION%, %LIBC_KRENEL_END%;
	overwrite_krenel_looptop:
		cbrx0;
		la 0;
		farista;
		rxincr;
		rxcmp;
		lt;
		jeq %overwrite_krenel_looptop%;

	side_process_looptop:
		la 'W';
		putchar;
		la '\n'; syscall;
		lla %0xDE02%; syscall; //sleep!
		la 100; alpush; proc_wait; alpop;
		farllda %&0xAABBCC%; 
		nota;
		jeq %side_process_looptop%;
	//halt;
	//Shutdown.
	//lla %0xDEAD%; syscall;

	//write to disk! See if it makes it through at all.


	
	llb %0x2500%;
	lrx0 %/0xFF000%;
	lla %0xDE01%; //Disk write.
	syscall;

	llb %0x2600%;
	lrx0 %/0xFF000%;
	lla %0xDE03%; //Disk read.
	syscall;
	la '\r'; putchar;
	la '\n'; putchar; 
	la 'q'; putchar;
	la 'u'; putchar;
	la 'e'; putchar;
	la '?'; putchar;
	la '\r'; putchar;
	la '\n'; putchar; interrupt;

	//print what we got from disk.
	lrx0 %/0x260000%;
	//proc_prints;

	la '\n'; interrupt;
	//Fork bomb.
	sc %0%; 
	lb 0;la 2;		farista;
	lb 1;la 1;		farista;
	lb 2;la 0x44;	farista;
	lla %0xDE06%;
	syscall;
	lrx2 %/0x50000%;
	lrx1 %/0x90000%;
	bruhtop:
		rx0_2;
		rx0_1; rxincr; rx1_0;
		sc %bruhtop%; jmpc;
	
	lb 0; mod;
	halt;
..(8):
	krenel_boot:
		lla %0xDE02%; syscall; //sleep!
		la '\r'; putchar;
		la '\n'; putchar;
		la '\n'; interrupt;
		push %10%; //make some room for that bootloader!
		//Attempt to get a lock on the segment.
		cpc;
		la 'q'; putchar;
		la '\n'; putchar; interrupt;
		lla %0xDE08%; syscall;
		jmpifneq;



		//exec region syscall. Exec region
		//copies krenel into the process's memory,
		//so it will definitely have it.
		lla %0xDE04%; lb 55; syscall;


		nota; sc %main_program_failure%; jmpifeq;
		lrx0 0, %&STR_my_string%;
		proc_prints;
		la '\n'; syscall;
		nota; sc %main_program_failure%; jmpifeq;
		//Send a KILL to pid0
		//lla %0xDE00%;lb 0;syscall;
		lrx0 0, %&STR_my_string%;
		proc_gets;
		la '\r'; putchar;
		la '\n'; putchar;
		la '\n'; syscall;
		lrx0 0, %&STR_my_string%;
		proc_prints;
		la '\r'; putchar;
		la '\n'; putchar;
		la '\n'; syscall;
		//Send a message to all processes! we send it to their memory, AABBCC
		lla %0xDE05%; 
		lb 1;
		sc %0%; 
		lrx0 %/0xAABBCC%; 
		syscall;

		
		lla %0xDE05%; 
		lb 1;
		sc %1%; 
		lrx0 %/0xAABBCC%; 
		syscall;		

		lla %0xDE05%; 
		lb 1;
		sc %2%; 
		lrx0 %/0xAABBCC%; 
		syscall;

		lla %0xDE05%; 
		lb 1;
		sc %2%; 
		lrx0 %/0xAABBCC%; 
		syscall;

		lla %0xDE05%; 
		lb 1;
		sc %3%; 
		lrx0 %/0xAABBCC%; 
		syscall;

		lla %0xDE05%; 
		lb 1;
		sc %4%; 
		lrx0 %/0xAABBCC%; 
		syscall;


		lrx0 0, %&STR_my_string%;
		proc_gets
		la '\r'; putchar;
		la '\n'; putchar;
		la '\n'; syscall;
		lrx0 0, %&STR_my_string%;
		proc_prints;
		la '\r'; putchar;
		la '\n'; putchar;
		la '\n'; syscall;
		lrx0 0, %&STR_yet_other_string%;
		proc_prints;
		
		
		[FOR]{kbooter_kill_children_loop}\
		{lrx0 %/2%;lrx1 %/33%;}\
		{rxcmp;lb 0;cmp;}\
		{brx0; lla %0xDE00%; syscall;rxincr;}

		lrx0 0, %&goodbye_string_loc%;
		proc_prints;
		la '\n'; interrupt;
		lla %0xDE00%;lb 1;syscall; //Kill ourselves. We are actually 1.
		//lla %0xDEAD%; syscall;
		lb 0; rx1b; rxdiv
		main_program_failure:
		la 'f'; putchar;
		halt;
