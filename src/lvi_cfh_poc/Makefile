
CFLAGS :=

LDFLAGS :=
LDFLAGS += -lpthread

all:
	nasm -f elf64 -o asm.o asmhelper.asm
	gcc $(CFLAGS) -o lvi asm.o lvi_cfh_poc.c $(LDFLAGS)
