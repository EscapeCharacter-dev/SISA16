CC = gcc
#CC = tcc

INSTALL_DIR     = /usr/bin
MAN_INSTALL_DIR = /usr/share/man/man1

GIT_HASH = $(shell git rev-parse > /dev/null 2>&1 && git rev-parse --short HEAD || echo no)
OPTLEVEL    = -Os -s -march=native -DSISA_GIT_HASH=\"$(GIT_HASH)\" 
#" dont remove this it fixes syntax highlighting in my editor

#-DUSE_COMPUTED_GOTO

#MSYS2_BUILD = yes

#the following lines are used for MSYS2 builds with SDL2
ifdef MSYS2_BUILD
MORECFLAGS  =  -DUSE_UNSIGNED_INT -DATTRIB_NOINLINE -DUSE_AV -lSDL2 -std=c99  -mwindows -DUSE_TERMIOS -DUSE_COMPUTED_GOTO 
#-DNO_INSN_WEIGHTS
STATIC =  #-static
INSTALL_DIR     = /usr/bin
LIB_INSTALL_DIR = C:/SISA16/
MAN_INSTALL_DIR = /usr/share/man/man1
else

#the following lines are used for WSL2 builds
MORECFLAGS  =  -DUSE_UNSIGNED_INT -DATTRIB_NOINLINE -DUSE_TERMIOS -DUSE_COMPUTED_GOTO 
#-DNO_INSN_WEIGHTS
STATIC =  -static
INSTALL_DIR     = /usr/bin
LIB_INSTALL_DIR = /usr/include/sisa16/
MAN_INSTALL_DIR = /usr/share/man/man1


endif


CFLAGS_NOERR = -Wno-pointer-sign -Wno-format-security
#-pedantic
CFLAGS      = $(MORECFLAGS) $(OPTLEVEL) $(CFLAGS_NOERR)

# ------=========------ COLORS ------=========------ #
ifndef NO_COLORS
COLOR_RED     = "\033[0;31m"
COLOR_GREEN   = "\033[0;32m"
COLOR_YELLOW  = "\033[0;33m"
COLOR_BLUE    = "\033[0;34m"
COLOR_PURPLE  = "\033[0;35m"
COLOR_CYAN    = "\033[0;36m"
# TODO: less repetitive
COLOR_BRED    = "\033[1;31m"
COLOR_BGREEN  = "\033[1;32m"
COLOR_BYELLOW = "\033[1;33m"
COLOR_BBLUE   = "\033[1;34m"
COLOR_BPURPLE = "\033[1;35m"
COLOR_BCYAN   = "\033[1;36m"

COLOR_RESET   = "\033[0;0m"
endif

all: main
	@printf "%b%s%b\n" $(COLOR_BCYAN) "~~Done building." $(COLOR_RESET)

#all_sdl2: main_sdl2 #asm_programs
#	@printf "%b%s%b\n" $(COLOR_BCYAN) "~~Done building with SDL2." $(COLOR_RESET)

sisa16_emu:
	$(CC) isa.c -o sisa16_emu  $(CFLAGS) $(STATIC) 
	@printf "%b%s%b\n" $(COLOR_BCYAN) "~~Built emulator." $(COLOR_RESET)

sisa16_asm:
	$(CC) assembler.c -o sisa16_asm  $(CFLAGS) $(STATIC) 
	@printf "%b%s%b\n" $(COLOR_BCYAN) "~~Built assembler (Which has an emulator built into it.)" $(COLOR_RESET)

sisa16_dbg:
	$(CC) debugger.c -o sisa16_dbg $(CFLAGS) $(STATIC) 
	@printf "%b%s%b\n" $(COLOR_BCYAN) "~~Built debugger." $(COLOR_RESET)

# SDL2 versions, if you want to mess with graphics and audio


main: sisa16_asm sisa16_emu
#main_sdl2: sisa16_sdl2_asm sisa16_sdl2_emu sisa16_sdl2_dbg

asm: sisa16_asm
	./asm_compile.sh

#used by github actions.
check:
#effectively, a check.
	sudo $(MAKE) -B libc
	sudo $(MAKE) clean
	./asmbuild.sh
	sisa16_asm -C
	sisa16_asm -v
	sisa16_asm -dis ./s16_1/clock.bin 0x10000
	sisa16_asm -dis ./s16_1/clock.bin 0x20000
	sisa16_asm -fdis ./s16_1/echo.bin 0
	sisa16_asm -dis ./s16_1/echo2.bin 0x20000
	sisa16_asm -dis ./s16_1/program2.bin 0x770000
	sisa16_asm -fdis ./s16_1/switchcase.bin 0
	sisa16_asm -fdis ./s16_1/controlflow_1.bin 0
	
install: sisa16_asm sisa16_emu sisa16_dbg
	@cp ./sisa16_emu $(INSTALL_DIR)/ || cp ./sisa16_emu.exe $(INSTALL_DIR)/ || echo "ERROR!!! Cannot install sisa16_emu"
	@cp ./sisa16_dbg $(INSTALL_DIR)/ || cp ./sisa16_dbg.exe $(INSTALL_DIR)/ || echo "ERROR!!! Cannot install sisa16_dbg"
	@cp ./sisa16_asm $(INSTALL_DIR)/ || cp ./sisa16_asm.exe $(INSTALL_DIR)/ || echo "ERROR!!! Cannot install sisa16_asm"
	@mkdir /usr/include/sisa16/ || echo "sisa16 include directory either already exists or cannot be created."
	@cp ./s16_1/*.hasm $(LIB_INSTALL_DIR) || echo "ERROR!!! Cannot install libc.hasm. It is the main utility library for sisa16"
	@cp ./*.1 $(MAN_INSTALL_DIR)/ || echo "Could not install manpages."

libc: install
	cd ./s16_1/ && sisa16_asm -i libc.asm -o libc.bin

	mkdir $(LIB_INSTALL_DIR)/ || echo "sisa16 include directory either already exists or cannot be created."
	cp ./s16_1/*.hasm $(LIB_INSTALL_DIR)/ && echo "Installed libraries." || echo "ERROR!!! Cannot install headers"
	cp ./s16_1/libc.bin.s16sym $(LIB_INSTALL_DIR)/libc_pre.hasm && echo "Installed libraries." || echo "ERROR!!! Cannot install libc_pre.hasm. It is the main utility library for sisa16"
	cp ./s16_1/libc.bin $(LIB_INSTALL_DIR)/libc_pre.bin && echo "Installed libraries." || echo "ERROR!!! Cannot install libc_pre.bin It is the main utility library for sisa16"
	
uninstall:
	rm -f $(INSTALL_DIR)/sisa16*
	rm -f $(MAN_INSTALL_DIR)/sisa16_emu.1
	rm -f $(MAN_INSTALL_DIR)/sisa16_dbg.1
	rm -f $(MAN_INSTALL_DIR)/sisa16_asm.1
	@echo "Uninstalled."
	@echo "Note that if you have libraries under $(LIB_INSTALL_DIR), they were *not* removed."

clean:
	rm -f *.exe *.dbg *.out *.dsk *.o *.bin *.tmp *.s16sym sisa16_emu sisa16_asm sisa16_dbg sisa16_sdl2_emu sisa16_sdl2_asm sisa16_sdl2_dbg s16asm2
	rm -f s16_1/*.exe s16_1/*.dbg s16_1/*.out s16_1/*.tmp s16_1/*.bin s16_1/*.dsk s16_1/*.s16sym
# clear || echo "cannot clear?"



q:
	admin $(MAKE) -B libc
	admin $(MAKE) clean
	git add .
	git commit -m "Developer time" || echo "nothing to commit"
	git push --force || echo "nothing to push"
	
w:
	$(MAKE) -B libc
	$(MAKE) clean
	#git add .
	#git commit -m "Developer Time on MSYS2" || echo "nothing to commit"
	#git push --force || echo "nothing to push"

d:
	admin $(MAKE) -B libc
	admin $(MAKE) clean


