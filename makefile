CFLAGS := -O3 -flto -std=c89 -pedantic `pkg-config --cflags gtk+-3.0`
LFLAGS = `pkg-config --libs gtk+-3.0`
NAME := master_pass
DIR := build
CFILES := $(wildcard *.c)
OBJ := $(CFILES:%.c=$(DIR)/%.o) $(DIR)/asm.o
DEPS := $(OBJ:%.o=%.d)

release: $(DIR)/$(NAME)

$(DIR)/$(NAME): $(OBJ)
	mkdir -p $(DIR)
	gcc $(CFLAGS) -o $@ $^ $(LFLAGS)

-include $(DEPS)

$(DIR)/asm.o: asm.asm
	nasm -felf64 -o $(DIR)/asm.o asm.asm

$(DIR)/%.o: %.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -MMD -MF $(patsubst %.o,%.d,$@) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf $(DIR)