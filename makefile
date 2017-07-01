CC:=$(if $(CC),$(CC),gcc)
CFLAGS=-std=c11 -Wall -Wextra -pedantic
CFLAGS_DEBUG=-g -O0 -fsanitize=address
CFLAGS_RELEASE=-O3
BUILD_DIR=./build
OBJS_DIR=./objs
SRC_DIR=./src
SRC=$(SRC_DIR)/%.c
OBJS=$(patsubst $(SRC_DIR)/%.c, $(OBJS_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))


.PHONY: all clean


all: CFLAGS += $(CFLAGS_RELEASE)
all: $(BUILD_DIR)/unes


$(BUILD_DIR)/unes: $(OBJS)
	@mkdir -p $(BUILD_DIR) 
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(OBJS_DIR)/%.o: $(SRC)
	@mkdir -p $(OBJS_DIR)
	$(CC) $(CFLAGS) -MP -MD -c $(SRC_DIR)/$*.c -o $(OBJS_DIR)/$*.o


-include $(shell ls $(OBJS_DIR)/*.d 2>/dev/null)


clean:
	rm -rf $(OBJS_DIR)/

