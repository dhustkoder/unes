CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -pedantic -g -O0 -fsanitize=address
BUILD_DIR=./build
OBJS_DIR=./objs
SRC_DIR=./src


$(BUILD_DIR)/unes: $(OBJS_DIR)/main.o
	mkdir -p $(BUILD_DIR)
	$(CC) -o $@ $< $(CFLAGS)

$(OBJS_DIR)/main.o: $(SRC_DIR)/main.c
	mkdir -p $(OBJS_DIR)	
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf $(OBJS_DIR)/


