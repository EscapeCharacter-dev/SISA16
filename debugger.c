#define SISA_DEBUGGER
#include "d.h"
#include "isa.h"
/*
	Textmode emulator for SISA16.
*/

static char* filename = NULL;
static char* env_home = NULL;
static char* settingsfilename = NULL;

unsigned char enable_dis_comments = 1;
static unsigned long max_lines_disassembler = 30;
#include "instructions.h"
#include "stringutil.h"
#include "disassembler.h"
#ifndef NO_SIGNAL
#include <signal.h>
#endif

static unsigned long debugger_run_insns = 0; /*if this is less than one, the debugger is not */
static char is_fresh_start = 1;
static char freedom = 0;
static char watched_register = '\0';
static UU watched_register_value = 0;
static char* names[0x10000];
static UU name_vals[0x10000];
static char name_buf_temp[80]; /*for sprintf*/
static unsigned long n_names = 0;
UU sisa_breakpoints[0x10000];
UU n_breakpoints = 0;
UU debugger_setting_maxhalts = 3;
UU debugger_setting_clearlines = 500;

char debugger_setting_do_dis = 1;
char debugger_setting_do_hex = 0;
char debugger_setting_minimal = 0;
char debugger_setting_repeat = 0; /*r*/
char* debugger_saved_last = NULL;
static u M2[(((UU)1)<<24)];

#define N "\r\n"
void respond(int bruh){
	(void)bruh;
	if(!debugger_setting_minimal)
		printf("\n\r<Heard SIGINT! if you want to quit, type q and hit enter at the REPL.>\r\n");
	else
		printf("[!]\r\n");
	freedom=0;
	debugger_run_insns=0;
	watched_register = '\0';
	return;
}
#if defined(linux) || defined(__linux__) || defined(__linux) || defined (_linux) || defined(_LINUX) || defined(__LINUX__)
void segmentation_violation(int bruh){
	(void)bruh;
	printf("\r\n<<SEGV>>\r\n");
	exit(1);
}
#endif

char savenames(const char* filename){
	unsigned long i = 0;
	unsigned char have_written = 0;
	FILE* fout = NULL;
	for(i=0; i < n_breakpoints; i++){
		if(sisa_breakpoints[i] != 0x1FFffFF)
			have_written = 1;
	}
	for(i=0; i < n_names; i++)
	{
		if(names[i])
			have_written = 1;
	}
	if(have_written == 0) {
		fout = fopen(filename, "r");
		if(!fout) return 0;
		fclose(fout);
		remove(filename);
		return 0;
	}
	have_written = 0;
	fout = fopen(filename, "w");
	
	if(!fout)
	{
		if(!debugger_setting_minimal)
			printf("\r\nCould not open output debugger file %s\r\n", filename);
		else
			printf("\r\n<badfile: %s>\r\n", filename);
		return 1;
	}
	for(i=0; i < n_names; i++)
	{
		if(names[i]) {
			if(!have_written) fprintf(fout, "!"); /*signal to use */
			fprintf(fout, "%s|%lu|", names[i], name_vals[i]);
			have_written = 1;
		}
	}
	if(have_written)
		fprintf(fout, "///|");
	else
		fprintf(fout, "_");
	/*
		write breakpoints to file.
	*/
	for(i=0; i < n_breakpoints; i++){
		if(sisa_breakpoints[i] != 0x1FFffFF)
			fprintf(fout, "%lu|", sisa_breakpoints[i]);
	}
	fclose(fout);
	if(!debugger_setting_minimal)
		printf("Saved Debugging file.\r\n");
	else
		printf("[saved]\r\n");
	return 0;
}
char loadnames(const char* filename){
	unsigned long i      = 0;
	unsigned long lenout = 0;
	char* entry = NULL;
	FILE* fin = fopen(filename, "r");
	if(!fin)
	{
		if(!debugger_setting_minimal)
			printf("\r\nCould not find debugger file %s\r\n", filename);
		else
			printf("\r\n<nofile: %s>\r\n", filename);
		return 1;
	}
	for(i=0;i<n_names;i++) if(names[i]) free(names[i]);
	n_names = 0;
	n_breakpoints = 0;
	if(fgetc(fin) == '!')
	{
		do{
			i = n_names;
			entry = read_until_terminator_alloced(fin, &lenout, '|', 40);
			if(!entry) {
				printf("\r\nFAILED MALLOC\r\n");
				fclose(fin);return 0;
			}
			if(streq(entry, "///") || strlen(entry) == 0){
				free(entry); 
				entry = NULL; break;
			}
			names[i] = entry;
			entry = read_until_terminator_alloced(fin, &lenout, '|',40);
			if(!entry) {
				printf("\r\nFAILED MALLOC\r\n");
				free(names[i]); names[i] = NULL; fclose(fin); return 0;
			}
			name_vals[i] = strtoul(entry, 0,0);
			free(entry); entry = NULL;
			n_names++;
		}while(n_names < 0x10000);
	} 
	/*We are now parsing breakpoints*/
	if(feof(fin)){fclose(fin);return 0;}
	/*We are now parsing breakpoints*/
	do{
		i = n_breakpoints;
		entry = read_until_terminator_alloced(fin, &lenout, '|', 40);
		if(!entry) {printf("\r\nFAILED MALLOC\r\n");fclose(fin);return 0;}
		if(strlen(entry) == 0 || entry[0] > 126 || entry[0] < 0) { /*Probably never happens.*/
			free(entry); entry = NULL;break;
		}
		sisa_breakpoints[i] = strtoul(entry, 0,0);
		free(entry);
		entry = NULL;
		n_breakpoints++;
	} while(n_breakpoints < 0x10000);
	fclose(fin);
	return 0;
}

static char* read_until_terminator_alloced_modified(FILE* f){
	unsigned char c;
	char* buf;
	unsigned long bcap = 40;
	char* bufold;
	unsigned long blen = 0;
	buf = STRUTIL_ALLOC(40);
	if(!buf) return NULL;
	fflush(stdout);
#ifdef USE_TERMIOS
	fcntl(STDIN_FILENO, F_SETFL, 0);
	setvbuf ( stdout, stdout_buf, _IONBF, sizeof(stdout_buf));
#endif
	
	while(1){
		if(feof(f)){break;}
		c = fgetc(f);
		if(c == '\n' || c=='\r') {break;}
		if(c > 127) continue; /*Pretend it did not happen.*/
		if(c < 8) continue; /*Also didn't happen*/
		if(
			c == 127
			|| c==8
		)
		{
			if(!debugger_setting_minimal)
				{if(blen) printf("[CANCELLED]\r\n");}
			else
				{if(blen) printf("[C]\r\n");}
			blen = 0;
			continue;
		}
		if(!is_fresh_start)
			putchar(c);
		if(blen == (bcap-1))	/*Grow the buffer.*/
			{
				bcap<<=1;
				bufold = buf;
				buf = STRUTIL_REALLOC(buf, bcap);
				if(!buf){
					free(bufold); 
					printf("<!UH OH! realloc failure.>\r\n");
					return NULL;
				}
			}
		buf[blen++] = c;
	}
	buf[blen] = '\0';
	return buf;
}
static char create_name(UU address, char* name){
	unsigned long i;
	for(i = 0; i < n_names; i++){
		if(names[i] == NULL) continue;
		if(streq(names[i], name)) {
			name_vals[i] = address;
			return 0;
		}
	}
	/*search for a null.*/
	for(i = 0; i < n_names; i++){
		if(names[i] == NULL){
			names[i] = strcatalloc(name,"");
			if(!names[i]){
				printf("\r\n!!Failed Malloc!! aborting...\r\n");
				exit(1);
			}
			name_vals[i] = address;
			return 0;
		}
	}
	if(n_names > 0xffFF){
		return 1;
	}
	names[n_names] = strcatalloc(name,"");
	if(!names[n_names]){
		printf("\r\n!!Failed Malloc!! aborting...\r\n");
		exit(1);
	}
	name_vals[n_names++] = address;
	return 0;
	
}
static void delete_name(UU addr){
	unsigned long i;
	for(i = 0; i < n_names; i++){
		if(names[i] == NULL) continue;
		if(name_vals[i] == addr){
			free(names[i]);
			names[i] = NULL;
			name_vals[i] = 0;
		}
	}
	return;
}
static char* get_name_eval(UU i){
	{
		if(names[i] != NULL)
		{
			name_buf_temp[79] = '\0';
			sprintf(name_buf_temp, "%lu", (unsigned long)name_vals[i] & 0xFFffFFff);
			return name_buf_temp;
		}
	}
	name_buf_temp[0] = '\0';
	return name_buf_temp; /*Always return a valid pointer!*/
}
static char make_breakpoint(UU new_breakpoint){
	unsigned long i = 0;
	new_breakpoint &= 0xffFFff;
	for(; i < n_breakpoints; i++){
		if(sisa_breakpoints[i] == new_breakpoint)
			return 0;
		else if(sisa_breakpoints[i] == (UU)0x1FFffFF){
			sisa_breakpoints[i] = new_breakpoint;
			return 1;
		}
	}
	if(n_breakpoints >= 0xffFF){
		if(!debugger_setting_minimal)
			printf("\r\n<Cannot make a breakpoint, there are too many already>\r\n");
		else
			printf("\r\n<too many>\r\n");
	}
	sisa_breakpoints[n_breakpoints++] = new_breakpoint;
	return 1;
}

static char delete_breakpoint(UU breakpoint){
	unsigned long i = 0;
	breakpoint &= 0xffFFff;
	for(; i < n_breakpoints; i++){
		if(sisa_breakpoints[i] == breakpoint){
			sisa_breakpoints[i] = (UU)0x1FFffFF;
			if(i == n_breakpoints-1) {
				n_breakpoints--;
				while(n_breakpoints && sisa_breakpoints[n_breakpoints-1] == 0x1ffFFff)n_breakpoints--;
			}
			return 1;
		}
	}
	return 0;
}
static void help(){
	if(!debugger_setting_minimal)
		printf(
				N "~~SISA16 Debugger~~"
				N "Author: DMHSW"
				N "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
				N "~~Let all that you do be done with love~~"
				N "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
				N "COMMANDS:"
		);
	else
		printf(
			N "[Let all that you do be done with love]"
		);
	printf(
			N "h for [h]elp            | display this."
			N "t for s[t]atus          | display CPU state."
			N "m to [m]odify           | modify a register."
			N "    Every register has a name, check s[t]atus."
			N "    Operations: =, +, -, *, /, &, |, ^"
			N "    m p = 3 will set the program counter to 3."
			N "    m 0 = 0xAABBCCDD will set RX0 to that value."
			N "s to [s]tep             | step some number of insns."
			N "    s steps a single insn."
			N "    s 10 steps 10 insns."
			N "S to print [S]tring     | display a string."
			N "    S 0xAF00B9 will print a null-terminated string starting at that address."
			N "    S 0xAF00B9 20 will print 20 characters of that string."
			N "R to watch [R]egister   | step until register changes"
			N "    RA waits until register A changes."
			N "b to set [b]reakpoint   | Set a breakpoint."
			N "    b sets a breakpoint here."
			N "    b 0x10030 will set a breakpoint at 0x10030"
			N "    b@+ will set a breakpoint one byte ahead."
			N "e to d[e]l breakpoint   | Delete a breakpoint."
			N "    e 0x100ff deletes a breakpoint at that insn."
			N "    e deletes a breakpoint at this insn."
			N "l to [l]ist             | Print all breakpoints and names."
			N "    this command also saves your breakpoints and names."
			N "u to r[u]n              | Run until breakpoint."
			N "x to view he[x]         | View raw bytes of disassembly"
			N "d to [d]isassemble      | disassemble."
			N "    SYNTAX: d 50 will disassemble 50 lines from the program counter."
			N "    You may use a location,"
			N "    d 50 0x100 will dis. 50 lines at 0x100."
			N "q to [q]uit             | quit debugging."
			N "w to [w]rite byte       | write byte to memory. Syntax: w 0xAB00E0 12"
			N "a to [a]lter u16        | Modify a short in memory. "
			N "    a 0xAF3344 0xBBCC will write 0xBBCC to 0xAF3344"
			N "A to [A]lter u32        | Modify a 32 bit uint in memory. "
			N "    A 0xAF3344 0xAABBCCDD will modify the u32 at 0xAF3344 to 0xAABBCCDD."
			N "v to [v]iew             | view the 8, 16, and 32 bit representations of a memory address."
			N "    v 0xAF00B9 will display the 8, 16, and 32 bit interpretations"
			N "    of the value at that address, as well as float"
			N "    if the floating point unit is enabled."
			N "n to [n]ame address     | name an address for quick reference."
			N "    A named address can be referenced in any command with &name&."
			N "    the syntax of this command is important,"
			N "    the name must be delimited by whitespace characters and the initial n."
			N "    Valid:"
			N "        n myLabel 30"
			N "        n myLabel         30"
			N "        nmyLabel 30"
			N "        nmyLabel     30"
			N "    Note that @ as a character is used as a special name equal to R<<16 + P"
			N "    You can thus do n myLabel @ to quickly create a label at the current location."
			N "    You can then do b &myLabel& to create a breakpoint there, or just b@"
			N "    There is a very fast shortcut for doing exactly this:"
			N "N to u[N]-name address  | delete an address name."
			N "    N 0x1ffee will delete all names associated with that address."
			N "    N &mylabel& will delete mylabel if it exists."
			N "r to [r]eload           | reload at the current emulation depth. "
			N "g for settin[g]         | view/change settings. g d 50 sets setting d to 50"
			N "p for dum[p]            | dump memory -> dump.bin"
			N "c for [c]lear           | print some blank lines."
			N "j for [j]ump            | Change the PC to a value. You can use + or t"
			N "    to jump forward by a number of instructions, rather than bytes."
			N "    j 0x130 jumps to 0x130 in the current region."
			N "    j +10 jumps forward 10 insns"
			N "    j t f jumps forward until it finds an instruction starting with f"
			N "J for far[J]ump         | Change the PC and PC region to a value."
			N "    J 0x10000  will jump to 0x10000 setting [R] to 1, and [P] to 0."
			N "    J 0xAFBBCC will jump to  0xBBCC in [R] 0xAF."
		N);
}

void debugger_hook(unsigned short *a,
									unsigned short *b,
									unsigned short *c,
									unsigned short *stack_pointer,
									unsigned short *program_counter,
									unsigned char *program_counter_region,
									UU *RX0,
									UU *RX1,
									UU *RX2,
									UU *RX3,
									u* EMULATE_DEPTH,
									u *M
){
	char* line = NULL;
	if(freedom)
	{	unsigned long i=0;
		if(n_breakpoints == 0) return;
		for(; i < n_breakpoints; i++){
			if(((unsigned long)*program_counter + (((unsigned long)*program_counter_region)<<16)) == sisa_breakpoints[i])
				{
					freedom = 0;
					debugger_run_insns = 0;
					watched_register = '\0';
				}
		}
		if(freedom) return; /*still in freedom mode.*/
	}
	

	
	if(debugger_run_insns)
	{unsigned long i = 0;
		debugger_run_insns--;
		if(n_breakpoints)
			for(; i < n_breakpoints; i++){
				if(((unsigned long)*program_counter + (((unsigned long)*program_counter_region)<<16)) == sisa_breakpoints[i])
				{
						freedom = 0;
						debugger_run_insns = 0;
						watched_register = '\0';
				}
			}
		if(debugger_run_insns) return;
	}

	if(watched_register != '\0'){unsigned long i = 0;
		if(n_breakpoints)
			for(; i < n_breakpoints; i++){
				if(((unsigned long)*program_counter + (((unsigned long)*program_counter_region)<<16)) == sisa_breakpoints[i])
				{
						freedom = 0;
						debugger_run_insns = 0;
						watched_register = '\0';
				}
			}
		switch(watched_register){
			case 'a':
			case 'A':
				if (*a == watched_register_value) return;
			break;
			case 'b':
			case 'B':
				if (*b == watched_register_value) return;
			break;
			case 'c':
			case 'C':
				if (*c == watched_register_value) return;
			break;
			case 'p':
			case 'P':
				if (*program_counter == watched_register_value) return;
			break;
			case 'r':
			case 'R':
				if (*program_counter_region == watched_register_value) return;
			break;
			case 's':
			case 'S':
				if (*stack_pointer == watched_register_value) return;
			break;
			case '0':
				if (*RX0 == watched_register_value) return;
			break;
			case '1':
				if (*RX1 == watched_register_value) return;
			break;
			case '2':
				if (*RX2 == watched_register_value) return;
			break;
			case '3':
				if (*RX3 == watched_register_value) return;
			break;
			case 'E':
			case 'e':
				if (*EMULATE_DEPTH == watched_register_value) return;
			break;
			case 'G':
			case 'g':
				if (SEGMENT_PAGES == watched_register_value) return;
			break;
			default:
				printf("\r\nWhat register are you talking about?\r\n");
				watched_register = '\0';
			goto repl_pre;
		}
		watched_register = '\0';
	}

	if(is_fresh_start){
		if(!debugger_setting_minimal)	help();
		if(!debugger_setting_minimal)
			printf(
				N "<Stopped at initialization, please note the first step will not actually execute an instruction, it is a dead cycle>"
			N);
		else
			printf(N "[START]" N);
		is_fresh_start = 0;
#ifndef NO_SIGNAL
		signal(SIGINT, respond);
#if defined(linux) || defined(__linux__) || defined(__linux) || defined (_linux) || defined(_LINUX) || defined(__LINUX__)
		signal(SIGSEGV, segmentation_violation);
#endif
#endif
	}
	repl_pre:
	if(debugger_setting_do_hex){
		unsigned long i = 0;
		unsigned long lines = 0;
		unsigned long n_halts = 0;
		unsigned long n_illegals = 0;
		unsigned long insns = 0x1000000;
		unsigned long location = (unsigned long)*program_counter + (((unsigned long)*program_counter_region)<<16);
		if(!debugger_setting_minimal) printf("\r\nHex:\r\n");
		for(;i<insns && lines < max_lines_disassembler && (location+i < 0x1000000);i++){
			unsigned char opcode;
			unsigned long j;
			opcode = M[location+i];
			printf(": %02lx", (unsigned long)M[(location+i) & 0xffFFff]);
			if(M[location+i] < n_insns){
				for(j=0;j<insns_numargs[opcode];j++){
					printf(" %02lx", (unsigned long)M[(location+i+j+1) & 0xffFFff]);
				}
				if(insns_numargs[opcode])
					i += j;
			} else{
				n_illegals++;
				n_halts = 0;
			}
			if(opcode == 0) {n_halts++;n_illegals = 0;}
			if(n_halts > debugger_setting_maxhalts || n_illegals > debugger_setting_maxhalts)break;
			printf("\r\n");lines++;
		}
	}
	if(debugger_setting_do_dis){
		disassembler(
			filename, 
			(unsigned long)*program_counter + (((unsigned long)*program_counter_region)<<16), 
			debugger_setting_maxhalts,
			0x1000000,
			M
		);
	}

	repl_start:

		printf("<region: %lu, pc: 0x%04lx >\n\r", (unsigned long)*program_counter_region, 
													  (unsigned long)*program_counter
		);
		if(line)free(line);
		line = NULL;
		line = read_until_terminator_alloced_modified(stdin);
		if(!line){
			puts("\r\n Failed Malloc.");
			dcl();
			exit(1);
		}
		if(line[0] == '\0' && debugger_setting_repeat && debugger_saved_last)
		{
			line = strcatalloc(debugger_saved_last, "");
			if(!line)
			{
				puts("\r\n Failed Malloc.");
				dcl();
				exit(1);
			}
			if(!debugger_setting_minimal)
				printf("\r\nRepeating command '%s'\r\n", line);
		}
		if(line[0] > 126 || line[0] <= 0) goto repl_start;
		printf("\r\n");
		{unsigned long i;
			unsigned long location = (unsigned long)*program_counter + (((unsigned long)*program_counter_region)<<16);
			for(i = 0; i < n_names; i++){
				if(names[i] == NULL) continue;
				while(strfind(line, names[i]) != -1){
					line = str_repl_allocf(line, names[i], get_name_eval(i));
							if(!line){
								puts("\r\n Failed Malloc.");
								dcl();
								exit(1);
							}
				}
			}
			sprintf(name_buf_temp, "%lu", location);
				while(strfind(line, "@") != -1) {
					line = str_repl_allocf(line, "@", name_buf_temp);
					if(!line){
						puts("\r\n Failed Malloc.");
						dcl();
						exit(1);
					}
				}
		}
		if(debugger_saved_last) free(debugger_saved_last);
		debugger_saved_last = NULL;
		debugger_saved_last = strcatalloc(line, "");
		switch(line[0]){
			default:
			if(!debugger_setting_minimal)
				puts("\r\n<unrecognized command>\r\n");
			else
				puts("\r\n<bad>\r\n");
			case 'h':help();goto repl_start;
			case 'c':
			{
#if defined(WIN32) || defined(_WIN32)
				system("CLS");
#else
#if defined(linux) || defined(__linux__) || defined(__linux) || defined (_linux) || defined(_LINUX) || defined(__LINUX__) || defined(__unix__)
				system("clear");
#else
				unsigned long i = 0;
				for(;i < debugger_setting_clearlines;i++){
					printf("\r\n");
				}
#endif
#endif
			}
			goto repl_start;
			case 'p':{
				FILE* duck = fopen("dump.bin", "wb");
				if(!duck){
					printf("\r\nCannot open dump.bin\r\n");
					goto repl_start;
				}
				fwrite(M, 1, 0x1000000, duck);
				fclose(duck);
				if(!debugger_setting_minimal)
					printf("\r\nSuccessfully dumped memory to dump.bin\r\n");
				else
					printf("\r\n[dumped]\r\n");
				goto repl_start;
			}
			case 'g':{
				unsigned long stepper = 1;
				char setting;
				unsigned long mode;
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == 0){
					printf("~~Settings~~\r\n");
					printf("d: 0x%06lx    | default number of lines to dis. ahead.\r\n", (unsigned long)max_lines_disassembler);
					printf("i: 0x%08lx  | disassemble at every step?\r\n", (unsigned long)debugger_setting_do_dis);
					printf("x: 0x%08lx  | show hex at every step?\r\n", (unsigned long)debugger_setting_do_hex);
					printf("c: 0x%08lx  | Show dis. comments?\r\n", (unsigned long)enable_dis_comments);
					printf("l: 0x%08lx  | blank lines in a clear command (*nix will use system clear)\r\n", (unsigned long)debugger_setting_clearlines);
					printf("h: 0x%08lx  | Maximum halts or illegals in a dis.?\r\n", (unsigned long)debugger_setting_maxhalts);
					printf("m: 0x%08lx  | Minimal display?\r\n", (unsigned long)debugger_setting_minimal);
					printf("r: 0x%08lx  | enter will repeat the previous command?\r\n", (unsigned long)debugger_setting_repeat);
					goto repl_start;
				}
				setting = line[stepper++];
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == 0){
					printf("Missing Mode.\r\n"); goto repl_start;
				}
				mode = strtoul(line + stepper, 0,0);
				switch(setting){
					default:
					if(!debugger_setting_minimal)
						printf("Unknown setting.\r\n");
					else
						printf("<bad>\r\n");
					goto repl_start;
					case 'd': max_lines_disassembler = mode & 0xffFFff;break;
					case 'i': debugger_setting_do_dis = mode;break;
					case 'x': debugger_setting_do_hex = mode;break;
					case 'c': enable_dis_comments = mode;break;
					case 'l': debugger_setting_clearlines = mode;break;
					case 'h': debugger_setting_maxhalts = mode;break;
					case 'm': debugger_setting_minimal = mode;break;					
					case 'r': debugger_setting_repeat = mode; debugger_saved_last = strcatalloc(line, ""); break;
				}
				if(settingsfilename)
				{
					FILE* settingsfile = NULL;
					settingsfile = fopen(settingsfilename, "w");
					if(!settingsfile) {
						printf("\r\nError working with settings file.\r\n");
						goto repl_start;
					}
					fprintf(settingsfile, "d %lu\n", (unsigned long)max_lines_disassembler);
					fprintf(settingsfile, "i %lu\n", (unsigned long)debugger_setting_do_dis);
					fprintf(settingsfile, "x %lu\n", (unsigned long)debugger_setting_do_hex);
					fprintf(settingsfile, "c %lu\n", (unsigned long)enable_dis_comments);
					fprintf(settingsfile, "l %lu\n", (unsigned long)debugger_setting_clearlines);
					fprintf(settingsfile, "h %lu\n", (unsigned long)debugger_setting_maxhalts);
					fprintf(settingsfile, "m %lu\n", (unsigned long)debugger_setting_minimal);
					fprintf(settingsfile, "r %lu\n", (unsigned long)debugger_setting_repeat);
					printf("\r\nSaved Settings.\r\n");
					fclose(settingsfile);
				}
				goto repl_start;
			}
			case 'q':
			dcl();
			{
				char* tmp =strcatalloc(filename, ".dbg");
				savenames(tmp);
				free(tmp);
			}
			exit(0);
			case '\0':
			case '\r':
			case '\n':
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			goto repl_start;
			case 'u': freedom = 1; free(line); return;
			case 'd':{
				unsigned long stepper = 1;
				unsigned long tempsetting = 0;
				unsigned long insns = max_lines_disassembler;
				unsigned long location = (unsigned long)*program_counter + (((unsigned long)*program_counter_region)<<16);
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') goto disassemble_end;
				insns = strtoul(line + stepper, 0,0); /*grab number of insns*/
				for(;!isspace(line[stepper]) && line[stepper];stepper++); /*skip the number*/
				for(;isspace(line[stepper]);stepper++); /*skip spaces.*/
				if(line[stepper] == '\0') goto disassemble_end;
				location = strtoul(line + stepper, 0,0);
				disassemble_end:
				tempsetting = max_lines_disassembler;
					max_lines_disassembler = insns;
					disassembler(
							filename, 
							location, 
							debugger_setting_maxhalts,
							0x1000000,
							M
					);
				max_lines_disassembler = tempsetting;
				goto repl_start;
			}
			case 'x':{
				unsigned long i = 0;
				unsigned long lines = 0;
				unsigned long n_halts = 0;
				unsigned long n_illegals = 0;
				unsigned long stepper = 1;
				unsigned long insns = max_lines_disassembler;
				unsigned long location = (unsigned long)*program_counter + (((unsigned long)*program_counter_region)<<16);
				/*Parse input.*/
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') goto hex_end;
				insns = strtoul(line + stepper, 0,0); /*grab number of insns*/
				for(;!isspace(line[stepper]) && line[stepper];stepper++); /*skip the number*/
				for(;isspace(line[stepper]);stepper++); /*skip spaces.*/
				if(line[stepper] == '\0') goto hex_end;
				location = strtoul(line + stepper, 0,0);

				hex_end:
				if(!debugger_setting_minimal) printf("\r\nHeX view:\r\n");
				for(;i<0x1000000 && lines < insns && (location+i < 0x1000000);i++){
					unsigned char opcode;
					unsigned long j;
					opcode = M[location+i];
					printf(": %02lx", (unsigned long)M[(location+i) & 0xffFFff]);
					if(M[location+i] < n_insns){
						for(j=0;j<insns_numargs[opcode];j++){
							printf(" %02lx", (unsigned long)M[(location+i+j+1) & 0xffFFff]);
						}
						if(insns_numargs[opcode])i += j;
					} else{
						n_illegals++;
						n_halts = 0;
					}
					if(opcode == 0) {n_halts++;n_illegals = 0;}
					if(n_halts > debugger_setting_maxhalts || n_illegals > debugger_setting_maxhalts)break;
					printf("\r\n");lines++;
				}
				goto repl_start;
			}
			case 'w':{
				unsigned long stepper = 1;
				unsigned long addr = 0;
				unsigned long value = 0;
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rSyntax Error: Need address. \n\r");
					else
						printf("\n\r<no address?>\n\r");
					goto repl_start;
				}
				addr = strtoul(line + stepper, 0,0); /*grab number of insns*/
				for(;!isspace(line[stepper]) && line[stepper];stepper++); /*skip the number*/
				for(;isspace(line[stepper]);stepper++); /*skip spaces.*/
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rSyntax Error: Need value. \n\r");
					else
						printf("\n\r<no value?>\n\r");
					goto repl_start;
				}
				value = strtoul(line + stepper, 0,0);
				M[addr & 0xFFffFF] = value;
				printf("\r\n");
				goto repl_start;
			}
			case 'N':{
				unsigned long stepper = 1;
				unsigned long addr = 0;
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rSyntax Error: Need value. \n\r");
					else
						printf("\n\r<no value?>\n\r");
					goto repl_start;
				}
				addr = strtoul(line+stepper,0,0);
				delete_name(addr);
				goto repl_start;
			}
			case 'n':{
				unsigned long stepper = 1;
				unsigned long addr = 0;
				char* startname;
				char* endname;
				for(;isspace(line[stepper]);stepper++); /*skip initial space*/
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rSyntax Error: Need Name to assign. \n\r");
					else
						printf("\n\r<no name?>\n\r");
					goto repl_start;
				}
				startname = line + stepper; /*The actual first character of the name.*/
				for(;line[stepper] && !isspace(line[stepper]);stepper++); /*Skip the name itself.*/
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rSyntax Error: Need value. \n\r");
					else
						printf("\n\r<no value?>\n\r");
					goto repl_start;
				}
				endname = line + stepper;
				stepper++; /*We landed on a space. Logically we must move on.*/
				for(;isspace(line[stepper]);stepper++); /*Skip additional space.*/
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rSyntax Error: Need value. \n\r");
					else
						printf("\n\r<no value?>\n\r");
					goto repl_start;
				}
				addr = strtoul(line + stepper,0,0);
				startname--; *startname = '/';
				*endname = '/';
				endname++;*endname = '\0';
				/*Try to prevent the creation of names that would make the shit unusable.*/
				if(
					streq(startname, "///")
					|| strprefix("//", startname)
					|| streq("/", startname)
					|| (strfind(startname, "|") != -1)
					|| (strfind(startname, ";") != -1)
				){
					if(!debugger_setting_minimal)
						printf("\r\nForbidden name.\r\n");
					else
						printf("\r\n<bad name>\r\n");
					goto repl_start;
				}
				printf("\r\nCreating name '%s' with value %lu", startname, (unsigned long)addr);
				if(create_name(addr,startname)){
					if(!debugger_setting_minimal)
						printf("\r\nToo Many Names!\r\n");
					else
						printf("\r\n[too many]\r\n");
				}
				goto repl_start;
			}
			case 'v':{
				unsigned long stepper = 1;
				unsigned long addr = 0;
				UU val32;
				SUU sval32;
				U val16;
				u val8;
#ifndef NO_FP
				float valf;
#endif
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rSyntax Error: Need address. \n\r");
					else
						printf("\n\r<no address?>\n\r");
					goto repl_start;
				}
				addr = strtoul(line + stepper, 0,0) & 0xffFFff;
				{
					val8  = 	  M[addr];
					val16 = 	  M[addr               ] * 256 
								+ M[(addr+1) & 0xffFFff];
					val32 = 	  M[addr				] * 256*256*256
								+ M[(addr+1) & 0xffFFff	] * 256*256
								+ M[(addr+2) & 0xffFFff	] * 256
								+ M[(addr+3) & 0xffFFff	];
					memcpy(&sval32, &val32, 4);
#ifndef NO_FP
					memcpy(&valf, &val32, 4);
#endif
					printf("\r\nU8 :   0x%02lx | %lu", (unsigned long)val8, (unsigned long)val8);
					printf("\r\nU16: 0x%04lx | %lu", (unsigned long)val16, (unsigned long)val16);
					printf("\r\nU32: 0x%04lx | %lu", (unsigned long)val32, (unsigned long)val32);
#ifdef USE_UNSIGNED_INT
					printf("\r\nS32: %d", sval32);
#else
					printf("\r\nS32: %ld", sval32);
#endif
#ifndef NO_FP
					printf("\r\nFLT: %f", valf);
#endif
					printf("\r\n");
				}
				goto repl_start;
			}
			case 'S':{
				unsigned long stepper = 1;
				unsigned long addr = 0;
				unsigned long chars = 0xffFF;
				unsigned long i = 0;
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rSyntax Error: Need address. \n\r");
					else
						printf("\n\r<no address?>\n\r");
					goto repl_start;
				}
				addr = 0xFFffFF & strtoul(line + stepper, 0,0); /*addr*/
				for(;!isspace(line[stepper]) && line[stepper];stepper++); /*skip the number*/
				for(;isspace(line[stepper]);stepper++); /*skip spaces.*/
				if(line[stepper] == '\0'){
					goto string_printer;
				}
				chars = 0xFFffFF & strtoul(line + stepper, 0,0);
				string_printer:
				for(;i<chars;i++){
					if(addr + i >= 0x1000000) break;
					if(isspace(M[addr+i]) && M[addr+i] != ' '){
						if(M[addr+i] == '\t')
							printf("<tab character>\r\n");
						else if(M[addr+i] == '\v')
							printf("<vertical tab character>\r\n");
						else if(M[addr+i] == '\r')
							printf("<cr>\r\n");
						else if(M[addr+i] == '\n')
							printf("<nl>\r\n");
						else
							printf("<spacechar %u>\r\n", M[addr+i]);
						continue;
					}
					if(M[addr+i] == 127) {
						printf("<del>\r\n");
						continue;
					}
					if(M[addr+i] > 127){
						printf("<unprintable %u>\r\n", M[addr+i]);
						continue;
					}
					if(M[addr+i] == 0) break;
					if(isgraph(M[addr+i]) || M[addr+i] == ' '){
						if(M[addr+i] == '<')
							putchar('\\');
						else if(M[addr+i] == '\\') {putchar('\\');putchar('\\');}
						putchar(M[addr+i]);
						continue;
					}else{
						printf("<unprintable %u>\r\n", M[addr+i]);
					}
					
				}
				printf("\r\n");
				goto repl_start;
			}
			case 'a':{
				unsigned long stepper = 1;
				unsigned long addr = 0;
				unsigned long value = 0;
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rSyntax Error: Need address. \n\r");
					else
						printf("\n\r<no address?>\n\r");
					goto repl_start;
				}
				addr = strtoul(line + stepper, 0,0); /*grab number of insns*/
				for(;!isspace(line[stepper]) && line[stepper];stepper++); /*skip the number*/
				for(;isspace(line[stepper]);stepper++); /*skip spaces.*/
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rSyntax Error: Need value. \n\r");
					else
						printf("\n\r<no value?>\n\r");
					goto repl_start;
				}
				value = strtoul(line + stepper, 0,0);
				M[addr & 0xFFffFF] = value/256;
				M[(addr+1) & 0xFFffFF] = value;
				printf("\r\n");
				goto repl_start;
			}
			case 'A':{
				unsigned long stepper = 1;
				unsigned long addr = 0;
				unsigned long value = 0;
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rSyntax Error: Need address. \n\r");
					else
						printf("\n\r<no address?>\n\r");
					goto repl_start;
				}
				addr = strtoul(line + stepper, 0,0); /*grab number of insns*/
				for(;!isspace(line[stepper]) && line[stepper];stepper++); /*skip the number*/
				for(;isspace(line[stepper]);stepper++); /*skip spaces.*/
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rSyntax Error: Need value. \n\r");
					else
						printf("\n\r<no value?>\n\r");
					goto repl_start;
				}
				value = strtoul(line + stepper, 0,0);
				M[addr & 0xFFffFF] = value/(256*256*256);
				M[(addr+1) & 0xFFffFF] = value/(256*256);
				M[(addr+2) & 0xFFffFF] = value/(256);
				M[(addr+3) & 0xFFffFF] = value;
				printf("\r\n");
				goto repl_start;
			}
			case 's':
			{unsigned long stepper = 1;
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') {
					free(line);
					debugger_run_insns = 0;
					goto repl_end;
				}
				debugger_run_insns = strtoul(line+stepper, 0,0);
				if(debugger_run_insns == 0) goto repl_start; /*Simulate reputable behavior.*/
				debugger_run_insns--;
				free(line);
				goto repl_end;
			}

			case 'j':
			{
				unsigned long stepper = 1;
				unsigned long targ = 0;
				char targchar = '\0';
				char modus = 0;
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("\n\rMissing Jump target");
					else
						printf("\n\r<no jump target?>");
					goto repl_start;
				}else if(line[stepper] == '+'){
					modus = 1;stepper++;
					for(;isspace(line[stepper]);stepper++);
					if(line[stepper] == '\0'){
						if(!debugger_setting_minimal)
							printf("\n\rMissing Jump target");
						else
							printf("\n\r<no jump target?>");
					}
				}else if(line[stepper] == 't'){
					modus = 2;stepper++;
					for(;isspace(line[stepper]);stepper++);
					if(!isalpha(line[stepper])){
						if(debugger_setting_minimal)
							printf("\n\r<bad insn name>\r\n");
						else
							printf("\n\r<Bad Instruction Name>\r\n");
						goto repl_start;
					}
					targchar = line[stepper];
					targ = 0;
				}
				
				if(modus == 0 || modus == 1)
					targ = strtoul(line+stepper, 0,0);
				
				if(modus == 0){
					targ &= 0xffFF;
					if(!debugger_setting_minimal)
						printf("Jumping to : 0x%06lx\r\n",targ + (((UU)(*program_counter_region))<<16));
					else
						printf("->0x%06lx\r\n",targ + (((UU)(*program_counter_region))<<16));
				} else if(modus == 1){
					if(!debugger_setting_minimal)
						printf("Jumping forward %lu insns\r\n", targ);
					else
						printf("+%lu\r\n",targ);
				} else if(modus == 2){
					if(!debugger_setting_minimal)
						printf("Jumping until we see an instruction beginning with %c\r\n", targchar);
					else
						printf("[until %c]\r\n",targchar);
				}

				
				if(modus == 0){
					*program_counter = targ;
				}else if (modus == 1){
					unsigned long i = 0;
					for(;i < targ;i++){
							*program_counter += 1 + insns_numargs[
								M[
									*program_counter + (((UU)(*program_counter_region))<<16)
								]
							];
					}
				}
				else if(modus == 2)
				{
					unsigned long i = 0;
					for(;i < 0xffFF;i++){
						if(M[*program_counter + (((UU)(*program_counter_region))<<16)] >= n_insns)
						{
							if(!debugger_setting_minimal)
								printf("Hit Illegal Opcode.\r\n");
							else
								printf("[illop]\r\n");
						}
						if(  (insns[M[*program_counter + (((UU)(*program_counter_region))<<16)]])[0] == targchar){
							if(!debugger_setting_minimal)
								printf("Hit Op %s\r\n", insns[M[*program_counter + (((UU)(*program_counter_region))<<16)]]);
							else
								printf("[Op %s]\r\n", insns[M[*program_counter + (((UU)(*program_counter_region))<<16)]]);
							goto repl_start;
						}
						*program_counter += 1 + insns_numargs[
							M[
								*program_counter + (((UU)(*program_counter_region))<<16)
							]
						];
					}
				}
				goto repl_start;
			}
			case 'J':
			{
				unsigned long stepper = 1;
				unsigned long targ = 0;
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') {
					if(!debugger_setting_minimal)
						printf("No farjump target.\r\n");
					else
						printf("<target?>\r\n");
					goto repl_start;
				}
				targ = strtoul(line+stepper, 0,0);
				targ &= 0xffFFff;
				if(!debugger_setting_minimal)
					printf("Jumping to : 0x%06lx\r\n",targ);
				else
					printf("->0x%06lx\r\n",targ);
				*program_counter_region = targ / (256 * 256);
				*program_counter = targ;
				goto repl_start;
			}
			case 'b':
			{
				unsigned long stepper = 1;
				unsigned long location = (unsigned long)*program_counter + (((unsigned long)*program_counter_region)<<16);
				char mode = '\0';
				unsigned long modval = 0;
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') {
					if(!make_breakpoint(location)){
						if(!debugger_setting_minimal)
							printf("\r\nBreakpoint already exists.");
						else
							printf("\r\n[nothing to do]");
					}
					goto repl_start;
				}
				location = strtoul(line+stepper, 0,0);
				if(line[stepper] == '\0') goto make_breakpoint_end;
				else for(;line[stepper] && line[stepper] != '+' && !isspace(line[stepper]);stepper++); /*skip over the damn number*/
				for(;isspace(line[stepper]);stepper++); /*skip over space.*/
				if(line[stepper] == '\0') goto make_breakpoint_end;
				mode = line[stepper];stepper++; /*read the operation.*/
				for(;isspace(line[stepper]);stepper++); /*skip over space.*/
				if(line[stepper] == '\0') 
					modval = 1; 
				else 
					modval = strtoul(line+stepper, 0,0); /*read mod value.*/
				{
					if(!debugger_setting_minimal)
						printf("\r\nSetting breakpoint %lu insns ahead.\r\n", modval);
					else
						printf("\r\n[+%lu insns]\r\n", modval);
					if(mode == '+'){
						unsigned long i = 0;
						for(;i < modval;i++){
							if(M[location & 0xffFFff] >= n_insns)
							{
								if(!debugger_setting_minimal)
									printf("Hit Illegal Opcode.\r\n");
								else
									printf("[illop]\r\n");
							}
							location += 1 + insns_numargs[
								M[location & 0xffFFff]
							];
						}
					}
				}
				make_breakpoint_end:
				if(!make_breakpoint(location)){
						if(!debugger_setting_minimal)
							printf("\r\n");
						else
							printf("\r\n[nothing to do]");
				}
				goto repl_start;
			}
			case 'l':{
				unsigned long i = 0;
				for(i = 0; i < n_breakpoints; i++){
					if(sisa_breakpoints[i] != 0x1ffFFff)
						printf("\r\nb @: 0x%06lx",sisa_breakpoints[i]);
				}
				for(i = 0; i < n_names; i++){
					if(names[i])
						printf("\r\n'%s': 0x%08lx",names[i], name_vals[i]);
				}
				{
					char* fff = strcatalloc(filename, ".dbg");
					if(!fff){
						printf("\r\nFailed Malloc!\r\n");
						exit(1);
					}
					printf("\r\n");
					savenames(fff);
					free(fff);
				}
				
				
				goto repl_start;
			}
			case 'R':
			{
				unsigned long stepper = 1;
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0'){
					if(!debugger_setting_minimal)
						printf("\r\nWhat register should we watch?\r\n");
					else
						printf("\r\n<register?>\r\n");
					goto repl_start;
				}
				watched_register = line[stepper];
				switch(watched_register){
					default:
						if(!debugger_setting_minimal)
							printf("\r\nThat is not a real register!\r\n");
						else
							printf("\r\n<bad register>\r\n");
						goto repl_start;
					case 'a':
					case 'A':watched_register_value = *a;break;
					case 'b':
					case 'B':watched_register_value = *b;break;
					case 'c':
					case 'C':watched_register_value = *c;break;
					case 's':
					case 'S':watched_register_value = *stack_pointer;break;
					case 'p':
					case 'P':watched_register_value = *program_counter;break;
					case 'r':
					case 'R':watched_register_value = *program_counter_region;break;
					case '0':watched_register_value = *RX0;break;
					case '1':watched_register_value = *RX1;break;
					case '2':watched_register_value = *RX2;break;
					case '3':watched_register_value = *RX3;break;
					case 'E':
					case 'e':watched_register_value = *EMULATE_DEPTH;break;
					case 'g':
					case 'G':watched_register_value = SEGMENT_PAGES;break;
				}
				printf("Watching Register %c\r\n", watched_register);
				return;
			}
			case 'e':
			{
				unsigned long stepper = 1;
				unsigned long location = (unsigned long)*program_counter + (((unsigned long)*program_counter_region)<<16);
				for(;isspace(line[stepper]);stepper++);
				if(line[stepper] == '\0') {
					if(!delete_breakpoint(location)){
						if(!debugger_setting_minimal)
							printf("\r\nBreakpoint already deleted.");
						else
							printf("\r\n[nothing to do]");
					}
					goto repl_start;
				}
				location = strtoul(line+stepper, 0,0);
				if(!delete_breakpoint(location)){
					if(!debugger_setting_minimal)
						printf("\r\nBreakpoint already deleted.");
					else
						printf("\r\n[nothing to do]");
				}
				goto repl_start;
			}
			case 'm':
			{
				unsigned long stepper = 1;
				char to_edit = 'A';
				char operation = '=';
				UU value = 0;
			 	for(;isspace(line[stepper]);stepper++);
			 	if(line[stepper] == '\0') 
			 	{
			 		if(!debugger_setting_minimal)
			 			printf("\r\n<Syntax Error, no register to modify.>\r\n");
			 		else
			 			printf("\r\n<no register?>\r\n");
			 		goto repl_start;
			 	}
			 	to_edit = line[stepper++];
			 	for(;isspace(line[stepper]);stepper++);
			 	if(line[stepper] == '\0') 
			 	{
			 		if(!debugger_setting_minimal)
			 			printf("\r\n<Syntax Error, no operation.>\r\n");
			 		else
			 			printf("\r\n<no operation?>\r\n");
			 		goto repl_start;
			 	}
			 	operation = line[stepper++];
			 	for(;isspace(line[stepper]);stepper++);
			 	if(line[stepper] == '\0') 
			 	{
			 		if(!debugger_setting_minimal)
			 			printf("\r\n<Syntax Error, no value.>\r\n");
			 		else
			 			printf("\r\n<no value?>\r\n");
			 		goto repl_start;
			 	}
		 		value = strtoul(line + stepper, 0,0);
#define perform_surgery(REG) 	switch(operation){\
						 			case '=': REG = value; break;\
						 			case '+': REG += value; break;\
						 			case '-': REG -= value; break;\
						 			case '*': REG *= value; break;\
						 			case '/': \
										if(value)\
						 					REG /= value; \
						 				else if(!debugger_setting_minimal)\
						 					printf("\r\n Cannot divide by zero.\r\n");\
						 				else\
						 					printf("\r\n</0>\r\n");\
						 			break;\
						 			case '%': \
										if(value)\
						 					REG %= value; \
						 				else if(!debugger_setting_minimal)\
						 					printf("\r\n Cannot modulo by zero.\r\n");\
						 				else\
						 					printf("\r\n<%%0>\r\n");\
						 			break;\
						 			case '&':  REG &= value;  break;\
						 			case '|':  REG |= value;  break;\
						 			case '^':  REG ^= value;  break;\
									default:\
						 				if(!debugger_setting_minimal)\
						 					printf("\r\nNo such operation.\r\n");\
						 				else\
						 					printf("\r\n<bad op>\r\n");\
						 		}
				{const char* error_fmt = "\r\n<WARNING> this is extremely dangerous. This will probably cause a segmentation violation in the emulator.\r\n";
			 	switch(to_edit){
			 		
			 		case 'a':
			 		case 'A':
				 		perform_surgery(*a);
			 			
			 		break;
			 		case 'b':
			 		case 'B':
				 		perform_surgery(*b);
			 		break;
			 		case 'c':
			 		case 'C':
				 		perform_surgery(*c);
			 		break;
			 		case 's':
			 		case 'S':
				 		perform_surgery(*stack_pointer);
			 		break;
			 		case 'p':
			 		case 'P':
				 		perform_surgery(*program_counter);
			 		break;
			 		case 'r':
			 		case 'R':
				 		perform_surgery(*program_counter_region);
			 		break;
			 		case '0':
				 		perform_surgery(*RX0);
			 		break;
			 		case '1':
				 		perform_surgery(*RX1);
			 		break;
					case '2':
				 		perform_surgery(*RX2);
			 		break;
					case '3':
				 		perform_surgery(*RX3);
			 		break;
			 		case 'e':
			 		case 'E':
			 			perform_surgery(*EMULATE_DEPTH);
			 			*EMULATE_DEPTH &= 1;
			 		break;
			 		case 'g':
			 		case 'G':
			 			printf("<ERROR> segment pages is a compiletime constant.");
			 			exit(1);
			 		break;
			 		default:
			 			puts("\r\n<Error, no such register conforming to this name. the names are A,B,C,S,P,R,0,1,2,3");
			 		
			 	}
			 	}
			 	goto repl_start;
			}
			case 't':
				if(!debugger_setting_minimal){
					printf("\r\nFile: %s", filename);
					printf("\r\n~~Registers~~");
				}
				printf("\r\n[A]         =     0x%04lx", (unsigned long)*a);
				printf("\r\n[B]         =     0x%04lx", (unsigned long)*b);
				printf("\r\n[C]         =     0x%04lx", (unsigned long)*c);
				printf("\r\n[S]TP       =     0x%04lx", (unsigned long)*stack_pointer);
				printf("\r\n[P]C        =     0x%04lx", (unsigned long)*program_counter);
				printf("\r\nPC [R]EGION =       0x%02lx", (unsigned long)*program_counter_region);
				printf("\r\nRX[0]       = 0x%08lx", (unsigned long)*RX0);
				printf("\r\nRX[1]       = 0x%08lx", (unsigned long)*RX1);
				printf("\r\nRX[2]       = 0x%08lx", (unsigned long)*RX2);
				printf("\r\nRX[3]       = 0x%08lx", (unsigned long)*RX3);
				printf("\r\n[E]MU DEPTH =       0x%02lx", (unsigned long)(*EMULATE_DEPTH));
				printf("\r\nSEG PA[G]ES = 0x%08lx", (unsigned long)SEGMENT_PAGES);
				printf("\r\n");
			goto repl_start;
			case 'r':
				memcpy(M, M2, 0x1000000);
				*a = 0;
				*b = 0;
				*c = 0;
				R=0;
				*program_counter_region = 0;
				*program_counter = 0;
				*RX0 = 0;
				*RX1 = 0;
				*RX2 = 0;
				*RX3 = 0;
			goto repl_start;
		}
		repl_end: return;
}
#define debugger_stringify(x) #x
int main(int rc,char**rv){
	static FILE* F;
	UU i , j=~(UU)0;
	SUU q_test = (SUU)-1;
	/*M = malloc((((UU)1)<<24));*/
	
	/*
		attempt to load settings.
		First, from the home directory.
		Then, from the current directory.
	*/
	env_home = getenv("HOME");
	if(env_home)
	{FILE* settingsfile = NULL;
		settingsfilename = strcatalloc(env_home, "/.sisa16_dbg");
		if(!settingsfilename){
			printf("\r\nFailed Malloc.\r\n");
			exit(1);
		}
		settingsfile = fopen(settingsfilename, "r");
		if(!settingsfile){
			free(settingsfilename);
			settingsfilename = NULL;
		} else 
		fclose(settingsfile);
	} 

	if(!settingsfilename)
	{ /*From the current working directory.*/
		FILE* settingsfile = NULL;
		settingsfilename = strcatalloc("", ".sisa16_dbg");
		if(!settingsfilename){
			printf("\r\nFailed Malloc.\r\n");
			exit(1);
		}
		settingsfile = fopen(settingsfilename, "r");
		if(!settingsfile){
			free(settingsfilename);
			settingsfilename = NULL;
		} else 
		fclose(settingsfile);
	}

	if(settingsfilename){
		FILE* settingsfile = NULL;
		settingsfile = fopen(settingsfilename, "r");
		if(settingsfile){
			char* line;
			/*Attempt to read settings from the settings file.*/
			while(!feof(settingsfile)){
				line = read_until_terminator_alloced_modified(settingsfile);
				if(!line){
					printf("\r\nFailed Malloc.\r\n");
					exit(1);
				}
				while(isspace(line[0])){
					char* line_old = line;
					line = strcatalloc(line+1, "");
					if(!line){
						printf("\r\nFailed Malloc.\r\n");
						exit(1);
					}
					free(line_old);
				}
				switch(line[0]){
					case 'd':
					max_lines_disassembler = strtoul(line+1,0,0) & 0xffFFff;
					break;
					case 'i':
					debugger_setting_do_dis = strtoul(line+1,0,0);
					break;
					case 'x':
					debugger_setting_do_hex = strtoul(line+1,0,0);
					break;
					case 'c':
					enable_dis_comments = strtoul(line+1,0,0);
					break;
					case 'l':
					debugger_setting_clearlines = strtoul(line+1,0,0);
					break;
					case 'h':
					debugger_setting_maxhalts = strtoul(line+1,0,0);
					break;
					case 'm':
					debugger_setting_minimal = strtoul(line+1,0,0);
					break;
					case 'r':
					debugger_setting_repeat = strtoul(line+1,0,0);
					break;
					default:
					/*printf("\r\nIn Settings File: unknown setting %c\r\n", line[0]);*/
					break;
				}
			}
			fclose(settingsfile);
		} else {
			printf("\r\n <internal error working with settings file on line " debugger_stringify(__LINE__) " of debugger.c.>\r\n");
		}
	} else {
		printf("\r\n !!!! No settings file found. Create a file in your home folder or the current working directory called .sisa16_dbg to save settings!!!!\r\n");
	}
	if(
		(sizeof(U) != 2) ||
		(sizeof(u) != 1) ||
		(sizeof(UU) != 4)||
		(sizeof(SUU) != 4)
#ifndef NO_FP
		|| (sizeof(float) != 4)
#endif
	){
		puts("SISA16 ERROR!!! Compilation Environment conditions unmet.");
		if(sizeof(U) != 2)
			puts("U is not 2 bytes. Try using something other than unsigned short (default).");
		if(sizeof(u) != 1)
			puts("u is not 2 bytes. Try using something other than unsigned char (default).");
		if(sizeof(UU) != 4)
			puts("UU is not 4 bytes. Try toggling -DUSE_UNSIGNED_INT. the default is to use unsigned int as UU.");
		if(sizeof(SUU) != 4)
			puts("SUU is not 4 bytes. Try toggling -DUSE_UNSIGNED_INT. the default is to use int as SUU.");
#ifndef NO_FP
		if(sizeof(float) != 4){
			puts("float is not 4 bytes. Disable the floating point unit during compilation, -DNO_FP");
		}
#endif
		return 1;
	}
	memcpy(&i,&q_test, sizeof(UU));
	if(i != j){
		puts("<COMPILETIME ENVIRONMENT ERROR>This is not a two's complement architecture. You must define NO_SIGNED_DIV");
		exit(1);
	}
	j = (UU)0x80000000;
	q_test = -2147483648;
	memcpy(&i,&q_test, sizeof(UU));
	if(i != j){
		puts("<COMPILETIME ENVIRONMENT ERROR>This is not a two's complement architecture. It appears the sign bit is not the highest bit.\nYou must define NO_SIGNED_DIV");
		exit(1);
	}
	if(rc<2){
			puts("SISA-16 Standalone Debugger written by D[MHS]W for the Public Domain");
			puts("This program is Free Software that respects your freedom, you may trade it as you wish.");
			puts("\n\nSISA-16 Macro Assembler, Disassembler, Debugger, and Emulator in Pure Glorious ANSI/ISO C90, Version " SISA_VERSION);
			puts("\"Let all that you do be done with love\"");
			puts("Authored by DMHSW for the Public Domain. Enjoy.\n\n");
			puts("Developer's original repository: https://github.com/gek169/Simple_ISA.git");
			puts("Please submit bug reports and... leave a star if you like the project! Every issue will be read.");
			puts("Programmer Documentation for this virtual machine is provided in the provided manpage sisa16_asm.1");
			puts("~~COMPILETIME ENVIRONMENT INFORMATION~~");
#if defined(NO_SEGMENT)
			puts("The segment was disabled during compilation. emulate is also disabled.");
#else
			puts("The segment is enabled, so is Emulate.");
#endif

#if defined(NO_EMULATE)
			puts("emulate and emulate_seg are disabled.");
#else

#if defined(NO_SEGMENT)
			puts("... but you CAN use emulate_seg! Just not emulate.");
#else
			puts("You may use emulate and emulate_seg");
#endif

#endif

#if defined(NO_FP)
			puts("Floating point unit was disabled during compilation. Float ops generate error code 8.");
#else
			puts("Floating point unit was enabled during compilation. You may use fltadd, fltsub, fltmul, fltdiv, and fltcmp");
			if(sizeof(float) != 4) puts("Floats are an incorrect size. You may not use the floating point unit. Disable the floating point unit.");
#endif
#if defined(NO_SIGNED_DIV)
			puts("32 bit signed integer division instructions were disabled during compilation, they generate error code 10.");
			puts("You can manually implement signed twos-complement integer division.");
#else
			puts("32 bit signed integer division instructions were enabled during compilation. Don't divide by zero!");
#endif
			printf("Size of u is %u, it should be 1\n", (unsigned int)sizeof(u));
			printf("Size of U is %u, it should be 2\n", (unsigned int)sizeof(U));
			printf("Size of UU is %u, it should be 4\n", (unsigned int)sizeof(UU));
			printf("Size of SUU is %u, it should be 4\n", (unsigned int)sizeof(SUU));
#if !defined(NO_FP)
			printf("Size of float is %u, it should be 4\n", (unsigned int)sizeof(float));
#endif

#ifdef __STDC_IEC_559__
#if __STDC_IEC_559__ == 0
		puts("Floating point compatibility of the environment is specifically not guaranteed.");
#else
		puts("Floating point compatibility of the environment is specifically guaranteed.");
#endif

#else
		puts("the STDC_IEC_559 define is not defined. Standard behavior is not guaranteed.");
#endif
		if(sizeof(void*) == 2) puts("Compiled for a... 16 bit architecture? This is going to be interesting...");
		if(sizeof(void*) == 4) puts("Compiled for a 32 bit architecture.");
		if(sizeof(void*) == 8) puts("Compiled for a 64 bit architecture.");
#if defined(__i386__) || defined(_M_X86) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__IA32__) || defined(__INTEL__) || defined(__386)
		puts("Compiled for x86");
#endif
#if defined(__ia64__) || defined(_IA64) || defined(__ia64) || defined(_M_IA64) || defined(__itanium__)
		puts("Compiled for Itanium.")
#endif
#if defined(__m68k__) || defined(M68000) || defined(__MC68K__)
		puts("Compiled for the M68k");
#endif
#if defined(__mips__) || defined(__mips) || defined(_MIPS_ISA) || defined(__MIPS__)
		puts("Compiled for MIPS.");
#endif
#if defined(__powerpc__) || defined(__powerpc) || defined(__powerpc64__) || defined(__POWERPC__) || defined(__ppc__) || defined(__ppc64__) || defined(_M_PPC) || defined(_ARCH_PPC) || defined(__PPCGECKO__) || defined(__PPCBROADWAY__) || defined(__ppc)
		puts("Compiled for PowerPC");
#endif
#if defined(__THW_RS6000)
		puts("Compiled for RS/6000");
#endif
#if defined(__sparc__) || defined(__sparc) || defined(__sparc_v8__) || defined(__sparc_v9__) || defined(__sparcv8) || defined(__sparcv9)
		puts("Compiled for sparc.");
#endif
#if defined(__sh__)
		puts("Compiled for SuperH.");
#endif
#if defined(__370__) || defined(__s390__) || defined(__s390x__) || defined(__zarch__) || defined(__SYSC_ZARCH__)
		puts("Compiled for SystemZ");
#endif
#if defined(_TMS320C2XX) || defined(_TMS320C5X) || defined(_TMS320C6X)
		puts("Compiled for TMS320");
#endif
#if defined(__TMS470__)
		puts("Compiled for TMS470");
#endif
#if defined(__SISA16__)
		puts("Compiled for... This architecture!\nIf you're reading this from the source code, let this be a challenge to you:"
		"\nWrite a C compiler for SISA16 and tell me about it on Github. Then, show me some screenshots.\nI'll give you a special prize.");
		puts("The first person to get sisa16_asm to be self-hosting will win something. If that's you, Submit an issue! https://github.com/gek169/Simple_ISA");
#endif

#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
		puts("Compiled for x86_64");
#endif
#if defined(__alpha__) || defined(_M_ALPHA)
		puts("Compiled for Alpha.");
#endif
#if defined(__arm__) || defined(_ARM) || defined(_M_ARM)|| defined(_M_ARMT) || defined(__thumb__) || defined(__TARGET_ARCH_ARM)|| defined(__TARGET_ARCH_THUMB)
		puts("Compiled for arm.");
#endif
#if defined(__aarch64__)
		puts("Compiled for aarch64");
#endif
#if defined(__bfin)
		puts("Compiled for blackfin.");
#endif
#if defined(__convex__)
		puts("Compiled for convex.");
#endif
#if defined(__epiphany__)
		puts("Compiled for epiphany.");
#endif
#if defined(__hppa__) || defined(__HPPA__)
		puts("Compiled for hppa.");
#endif
#if defined(WIN32) || defined(_WIN32)
			puts("Compiled for Macrohard Doors.");
#endif
#if defined(__unix__)
			puts("Targetting *nix");
#endif
#if defined(linux) || defined(__linux__) || defined(__linux) || defined (_linux) || defined(_LINUX) || defined(__LINUX__)
			puts("Targetting Linux. Free Software Is Freedom.");
#endif
#if defined(__MINGW32__)
			puts("Compiler self-identifying as MINGW.");
#endif
#if defined(__TenDRA__)
			puts("Compiled with TenDRA. Gotta say, that's a cool name for a compiler.");
			return 0;
#endif

#ifdef __INTEL_LLVM_COMPILER
			puts("Compiled with Intel LLVM. Duopoly Inside.");
			return 0;
#endif

#ifdef __cilk
			puts("Compiled with Cilk. Duopoly inside.");
			return 0;
#endif


#ifdef __TINYC__
			puts("Compiled with TinyCC. All Respects to F. Bellard and Crew~~ Try TinyGL! https://github.com/C-Chads/tinygl/");
			return 0;
#endif

#ifdef _MSVC_VER
			puts("Gah! You didn't really compile my beautiful software with that disgusting compiler?");
			puts("MSVC is the worst C compiler on earth. No, not just because Microsoft wrote it. They make *some* good products.");
			puts("The bytecode is horrible and the compiler is uncooperative and buggy.");
			puts("Herb Sutter can go suck an egg! He's trying to kill C++.");
			return 0;
#endif

#ifdef __SDCC
			puts("Compiled with SDCC. Please leave feedback on github about your experiences compiling this with SDCC, I'd like to know.");
			return 0;
#endif

#ifdef __clang__
			puts("Compiled with Clang. In my testing, GCC compiles this project faster *and* produces faster x86_64 code.");
			return 0;
#endif

#ifdef __GNUC__
			puts("Compiled with GCC. Free Software Is Freedom.");
			return 0;
#endif
			puts("The C compiler does not expose itself to be one of the ones recognized by this program. Please tell me on Github what you used.");
			return 0;
	}
	{
		char* fn = NULL;
		fn = strcatalloc(rv[1], ".dbg");
		if(fn){
			loadnames(fn);
			free(fn);
		}
	}
	F=fopen(rv[1],"rb");
	if(!F){
		puts("SISA16 debugger cannot open this file.");
		exit(1);
	}
	filename = rv[1];
		for(i=0;i<0x1000000 && !feof(F);){M_SAVER[0][i++]=fgetc(F);}
		memcpy(M2, M_SAVER[0], 0x1000000);
	fclose(F);
	R=0;

	/*
	*/
	e();
	puts("\r\nExecution Finished normally.\r\n");
	if(R==0)puts("\r\nNo Errors Encountered.\r\n");
	for(i=0;i<(1<<24)-31&&rc>2;i+=32)	
		for(j=i,printf("%s\n%04lx|",(i&255)?"":"\n~",(unsigned long)i);j<i+32;j++)
			printf("%02x%c",M_SAVER[0][j],((j+1)%8)?' ':'|');
	if(R==1)puts("\n<Errfl, 16 bit div by 0>\n");
	if(R==2)puts("\n<Errfl, 16 bit mod by 0>\n");
	if(R==3)puts("\n<Errfl, 32 bit div by 0>\n");
	if(R==4)puts("\n<Errfl, 32 bit mod by 0>\n");
	if(R==5)puts("\n<Errfl, Bad Segment Page>\n");
	if(R==6){
		/*puts("\n<Errfl, Segment Cannot be Zero Pages>\n");*/
		puts("\r\n<Errfl, deprecated error>\r\n");
	}
	if(R==7)puts("\n<Errfl, Segment Failed Allocation>\n");
#if defined(NO_FP)
	if(R==13)
		{puts("\n<Errfl, Either signed division or the FPU were disabled during compilation.>\n");
		R=0;}
	if(R==8)puts("\n<Errfl, Floating point unit disabled by compiletime options>\n");
#else
	if(R==8)puts("\n<Errfl, Internal error, reporting broken SISA16 FPU. Report this bug! https://github.com/gek169/Simple_ISA/  >\n");
#endif

	if(R==9)puts("\n<Errfl, Floating point divide by zero>\n");
#if defined(NO_SIGNED_DIV)
	if(R==13)
		{puts("\n<Errfl, Either signed division or the FPU were disabled during compilation.>\n");
		R=0;}
	if(R==10)puts("\n<Errfl, Signed 32 bit division disabled by compiletime options>\n");
#else
	if(R==10)puts("\n<Errfl, Internal error, reporting broken SISA16 signed integer division module. Report this bug! https://github.com/gek169/Simple_ISA/  >\n");
#endif
	if(R==11)puts("\n<Errfl, Sandboxing limit reached >\n");
	if(R==12)puts("\n<Errfl, Sandboxing could not allocate needed memory.>\n");
	if(R==13)
		{
			puts("\n<Errfl, Internal error, Broken Float-Int Interop. Report this bug! https://github.com/gek169/Simple_ISA/  >\n");
			R=0;
		}
	if(R==14){
#if defined(NO_SEGMENT)
		puts("\n<Errfl, Segment Disabled>");
#else
		puts("\n<Errfl, Internal error, Reporting segment disabled but not set that way at compiletime. Report this bug! https://github.com/gek169/Simple_ISA/   >");
#endif
	}
	if(R==15 || R==16 || R==17 || R==18 || R==19){
		puts("\n<Errfl, Privileged opcode executed underprivileged.>");
	}
	return 0;
}
