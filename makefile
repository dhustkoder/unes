CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -pedantic
CFLAGS_DEBUG=-g -O0 -fsanitize=address
CFLAGS_RELEASE=-O3 -flto
BUILD_DIR=./build
OBJS_DIR=./objs
SRC_DIR=./src

all: CFLAGS += $(CFLAGS_RELEASE)
all: $(BUILD_DIR)/unes


debug: CFLAGS += $(CFLAGS_DEBUG)
debug: $(BUILD_DIR)/unes-d


## release build
$(BUILD_DIR)/unes: $(OBJS_DIR)/main.o
	mkdir -p $(BUILD_DIR)
	$(CC) -o $@ $< $(CFLAGS)

$(OBJS_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/rom.h
	mkdir -p $(OBJS_DIR)	
	$(CC) -c -o $@ $< $(CFLAGS)


## debug build
$(BUILD_DIR)/unes-d: $(OBJS_DIR)/main-d.o
	mkdir -p $(BUILD_DIR)
	$(CC) -o $@ $< $(CFLAGS)


$(OBJS_DIR)/main-d.o: $(SRC_DIR)/main.c $(SRC_DIR)/rom.h
	mkdir -p $(OBJS_DIR)
	$(CC) -c -o $@ $< $(CFLAGS)


clean:
	rm -rf $(OBJS_DIR)/


