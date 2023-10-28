CC=oscar64
EMU=xvic.exe
RM=del
CFLAGS=-tm=vic20 -dOSCAR64

all: baguel.prg

%.prg: %.c
	$(CC) $(CFLAGS) $<
	
clean:
	$(RM) bin\*.asm bin\*.int bin\*.lbl bin\*.map bin\*.prg bin\*.bcs bin\*.d64 bin\*.crt	
	
baguel.prg: main.c
	$(CC) $(CFLAGS) $< -n main.c -Oz -d64=bin/baguel.d64 -f=bin/r01.prg -f=bin/r02.prg -f=bin/r03.prg 
	
run:
	$(EMU) bin/baguel.d64
	
; -f=bin/r11.prg -f=bin/r12.prg -f=bin/r13.prg -f=bin/r14.prg -f=bin/r15.prg -f=bin/r16.prg -f=bin/r17.prg -f=bin/r18.prg