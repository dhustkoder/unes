CFLAGS=-std=c99 -pedantic-errors -Wall -Wextra -Wshadow \
       -I ./src -I ./src/sdl2 $(shell sdl2-config --cflags) \
       -DPLATFORM_SDL2

CFLAGS_DEBUG=-g -O0 -fsanitize=address -DDEBUG

CFLAGS_RELEASE=-Werror -O3 -march=native -ffast-math -fstrict-aliasing \
	       -ffunction-sections -fdata-sections -fno-unwind-tables  \
	       -fno-asynchronous-unwind-tables -DNDEBUG

CFLAGS_PERF=-g -O3 -fno-omit-frame-pointer

LDFLAGS=$(shell sdl2-config --libs)
LDFLAGS_DEBUG=-g
LDFLAGS_RELEASE=
LDFLAGS_PERF=-g

BUILD_DIR=./build

SRC=./src/sdl2/cunits.c


ifeq ($(CC),)
	CC:=gcc
endif

ifeq ($(BUILD_TYPE),)
	BUILD_TYPE=Debug
endif

ifeq ($(BUILD_TYPE),Release)
	CFLAGS += $(CFLAGS_RELEASE)
	LDFLAGS += $(LDFLAGS_RELEASE)
else ifeq ($(BUILD_TYPE),Perf)
	CFLAGS += $(CFLAGS_PERF)
	LDFLAGS += $(LDFLAGS_PERF)
else ifeq ($(BUILD_TYPE),Debug)
	CFLAGS += $(CFLAGS_DEBUG)
	LDFLAGS += $(LDFLAGS_DEBUG)
else
$(error "Unknown BUILD_TYPE")
endif

ifeq ($(ENABLE_LTO),ON)
	CFLAGS += -flto
endif

ifeq ($(UNES_DEBUGGER), ON)
	CFLAGS += -DUNES_DEBUGGER
endif


.PHONY: all


all: $(BUILD_DIR)/unes


$(BUILD_DIR)/unes: $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)


