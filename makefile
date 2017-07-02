CC:=$(if $(CC),$(CC),gcc)
CFLAGS=-std=c11 -Wall -Wextra -pedantic
CFLAGS_DEBUG=-g -O0 -fsanitize=address
CFLAGS_RELEASE=-O3
BUILD_DIR=./build
OBJS_DIR=./objs
ASM_DIR=./asm
SRC_DIR=./src
SRC=$(SRC_DIR)/%.c
OBJS=$(patsubst $(SRC_DIR)/%.c, $(OBJS_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))
ASM=$(patsubst $(SRC_DIR)/%.c, $(ASM_DIR)/%.asm, $(wildcard $(SRC_DIR)/*.c))


ifeq ($(BUILD_TYPE),Release)
	CFLAGS += $(CFLAGS_RELEASE)
else
	CFLAGS += $(CFLAGS_DEBUG)
endif

ifeq ($(ENABLE_LTO),ON)
	CFLAGS += -flto
endif


.PHONY: all clean asm


all: $(BUILD_DIR)/unes
asm: $(ASM)


$(BUILD_DIR)/unes: $(OBJS)
	@mkdir -p $(BUILD_DIR) 
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(OBJS_DIR)/%.o: $(SRC)
	@mkdir -p $(OBJS_DIR)
	$(CC) $(CFLAGS) -MP -MD -c $(SRC_DIR)/$*.c -o $(OBJS_DIR)/$*.o


$(ASM_DIR)/%.asm: $(SRC)
	@mkdir -p $(ASM_DIR)
	$(CC) $(CFLAGS) -S $(SRC_DIR)/$*.c -o $(ASM_DIR)/$*.asm

-include $(shell ls $(OBJS_DIR)/*.d 2>/dev/null)


clean:
	rm -rf $(ASM_DIR)/
	rm -rf $(OBJS_DIR)/

