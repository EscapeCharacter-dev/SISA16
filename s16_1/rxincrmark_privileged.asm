#!/usr/bin/sisa16_asm -run
lb 0;rx1b;rx0b;
sc %looptop%;
looptop:
rxincr;
rxcmp;
jmpifneq;
halt;
