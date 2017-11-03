CFLAGS = -std=c89 `pkg-config --cflags gtk+-3.0`
LFLAGS = `pkg-config --libs gtk+-3.0`
CC = gcc
NAME = master_pass

debug:
	$(CC) -o $(NAME) $(CFLAGS) -g *.c $(LFLAGS)

release:
	$(CC) -o $(NAME) $(CFLAGS) -O2 -flto -DNDEBUG *.c $(LFLAGS)
