/*Default textmode driver for SISA16.*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const size_t DISK_ACCESS_MASK = 0x3FffFFff;

static unsigned short shouldquit = 0;


#ifndef NO_SIGNAL
#include <signal.h>
#define TRAP_CTRLC signal(SIGINT, emu_respond);
void emu_respond(int bruh){
	(void)bruh;
	shouldquit = 0xffFF;
	return;
}
#else
#define TRAP_CTRLC /*a comment.*/
#endif




#include "isa_pre.h"
/*
	buffers for stdout and stdin.
*/
#define SCREEN_WIDTH_CHARS 80
#define SCREEN_HEIGHT_CHARS 60
#define CHARACTER_WIDTH 8
#define CHARACTER_PXLS (CHARACTER_WIDTH * CHARACTER_WIDTH)
#define SCREEN_BYTES (SCREEN_WIDTH_CHARS*SCREEN_HEIGHT_CHARS*CHARACTER_PXLS)
static unsigned char stdout_buf[(SCREEN_WIDTH_CHARS * SCREEN_HEIGHT_CHARS) + SCREEN_WIDTH_CHARS] = {0};
/*Video memory, 640*480 bytes. Twice as much to prevent bad copies.*/
static unsigned char videomemory[SCREEN_BYTES];
/*1 Megabyte of Audio Memory. Copied to in 64k chunks.*/
static unsigned char audiomemory[0x100000];


/*
	The SDL2 driver keeps a "standard in" buffer.
*/
static unsigned char stdin_buf[(SCREEN_WIDTH_CHARS * 64 * SCREEN_HEIGHT_CHARS)] = {0};
/*
	buffer pointer.
*/
unsigned short stdin_bufptr = 0;

/*
	Cursor position.
*/
UU  curpos = 0;

/*
	SDL2 driver, plus simple text mode.
*/

#define SDL_MAIN_HANDLED


#ifdef __TINYC__
#define SDL_DISABLE_IMMINTRIN_H 1
#endif

#include "font8x8_basic.h"
#include <SDL2/SDL.h>
/*
	TODO- refactor/rewrite av driver, integrate with getchar/putchar to make a "text mode".
*/
static SDL_Window *sdl_win = NULL;
static SDL_Renderer *sdl_rend = NULL;
static SDL_Texture *sdl_tex = NULL;
static SDL_AudioSpec sdl_spec = {0};
static unsigned int display_scale = 1;
static SUU audio_left = 0;
static unsigned char active_audio_user = 0;
static unsigned char FG_color = 15;
static unsigned char BG_color = 0;
static UU SDL_targ[SCREEN_WIDTH_CHARS * SCREEN_HEIGHT_CHARS * CHARACTER_PXLS] = {0}; /*Default to all black.*/
static UU vga_palette[256] = {
#include "vga_pal.h"
};
static void DONT_WANT_TO_INLINE_THIS sdl_audio_callback(void *udata, Uint8 *stream, int len){
	SDL_memset(stream, 0, len);
	if(audio_left == 0){return;}
	len = (len < audio_left) ? len : audio_left;
	
	SDL_MixAudio(stream, audiomemory + (0x100000 - audio_left), len, SDL_MIX_MAXVOLUME);
	audio_left -= len;
}

static void DONT_WANT_TO_INLINE_THIS di(){
		SDL_DisplayMode DM;
	    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	    {
	        printf("SDL2 could not be initialized!\n"
	               "SDL_Error: %s\n", SDL_GetError());
	        exit(1);
	    }
	    SDL_GetCurrentDisplayMode(0, &DM);

	    /*if(DM.w > 1000) display_scale = 2;*/
		sdl_spec.freq = 16000;
		sdl_spec.format = AUDIO_S16MSB;
		sdl_spec.channels = 1;
		sdl_spec.silence = 0;
		sdl_spec.samples = 2048;
		sdl_spec.callback = sdl_audio_callback;
		sdl_spec.userdata = NULL;
		sdl_win = SDL_CreateWindow("[SISA-16]",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			SCREEN_WIDTH_CHARS * CHARACTER_WIDTH * display_scale, 
			SCREEN_HEIGHT_CHARS * CHARACTER_WIDTH * display_scale,
			SDL_WINDOW_SHOWN
		);
		if(!sdl_win)
		{
			printf("SDL2 window creation failed.\n"
				"SDL_Error: %s\n", SDL_GetError());
			exit(1);
		}
		sdl_rend = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
		if(!sdl_rend){
			printf("SDL2 renderer creation failed.\n"
				"SDL_Error: %s\n", SDL_GetError());
			exit(1);
		}
		sdl_tex = SDL_CreateTexture(sdl_rend, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 
			SCREEN_WIDTH_CHARS * CHARACTER_WIDTH,
			SCREEN_HEIGHT_CHARS * CHARACTER_WIDTH
		);
		if(!sdl_tex){
			printf("SDL2 texture creation failed.\n"
				"SDL_Error: %s\n", SDL_GetError());
			exit(1);
		}
		if ( SDL_OpenAudio(&sdl_spec, NULL) < 0 ){
		  printf("\r\nSDL2 audio opening failed!\r\n"
		  "SDL_Error: %s\r\n", SDL_GetError());
		  exit(-1);
		}
		SDL_PauseAudio(0);
		SDL_StartTextInput();
#ifndef SISA_DEBUGGER
		TRAP_CTRLC
#endif
}
static void DONT_WANT_TO_INLINE_THIS dcl(){
		/*
		Delay the destruction of the window for a couple seconds.
		*/
		SDL_Delay(3000);
		SDL_DestroyTexture(sdl_tex);
		SDL_DestroyRenderer(sdl_rend);
		SDL_CloseAudio();
    	SDL_DestroyWindow(sdl_win);
	    SDL_Quit();
}


/*
	IMPLEMENT YOUR CLOCK HERE!!!!
	a must be milliseconds
	b must be seconds
	c is presumed to be some raw measurement of the clock- it can be whatever
*/
#define clock_ins(){\
	size_t q;\
	{\
		STASH_REGS;\
		q=clock();\
		UNSTASH_REGS;\
	}\
	a=((q)/(CLOCKS_PER_SEC/1000));\
	b=q/(CLOCKS_PER_SEC);\
	c=q;\
}

static void pollevents(){
	SDL_Event ev;
	while(SDL_PollEvent(&ev)){
		if(ev.type == SDL_QUIT) shouldquit = 0xFFff; /*Magic value for quit.*/
		else if(ev.type == SDL_TEXTINPUT){
			char* b = ev.text.text;
			while(*b) {stdin_buf[stdin_bufptr++] = *b; b++;}
		}else if(ev.type == SDL_KEYDOWN){
			switch(ev.key.keysym.scancode){
				default: break;
				case SDL_SCANCODE_DELETE: stdin_buf[stdin_bufptr++] = 0x7F; break;
				case SDL_SCANCODE_BACKSPACE: stdin_buf[stdin_bufptr++] = 0x7F;break;
				case SDL_SCANCODE_KP_BACKSPACE: stdin_buf[stdin_bufptr++] = 0x7F;break;
				case SDL_SCANCODE_RETURN: stdin_buf[stdin_bufptr++] = 0xa;break;
				case SDL_SCANCODE_RETURN2: stdin_buf[stdin_bufptr++] = 0xa;break;
				case SDL_SCANCODE_KP_ENTER: stdin_buf[stdin_bufptr++] = 0xa;break;
				case SDL_SCANCODE_ESCAPE: stdin_buf[stdin_bufptr++] = '\e';break;
			}
		}
	}
}
static unsigned short gch(){
	if(stdin_bufptr){
		stdin_bufptr--;
		return stdin_buf[stdin_bufptr];
	} else return 255;
}

static void renderchar(unsigned char* bitmap, UU p) {
	UU x, y, _x, _y;
	UU set;
	/*640/8 = 80, 480/8 = 60*/
	_x = p%SCREEN_WIDTH_CHARS;
	_y = p/SCREEN_WIDTH_CHARS;

	_x *= CHARACTER_WIDTH;
	_y *= CHARACTER_WIDTH;

	for (x = 0; x < CHARACTER_WIDTH; x++) {
		for (y = 0; y < CHARACTER_WIDTH; y++) {
			set = bitmap[x] & (1 << y);
			if (set) SDL_targ[(_x+y) + (_y+x) * (SCREEN_WIDTH_CHARS * CHARACTER_WIDTH)] = vga_palette[FG_color];
		}
	}
}
static void pch(unsigned short a){
	if(a == '\n'){
		curpos += SCREEN_WIDTH_CHARS;
	} else if(a == '\r'){
		while(curpos%SCREEN_WIDTH_CHARS)curpos--;
	} else if(a == 8 || a == 0x7f){
		stdout_buf[curpos-- % (SCREEN_WIDTH_CHARS * SCREEN_HEIGHT_CHARS)] = ' ';
	} else if(a == '\t'){
		do{
			stdout_buf[curpos++ % (SCREEN_WIDTH_CHARS * SCREEN_HEIGHT_CHARS)] = ' ';
		}while(curpos % 4);
	} else {
		stdout_buf[curpos++ % (SCREEN_WIDTH_CHARS * SCREEN_HEIGHT_CHARS)] = a;
	}
	if(curpos>=(SCREEN_WIDTH_CHARS * SCREEN_HEIGHT_CHARS)){
		curpos -= SCREEN_WIDTH_CHARS;
		memcpy(stdout_buf, stdout_buf + SCREEN_WIDTH_CHARS, SCREEN_WIDTH_CHARS * SCREEN_HEIGHT_CHARS);
		memset(stdout_buf+(SCREEN_WIDTH_CHARS- 1)*SCREEN_HEIGHT_CHARS, ' ', SCREEN_WIDTH_CHARS);
	}
	/*putchar_unlocked(a);*/ /*for those poor terminal users at home.*/
}







static unsigned short DONT_WANT_TO_INLINE_THIS interrupt(unsigned short a,
									unsigned short b,
									unsigned short c,
									unsigned short stack_pointer,
									unsigned short program_counter,
									unsigned char program_counter_region,
									UU RX0,
									UU RX1,
									UU RX2,
									UU RX3,
									u* M
								)
{
	if(a == 0x80) return 0x80; /*Ignore 80- it is reserved for system calls!*/
	if(a == 0) return 0;
	if(a == '\n'){ /*magic values to display the screen.*/ /*TODO remove backslash r*/
		UU i = 0;
		SDL_Rect screenrect;
		SDL_Rect screenrect2;
		/*
			B holds the process whose memory we want to use.
		*/
		b %= (SISA_MAX_TASKS+1);
		screenrect.x = 0;
		screenrect.y = 0;
		screenrect.w = CHARACTER_WIDTH * SCREEN_WIDTH_CHARS;
		screenrect.h = CHARACTER_WIDTH * SCREEN_HEIGHT_CHARS;
		screenrect2 = screenrect;
		screenrect2.w *= display_scale;
		screenrect2.h *= display_scale;
		for(i=0;i<(CHARACTER_PXLS * SCREEN_WIDTH_CHARS * SCREEN_HEIGHT_CHARS);i++){
			unsigned char val = videomemory[i];
			SDL_targ[i] = vga_palette[val];
		}
		for(i=0;i<(SCREEN_WIDTH_CHARS * SCREEN_HEIGHT_CHARS);i++){
			if(stdout_buf[i] && stdout_buf[i] != ' ' && isprint(stdout_buf[i]))
				renderchar(font8x8_basic[stdout_buf[i]], i);
		}
		SDL_UpdateTexture(
			sdl_tex,
			NULL,
			SDL_targ, 
			(SCREEN_WIDTH_CHARS*CHARACTER_WIDTH) * 4
		);
		SDL_RenderCopy(
			sdl_rend, 
			sdl_tex,
			&screenrect,
			&screenrect2
		);
		SDL_RenderPresent(sdl_rend);
		return a;
	}
	if(a == 1){ /*Poll events.*/
		pollevents();
		return shouldquit;
	}
	if(a == 2){ /*Read gamer buttons!!!!*/
		unsigned short retval = 0;
		const unsigned char *state;
		SDL_PumpEvents();
		state = SDL_GetKeyboardState(NULL);
		retval |= 0x1 * (state[SDL_SCANCODE_UP]!=0);
		retval |= 0x2 * (state[SDL_SCANCODE_DOWN]!=0);
		retval |= 0x4 * (state[SDL_SCANCODE_LEFT]!=0);
		retval |= 0x8 * (state[SDL_SCANCODE_RIGHT]!=0);

		retval |= 0x10 * (state[SDL_SCANCODE_RETURN]!=0);
		retval |= 0x20 * (state[SDL_SCANCODE_RSHIFT]!=0);
		retval |= 0x40 * (state[SDL_SCANCODE_Z]!=0);
		retval |= 0x80 * (state[SDL_SCANCODE_X]!=0);

		retval |= 0x100 * (state[SDL_SCANCODE_C]!=0);
		retval |= 0x200 * (state[SDL_SCANCODE_A]!=0);
		retval |= 0x400 * (state[SDL_SCANCODE_S]!=0);
		retval |= 0x800 * (state[SDL_SCANCODE_D]!=0);
		return retval;
	}
	/*TODO: play samples from a buffer.*/
	if(a == 3){
		audio_left = 0x100000;
		return 1;
	}
	/*kill the audio.*/
	if(a == 4){
		audio_left = 0;
		return 1;
	}

	/*DEVICE MEMORY COMMANDS*/
	if(a == 0x400){ /*Copy user memory region to audio memory.*/
		/*audiomemory[RX0] = M[RX1], copy 64k*/
		memcpy(
		audiomemory+(b & 0xf),
		M_SAVER[active_audio_user] + ((c & 0xff)*0x10000),
		0x10000
		);
		return a;
	}
	if(a == 0x500){ /*Fill rect with solid value.*/
		UU i;
		/*fill rect x=RX0, y=RX1, w=RX2, h=RX3 with byte b*/
		if(RX0 > SCREEN_WIDTH_CHARS*CHARACTER_WIDTH) return 0;
		if(RX1 > SCREEN_HEIGHT_CHARS*CHARACTER_WIDTH) return 0;
		/*We have something to render!*/
		if(  (RX0+RX2) > (SCREEN_WIDTH_CHARS*CHARACTER_WIDTH)  )
		{
			RX2 = (SCREEN_WIDTH_CHARS*CHARACTER_WIDTH) - RX0;
		}
		if(  (RX1+RX3) > (SCREEN_HEIGHT_CHARS*CHARACTER_WIDTH)  )
		{
			RX3 = (SCREEN_HEIGHT_CHARS*CHARACTER_WIDTH) - RX1;
		}
		for(i = RX1; i < RX3; i++){
			memset(
				videomemory + /*Videomemory...*/
				RX0 + /*Where on the line?*/
				/*What line?*/
				i * (SCREEN_WIDTH_CHARS * CHARACTER_WIDTH),
				b, /*What byte?*/
				RX2 /*How many bytes*/
			);
		}
		return 1;
	}

	/*Get screen width and height*/
	if(a == 0x502) return (SCREEN_WIDTH_CHARS * CHARACTER_WIDTH);
	if(a == 0x503) return (SCREEN_HEIGHT_CHARS * CHARACTER_WIDTH);
	if(a == 0x501){/*TODO video command, write pixels to output buffer*/
		UU loc = RX0; /*Location in the AV user's memory.*/
		UU n_pixels = RX1;
		UU position = RX2;
		/*Invalid call!*/
		if(loc > 0x1000000) return a;
		if(position > sizeof(videomemory)) return a;
		/*Copy fewer pixels if this is true...*/
		if(n_pixels > sizeof(videomemory)) n_pixels = sizeof(videomemory);
		
		if((loc + n_pixels) > 0x1000000) return a;
		if( (position + n_pixels) > sizeof(videomemory)) return a;
		memcpy(
			videomemory + position,
			M_SAVER[active_audio_user] + loc,
			n_pixels
		);
		
		return 1;
	}
	if(a == 5){
		FG_color = b;
		return 1;
	}
	if(a == 7){ /*They want to set the active user for audio.*/
		active_audio_user = b % (SISA_MAX_TASKS+1);
	}
	if(a == 0x17){
		return active_audio_user;
	}
	if(a == 8){
		vga_palette[b&0xff] = RX0 & 0xFFffFF;
	}
	if(a == 0xc){
		memset(stdout_buf, 0, SCREEN_WIDTH_CHARS * SCREEN_HEIGHT_CHARS);
		curpos = 0;
		return a;
	}

	if(a==0xE000){
		return 1;
	}
	if(a==0xE001){
		return 0;
	}
	if(a == 0xffFF){ /*Perform a memory dump.*/
		unsigned long i,j;
		for(i=0;i<(1<<24)-31;i+=32)
			for(j=i,printf("%s\r\n%06lx|",(i&255)?"":"\r\n~",i);j<i+32;j++)
					printf("%02x%c",M[j],((j+1)%8)?' ':'|');
		return a;
	}
	if(a == 0xFF10){ /*Read 256 bytes from saved disk to page b*/
		size_t location_on_disk = ((size_t)RX0) << 8;
		FILE* f = fopen("sisa16.dsk", "rb+");
		location_on_disk &= DISK_ACCESS_MASK;
		if(!f){
			UU i = 0;
			for(i = 0; i < 256; i++){
				M[((UU)b<<8) + i] = 0;
			}
			return 0;
		}
		if(fseek(f, location_on_disk, SEEK_SET)){
			UU i = 0;
			for(i = 0; i < 256; i++){
				M[((UU)b<<8) + i] = 0;
			}
			return 0;
		}
		{
			UU i = 0;
			for(i = 0; i < 256; i++){
				M[((UU)b<<8) + i] = fgetc(f);
			}
		}
		fclose(f);
		return 1;
	}

	if(a == 0xFF11){ /*write 256 bytes from page 'b' to saved disk.*/
		size_t location_on_disk = ((size_t)RX0) << 8;
		FILE* f = fopen("sisa16.dsk", "rb+");
		location_on_disk &= DISK_ACCESS_MASK;
		if(!f){
			f = fopen("sisa16.dsk", "wb");
			if(!f) return 0;
			while((unsigned long)ftell(f) < (unsigned long)location_on_disk) if(EOF == fputc(0, f)) return 0;
			fflush(f);
			fclose(f);
			f = fopen("sisa16.dsk", "rb+");
			if(!f) return 0;
		}
		if(fseek(f, location_on_disk, SEEK_SET)){
			fseek(f, 0, SEEK_END);
			while((unsigned long)ftell(f) < (unsigned long)location_on_disk) if(EOF == fputc(0, f)) return 0;
		}
		{
			UU i = 0;
			for(i = 0; i < 256; i++){
				fputc(M[((UU)b<<8) + i], f);
			}
		}
		fclose(f);
		return 1;
	}
	return a;
}
