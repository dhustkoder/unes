ifeq ($(DEVKITPPC),)
$(error "Set DEVKITPPC and DEVKITPRO enviroment variables")
endif
include $(DEVKITPPC)/gamecube_rules

LD=$(CC)

CFLAGS=-std=gnu11 -Wall -Wextra -Wshadow -Wno-main -ffreestanding \
       -I $(SRC_DIR) -I $(SRC_DIR)/GC                             \
       $(MACHDEP) $(INCLUDE)  -I$(LIBOGC_INC) -DPLATFORM_GC

CFLAGS_DEBUG=-Og -DDEBUG

CFLAGS_RELEASE=-Werror -O3 -ffast-math -fstrict-aliasing \
	       -ffunction-sections -fdata-sections -fno-unwind-tables  \
	       -fno-asynchronous-unwind-tables -DNDEBUG

LDFLAGS=$(MACHDEP) -L$(LIBOGC_LIB) -logc
LDFLAGS_DEBUG=-g
LDFLAGS_RELEASE=-s -Wl,--gc-sections

BUILD_DIR=./build
OBJS_DIR=./objs
ASM_DIR=./asm
SRC_DIR=./src
PLATFORM_OBJS_DIR=$(OBJS_DIR)/GC
PLATFORM_SRC_DIR=$(SRC_DIR)/GC
PLATFORM_ASM_DIR=$(ASM_DIR)/GC

SRC=$(SRC_DIR)/%.c
PLATFORM_SRC=$(PLATFORM_SRC_DIR)/%.c

OBJS=$(patsubst $(SRC_DIR)/%.c, $(OBJS_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))
ASM=$(patsubst $(SRC_DIR)/%.c, $(ASM_DIR)/%.asm, $(wildcard $(SRC_DIR)/*.c))
PLATFORM_OBJS=$(patsubst $(PLATFORM_SRC_DIR)/%.c, $(PLATFORM_OBJS_DIR)/%.o, $(wildcard $(PLATFORM_SRC_DIR)/*.c))
PLATFORM_ASM=$(patsubst $(PLATFORM_SRC_DIR)/%.c, $(PLATFORM_ASM_DIR)/%.asm, $(wildcard $(PLATFORM_SRC_DIR)/*.c))

$(shell mkdir -p $(BUILD_DIR))

ifeq ($(BUILD_TYPE),)
	BUILD_TYPE :=Debug
endif

ifeq ($(BUILD_TYPE),Release)
	CFLAGS += $(CFLAGS_RELEASE)
	LDFLAGS += $(LDFLAGS_RELEASE)
else ifeq ($(BUILD_TYPE),Debug)
	CFLAGS += $(CFLAGS_DEBUG)
	LDFLAGS += $(LDFLAGS_DEBUG)
else
	$(error "Unknown BUILD_TYPE")
endif

ifeq ($(ENABLE_LTO),ON)
	CFLAGS += -flto
endif


.PHONY: all clean asm


all: $(BUILD_DIR)/unes.dol
asm: $(ASM) $(PLATFORM_ASM)


$(BUILD_DIR)/unes.dol: $(BUILD_DIR)/unes.elf
$(BUILD_DIR)/unes.elf: $(OBJS) $(PLATFORM_OBJS)

$(OBJS_DIR)/%.o: $(SRC)
	@mkdir -p $(PLATFORM_OBJS_DIR)
	$(CC) $(CFLAGS) -MP -MD -c $< -o $@

$(ASM_DIR)/%.asm: $(SRC)
	@mkdir -p $(PLATFORM_ASM_DIR)
	$(CC) $(CFLAGS) -S $< -o $@


-include $(shell ls $(OBJS_DIR)/*.d 2>/dev/null)

clean:
	rm -rf $(ASM_DIR)/
	rm -rf $(OBJS_DIR)/

