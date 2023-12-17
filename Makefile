CC = gcc
CFLAGS = -Wall -Wextra
SOURCES_SERVER = src/server.c src/common.c
SOURCES_USER = src/user.c src/common.c
OBJECTS_SERVER = $(SOURCES_SERVER:src/%.c=src/%.o)
OBJECTS_USER = $(SOURCES_USER:src/%.c=src/%.o)
EXECUTABLE_SERVER = AS
EXECUTABLE_USER = user

all: $(EXECUTABLE_SERVER) $(EXECUTABLE_USER)

$(EXECUTABLE_SERVER): $(OBJECTS_SERVER)
	$(CC) $(OBJECTS_SERVER) -o $(EXECUTABLE_SERVER)

$(EXECUTABLE_USER): $(OBJECTS_USER)
	$(CC) $(OBJECTS_USER) -o $(EXECUTABLE_USER)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(EXECUTABLE_SERVER) $(EXECUTABLE_USER)

.PHONY: all clean