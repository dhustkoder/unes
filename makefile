CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -pedantic -g -O0 -fsanitize=address

all: %.o
	mkdir -p build
	$(CC) $(CFLAGS) obj/main.o -o build/unes
%.o:
	mkdir -p obj
	$(CC) $(CFLAGS) -c src/main.c -o obj/main.o

clean:
	rm -rf obj/
