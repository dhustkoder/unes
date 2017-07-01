CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -pedantic
CFLAGS_DEBUG=-g -O0 -fsanitize=address
CFLAGS_RELEASE=-O3 -flto
BUILD_DIR=build
OBJS_DIR=objs
SRC_DIR=src


all: CFLAGS += $(CFLAGS_RELEASE)
all: $(BUILD_DIR)/unes


$(BUILD_DIR)/unes: $(patsubst $(SRC_DIR)/%.c, $(OBJS_DIR)/%.o,$(shell ls $(SRC_DIR)/*.c))
	@mkdir -p $(BUILD_DIR) 
	$(CC) $(CFLAGS) $(wildcard $(OBJS_DIR)/*.o) -o $@

$(OBJS_DIR)/%.o: $(wildcard $(SRC_DIR)/*.c)
	@mkdir -p $(OBJS_DIR)
	$(CC) $(CFLAGS) -MP -MD -MD -c $(SRC_DIR)/$*.c -o $(OBJS_DIR)/$*.o

-include $(shell ls $(OBJS_DIR)/*.d 2>/dev/null)

clean:
	rm -rf $(OBJS_DIR)/


