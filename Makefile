CC = gcc
CFLAGS = -Wall -Wextra
SOURCES_SERVER = src/server.c src/communication.c
SOURCES_USER = src/user.c src/communication.c
OBJECTS_SERVER = $(SOURCES_SERVER:src/%.c=%.o)
OBJECTS_USER = $(SOURCES_USER:src/%.c=%.o)
EXECUTABLE_SERVER = server
EXECUTABLE_USER = user

all: $(EXECUTABLE_SERVER) $(EXECUTABLE_USER)

$(EXECUTABLE_SERVER): $(OBJECTS_SERVER)
	$(CC) $(OBJECTS_SERVER) -o $(EXECUTABLE_SERVER)

$(EXECUTABLE_USER): $(OBJECTS_USER)
	$(CC) $(OBJECTS_USER) -o $(EXECUTABLE_USER)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXECUTABLE_SERVER) $(EXECUTABLE_USER)

.PHONY: all clean