#!/usr/bin/sisa16_asm -run
..zero:
	sc %pbh_beg%; jmpc;
pbh_beg:
	getchar;
	apush;
	lb 4;rsh;
	lb 0xf;and;
	lb 7;mul;
	llb %printbytehex_jmptable_1%; add;ca;jmpc;
	:printbytehex_jmptable_1:
		la 0x30;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x31;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x32;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x33;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x34;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x35;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x36;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x37;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x38;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x39;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x41;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x42;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x43;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x44;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x45;putchar;sc %printbytehex_jmptable_1_end%;jmpc
		la 0x46;putchar;sc %printbytehex_jmptable_1_end%;jmpc
	:printbytehex_jmptable_1_end:
	apop;lb 15;and;lb 7;mul;
	llb %printbytehex_jmptable_2%;add;ca;jmpc;
	:printbytehex_jmptable_2:
		la 0x30;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x31;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x32;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x33;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x34;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x35;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x36;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x37;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x38;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x39;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x41;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x42;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x43;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x44;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x45;putchar;sc %printbytehex_jmptable_2_end%;jmpc
		la 0x46;putchar;sc %printbytehex_jmptable_2_end%;jmpc
	:printbytehex_jmptable_2_end:
halt;
