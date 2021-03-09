all:
	nasm -f elf64 -o asm.o asmhelper.asm
	gcc -o lvi asm.o lvi_cfh_poc.c -lpthread
