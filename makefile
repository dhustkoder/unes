CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -pedantic -g -O0 -fsanitize=address
all:
	mkdir -p build/
	$(CC) $(CFLAGS) src/main.c -o build/unes
