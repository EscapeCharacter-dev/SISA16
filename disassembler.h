static int disassembler(char* fname, 
		unsigned long location, 
		unsigned long SISA16_DISASSEMBLER_MAX_HALTS,
		unsigned long end_location,
		u* M){
	unsigned long n_halts = 0;
	unsigned long n_illegals = 0;
	unsigned long i = location & 0xffFFff;
	unsigned long linenum = 0;
	unsigned long stepper = i;
	unsigned char opcode;
	unsigned char bad_binary_flag;
	unsigned long arg_i = 0;
	unsigned char bad_flag = 0;
	unsigned int unfinished_flag = 0;
	unsigned long required_spaces = 20;
	unsigned long opcode_i = i-1;
	unsigned long li;
	U short_interpretation = 0;
	u byte_interpretation = 0;
#ifndef SISA_DEBUGGER
	FILE* f; 
	f = fopen(fname, "rb");
	if(!f){puts("//ERROR: Could not open file to disassemble.");	exit(1);}
#endif
	
#ifndef SISA_DEBUGGER
	fseek(f, location, SEEK_SET);
#endif
	location &= 0xffFFff;
#ifndef SISA_DEBUGGER
	printf("\nsection 0x%lx;\n", location);
#else
	printf("\n\rsection 0x%lx;\n\r", location);
#endif
	for(i = location; i < end_location && i < 0x1000000 && linenum < max_lines_disassembler;){
		opcode;
		bad_binary_flag = 0;
		if(i >= 0x1000000){
#ifdef SISA_DEBUGGER
			printf("//Disassembly reached end of usable memory.\r\n");
#else
			puts("//Disassembly reached end of usable memory.");
#endif
			goto end_for_realsies;
		}

#ifndef SISA_DEBUGGER
		if(feof(f)) goto end;
		opcode = fgetc(f);i++;
#else
		opcode = M[stepper++ & 0xffFFff];i++;
#endif
		if(opcode == 0) 			n_halts++; 		
		else if(opcode < n_insns) 	n_halts = 0;

		if(opcode >= n_insns) 		n_illegals++; 	
		else if(opcode != 0) 		n_illegals = 0;
		
		if(
			opcode < n_insns &&  /*Valid opcode*/
			(insns_numargs[opcode] > 0) && /*With arguments*/
			(((i-1)+insns_numargs[opcode])>>16) != ((i-1)>>16)
		){bad_binary_flag = 1;}
		
		if(opcode >= n_insns){
			if(enable_dis_comments)
#ifndef SISA_DEBUGGER
				printf("%-12s 0x%x                     ;//0x%06lx  : Illegal Opcode (E_ILLEGAL_OPCODE)\n","bytes", (unsigned int)opcode, i-1);
#else
				printf("%-12s 0x%x                     ;//0x%06lx  : Illegal Opcode (E_ILLEGAL_OPCODE)\n\r","bytes", (unsigned int)opcode, i-1);
#endif
			else
#ifndef SISA_DEBUGGER
				printf("%-12s 0x%x                     ;\n","bytes", (unsigned int)opcode);
#else
				printf("%-12s 0x%x                     ;\n\r","bytes", (unsigned int)opcode);
#endif
			continue;
		}else{
			arg_i = 0;
			bad_flag = 0;
			unfinished_flag = 0;
			required_spaces = 25;
			opcode_i = i-1;
			short_interpretation = 0;
			byte_interpretation = 0;
			printf("%-13s",insns[opcode]);
			for(arg_i = 0; arg_i < insns_numargs[opcode]; arg_i++){
				unsigned char thechar;
				if(  ((i & 0xffFF) == 0) || (i >= 0x1000000)){
					bad_flag = 1;
				}
				if(arg_i > 0) {
					printf(",");
					required_spaces -= 5;
				}else{
					required_spaces -= 4;
				}


#ifndef SISA_DEBUGGER
				if(feof(f) || (unsigned long)ftell(f) != i){
					unfinished_flag = arg_i + 1;
				}
				thechar = fgetc(f);
#else
				thechar = M[stepper++ & 0xffFFff];
#endif
				short_interpretation += thechar;
				byte_interpretation = thechar;
				if(arg_i == 0)short_interpretation *= 256;
				printf("0x%02x",(unsigned int)thechar);i++;
			}
			
			if(required_spaces < 50)
			{for(li=0;li<required_spaces;li++){
				printf(" ");
			}}
			if(!enable_dis_comments) {
#ifndef SISA_DEBUGGER
				printf(";\n");
#else
				printf(";\n\r");
#endif
			}
			linenum++; if(linenum > max_lines_disassembler) goto end;
			if(enable_dis_comments)
			{
				printf(";//0x%06lx  :", opcode_i);
				if(bad_flag) printf("(E_WONT_RUN)");
				if(bad_binary_flag) printf("(E_INSN_ARGS_CROSS_REGION_BOUNDARY)");
				if(unfinished_flag) printf("(E_UNFINISHED_EOF AT ARGUMENT %u)", unfinished_flag-1);
				if(streq(insns[opcode], "farret")){
#ifndef SISA_DEBUGGER
					puts(" End of Procedure");
#else
					printf(" End of Procedure\r\n");
#endif
				}else if(
					streq(insns[opcode], "lla") 
					|| streq(insns[opcode], "llb")
				)
				{
					if( (short_interpretation & 0xFF00) == 0){
#ifndef SISA_DEBUGGER
						puts(" <TIP> replace with single-byte assignment, high byte is zero.");
#else
						printf(" <TIP> replace with single-byte assignment, high byte is zero.\r\n");
#endif
					}else{
#ifndef SISA_DEBUGGER
						printf("\n");
#else
						printf("\n\r");
#endif
					}
				}else if(streq(insns[opcode], "ret")){
#ifndef SISA_DEBUGGER
					puts(" End of Local Procedure");
#else
					printf(" End of Local Procedure\r\n");					
#endif
				} else if(opcode == 0 && n_halts == 1){
#ifndef SISA_DEBUGGER
					puts(" End of Control Flow");
#else
					printf(" End of Control Flow\r\n");
#endif
				} else if(
					streq(insns[opcode], "jmpc")||
					streq(insns[opcode], "jim")
				){
#ifndef SISA_DEBUGGER				
					puts(" Jump");
#else
					printf(" Jump\r\n");
#endif
				}else if(
					streq(insns[opcode], "jmpifeq") ||
					streq(insns[opcode], "jmpifneq") ||
					streq(insns[opcode], "jeq") ||
					streq(insns[opcode], "jne")
					
				){
#ifndef SISA_DEBUGGER
					puts(" Cond. Jump");
#else
					printf(" Cond. Jump\r\n");
#endif
				}else if(streq(insns[opcode], "cbrx0")
						||streq(insns[opcode], "carx0")){
#ifndef SISA_DEBUGGER
					puts(" ?: Far memory array access through RX0. Check array alignment!");
#else
					printf(" ?: Far memory array access through RX0. Check array alignment!\r\n");
#endif
				}else if(streq(insns[opcode], "clock")){
#ifndef SISA_DEBUGGER
					puts(" get sys time: A=ms, B=s");
#else
					printf(" get sys time: A=ms, B=s\r\n");
#endif
				}else if(streq(insns[opcode], "emulate")){
#ifndef SISA_DEBUGGER
					puts(" Drop into task process duplicating privileged memory.");
#else
					printf(" Drop into task process duplicating privileged memory.\r\n");
#endif				
				}else if(streq(insns[opcode], "priv_drop")){
#ifndef SISA_DEBUGGER
					puts(" Return to unprivileged process.");
#else
					printf(" Return to unprivileged process.\r\n");
#endif
				}else if(streq(insns[opcode], "task_ric")){
#ifndef SISA_DEBUGGER
					puts(" Reset preemption instruction counter for task.");
#else
					printf(" Reset preemption instruction counter for task.\r\n");
#endif				
				}else if(strprefix("task_",insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" Manipulate task regfile.");
#else
					printf(" Manipulate task regfile.\r\n");
#endif
				}else if(streq(insns[opcode], "interrupt")){
#ifndef SISA_DEBUGGER
					puts(" <DEVICE> interrupt. Return value in A.");
#else
					printf(" <DEVICE> interrupt. Return value in A.\r\n");
#endif
				}else if(streq(insns[opcode], "putchar")){
#ifndef SISA_DEBUGGER
					puts(" <DEVICE> A->device");
#else
					printf(" <DEVICE> A->device\r\n");
#endif
				}else if(streq(insns[opcode], "getchar")){
#ifndef SISA_DEBUGGER
					puts(" <DEVICE> device->A");
#else
					printf(" <DEVICE> device->A\r\n");
#endif
				}else if(streq(insns[opcode], "sc")){
					if(short_interpretation == (opcode_i & 0xffFF) ||
						short_interpretation == ((opcode_i+3) & 0xffFF)){
#ifndef SISA_DEBUGGER
						puts(" !: Loop top. <TIP>: replace with cpc.");
#else
						printf(" !: Loop top. <TIP>: replace with cpc.\r\n");
#endif
					}else{
#ifndef SISA_DEBUGGER
						puts(" ?: arg is address");
#else
						printf(" ?: arg is address\r\n");
#endif
					}
				}else if(streq(insns[opcode], "crx0")
						||streq(insns[opcode], "crx1")
						||streq(insns[opcode], "crx2")
						||streq(insns[opcode], "crx3")){
#ifndef SISA_DEBUGGER
					puts(" ?: Addressing through RX reg.");
#else
					printf(" ?: Addressing through RX reg.\r\n");
#endif
				}else if(streq(insns[opcode], "ca")){
#ifndef SISA_DEBUGGER			
					puts(" ?\?: Addressing through reg A");
#else
					printf(" ?\?: Addressing through reg A\r\n");
#endif
				}else if(streq(insns[opcode], "cb")){
#ifndef SISA_DEBUGGER
					puts(" ?\?: Addressing through reg B");
#else
					printf(" ?\?: Addressing through reg B\r\n");
#endif
				}else if(streq(insns[opcode], "astp")){
#ifndef SISA_DEBUGGER
					puts(" Stack manip, maybe retrieving fn arguments?");
#else
					printf(" Stack manip, maybe retrieving fn arguments?\r\n");
#endif
				}else if(streq(insns[opcode], "lcall")){
#ifndef SISA_DEBUGGER
					puts(" Local Procedure Call");
#else
					printf(" Local Procedure Call\r\n");
#endif
				}else if(streq(insns[opcode], "call")){
#ifndef SISA_DEBUGGER
					puts(" Local Procedure Call Through C");
#else
					printf(" Local Procedure Call Through C\r\n");
#endif
				}else if(streq(insns[opcode], "fcall")){
#ifndef SISA_DEBUGGER
					puts(" Procedure Call");
#else
					printf(" Procedure Call\r\n");
#endif
				}else if(streq(insns[opcode], "farcall")){
#ifndef SISA_DEBUGGER
					puts(" Procedure Call through pcr:A and pc:C");
#else
					printf(" Procedure Call through pcr:A and pc:C\r\n");
#endif
				}else if(streq(insns[opcode], "lfarpc")){
#ifndef SISA_DEBUGGER
					printf(" Region Jump to region %u\n", (unsigned)byte_interpretation);
#else
					printf(" Region Jump to region %u\r\n", (unsigned)byte_interpretation);
#endif
				}else if(streq(insns[opcode], "farjmprx0")){
#ifndef SISA_DEBUGGER
					puts(" Far Jump Through RX0");
#else
					printf(" Far Jump Through RX0\r\n");
#endif
				}else if( streq(insns[opcode], "farllda")
						||streq(insns[opcode], "farlldb")
						||streq(insns[opcode], "farldc")
						||streq(insns[opcode], "farldrx0")
						||streq(insns[opcode], "farldrx1")
						||streq(insns[opcode], "farldrx2")
						||streq(insns[opcode], "farldrx3")
						||streq(insns[opcode], "lda")
						||streq(insns[opcode], "ldb")
						||streq(insns[opcode], "ldc")
						||streq(insns[opcode], "llda")
						||streq(insns[opcode], "lldb")
				){
#ifndef SISA_DEBUGGER
					puts(" ?: Loading from fixed variable.");
#else
					printf(" ?: Loading from fixed variable.\r\n");
#endif
				}else if( streq(insns[opcode], "farstla")
						||streq(insns[opcode], "farstlb")
						||streq(insns[opcode], "farstc")
						||streq(insns[opcode], "farstrx0")
						||streq(insns[opcode], "strx0")
						||streq(insns[opcode], "farstrx1")
						||streq(insns[opcode], "strx1")
						||streq(insns[opcode], "farstrx2")
						||streq(insns[opcode], "strx2")
						||streq(insns[opcode], "farstrx3")
						||streq(insns[opcode], "strx3")
						||streq(insns[opcode], "sta")
						||streq(insns[opcode], "stb")
						||streq(insns[opcode], "stc")
						||streq(insns[opcode], "stla")
						||streq(insns[opcode], "stlb")
				){
#ifndef SISA_DEBUGGER
					puts(" ?: Storing to fixed variable.");
#else
					printf(" ?: Storing to fixed variable.\r\n");
#endif
				}else if(streq(insns[opcode], "cpc")){
#ifndef SISA_DEBUGGER
					puts(" ?: Loop top is next insn");
#else
					printf(" ?: Loop top is next insn\r\n");
#endif
				}else if(streq(insns[opcode], "lgp")){
#ifndef SISA_DEBUGGER
					printf(" imm. value into GP reg");
#else
					printf(" imm. value into GP reg\r\n");
#endif
				}else if(streq("farldgp", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" load from far address into GP reg");
#else
					puts(" load from far address into GP reg\r\n");
#endif
				}else if(streq("farstgp", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" store into far address from GP reg");
#else
					puts(" store into far address from GP reg\r\n");
#endif
				}else if(streq("farildgp", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" Indirect load GP reg through RX0");
#else
					puts(" Indirect load GP reg through RX0\r\n");
#endif
				}else if(streq("faristgp", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" Indirect store GP reg through RX0");
#else
					puts(" Indirect store GP reg through RX0\r\n");
#endif

				}else if(streq("gprx0", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" Move RX0 to gp reg");
#else
					puts(" Move RX0 to gp reg\r\n");
#endif
				}else if(streq("gprx1", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" Move RX1 to gp reg");
#else
					puts(" Move RX1 to gp reg\r\n");
#endif
				}else if(streq("gprx2", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" Move RX2 to gp reg");
#else
					puts(" Move RX3 to gp reg\r\n");
#endif
				}else if(streq("gprx3", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" Move RX3 to gp reg");
#else
					puts(" Move RX3 to gp reg\r\n");
#endif

				}else if(streq("rx0gp", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" Move gp reg to RX0");
#else
					puts(" Move gp reg to RX0\r\n");
#endif
				}else if(streq("rx1gp", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" Move gp reg to RX1");
#else
					puts(" Move gp reg to RX1\r\n");
#endif
				}else if(streq("rx2gp", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" Move gp reg to RX2");
#else
					puts(" Move gp reg to RX2\r\n");
#endif
				}else if(streq("rx3gp", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" Move gp reg to RX3");
#else
					puts(" Move gp reg to RX3\r\n");
#endif
				}else if(streq("rx3gp", insns[opcode])){
#ifndef SISA_DEBUGGER
					puts(" Move gp reg to another");
#else
					puts(" Move gp reg to another");
#endif
				}else {
#ifndef SISA_DEBUGGER
					printf("\n");
#else
					printf("\r\n");
#endif
				}
				if(unfinished_flag){
#ifndef SISA_DEBUGGER
					puts("\n//End of File, Last Opcode is inaccurately disassembled (E_UNFINISHED_EOF)");
#else
					printf("\n\r//End of File, Last Opcode is inaccurately disassembled (E_UNFINISHED_EOF)\r\n");
#endif
					goto end_for_realsies;
				}
			}
		}
		if((n_halts) > SISA16_DISASSEMBLER_MAX_HALTS
			|| (n_illegals) > SISA16_DISASSEMBLER_MAX_HALTS){
			goto end;
		}
	}

	end:
#ifndef SISA_DEBUGGER
	puts("\n//Reached End.\n");
	end_for_realsies:
	fclose(f);
#else
	printf("\n\r//Reached End.\n\r");
	end_for_realsies:
#endif
	return 0;
}
