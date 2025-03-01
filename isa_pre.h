#include <string.h>

typedef unsigned char u;
typedef unsigned short U;
#if defined(USE_UNSIGNED_INT)
typedef unsigned int UU;
typedef signed int SUU;
#else
typedef unsigned long UU;
typedef long SUU;
#endif
static u R=0;

/*
	MUST BE POWER OF TWO!!!!
	that means the lead digit must be 1, 2, 4, or 8, followed entirely by zeroes.

	32 megabytes are allocated for the segment.
*/
#define SEGMENT_PAGES 0x20000

/*This is important later!!! Only two privilege levels.
0:		Kernel mode
1:		User mode

User mode is pre-empted.
*/


/*Other options.
#define NO_PREEMPT
#define NO_DEVICE_PRIVILEGE
*/


/*
	HUGE NOTE:
	you should alter krenel.hasm if you change this constant!!!
	the libc is designed specifically to work with this!
*/
#ifndef SISA_MAX_TASKS
#define SISA_MAX_TASKS 16
#endif



typedef struct {
	UU gp[64];
	UU RX0,RX1,RX2,RX3;
#ifndef NO_PREEMPT
	UU instruction_counter;
#endif
	U a,b,c,program_counter,stack_pointer;
	u program_counter_region;
}sisa_regfile;
static u M_SAVER[1+SISA_MAX_TASKS][0x1000000] = {0};
static u SEGMENT[SEGMENT_PAGES * 256];
#define SAVE_REGISTER(XX, d) REG_SAVER[d].XX = XX;
#define LOAD_REGISTER(XX, d) XX = REG_SAVER[d].XX;
#define SAVE_GP(d) {memcpy(REG_SAVER[d].gp,  gp, 64*4);}
#define LOAD_GP(d) {memcpy(gp, REG_SAVER[d]. gp, 64*4);}
#ifdef ATTRIB_NOINLINE
#define DONT_WANT_TO_INLINE_THIS __attribute__ ((noinline))
#else
#define DONT_WANT_TO_INLINE_THIS /*A comment.*/
#endif

