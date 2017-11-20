CFLAGS = -std=c89 `pkg-config --cflags gtk+-3.0`
LFLAGS = `pkg-config --libs gtk+-3.0`
CC = gcc
NAME = master_pass

debug: asm
	$(CC) -o $(NAME) $(CFLAGS) -g *.c asm.o $(LFLAGS)
	rm asm.o

release: asm
	$(CC) -o $(NAME) $(CFLAGS) -O2 -flto -DNDEBUG *.c asm.o $(LFLAGS)
	rm asm.o

asm: asm.asm
	nasm -felf64 -o asm.o asm.asm
