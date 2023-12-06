CC = gcc
DEBUG_FLAGS = -g -DDEBUG=1
CFLAGS = -Wall -Wextra
TARGET = ./src/Cache.elf
INCLUDE_DIR = ../lib/include


INCLUDE_HEADER_DEPS := $(shell find $(INCLUDE_DIR) -type f -name '*.h')
HEADER_DEPS := $(shell find ./src -type f -name '*.h')
CFILES := $(shell find ./src -type f -name '*.c')
OBJ := $(CFILES:.c=.o)


%.o: %.c $(INCLUDE_HEADER_DEPS) $(HEADER_DEPS) Makefile
	$(CC) $(CFLAGS) -c -I $(INCLUDE_DIR) $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)


.PHONY: all
all: $(TARGET)

.PHONY: run
run: $(TARGET)
	./$(TARGET)

.PHONY: clean
clean:
	rm -f $(TARGET) ./src/*.o ./src/*.elf
