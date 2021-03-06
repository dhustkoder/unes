EE_BIN = build/unes.elf
EE_SRC  = $(wildcard src/ps2/*.c) \
          $(wildcard src/*.c)
EE_OBJS = $(EE_SRC:.c=.o)
EE_LIBS = -lgskit -ldmakit -laudsrv
EE_INCS = -I./src -I./src/ps2 -I$(PS2DEV)/gsKit/include
EE_CFLAGS = -std=c99 -DPLATFORM_PS2SDK
EE_LDFLAGS = -L$(PS2SDK)/ports/lib -L$(PS2DEV)/gsKit/lib


all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

reset:
	ps2client reset




# Makefile.pref

EE_PREFIX ?= ee-
EE_CC = $(EE_PREFIX)gcc
EE_CXX= $(EE_PREFIX)g++
EE_AS = $(EE_PREFIX)as
EE_LD = $(EE_PREFIX)ld
EE_AR = $(EE_PREFIX)ar
EE_OBJCOPY = $(EE_PREFIX)objcopy
EE_STRIP = $(EE_PREFIX)strip

IOP_PREFIX ?= iop-
IOP_CC = $(IOP_PREFIX)gcc
IOP_AS = $(IOP_PREFIX)as
IOP_LD = $(IOP_PREFIX)ld
IOP_AR = $(IOP_PREFIX)ar
IOP_OBJCOPY = $(IOP_PREFIX)objcopy
IOP_STRIP = $(IOP_PREFIX)strip





# Makefile.eeglobal

EE_CC_VERSION := $(shell $(EE_CC) -dumpversion)

# Include directories
EE_INCS := $(EE_INCS) -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include -I. 

# C compiler flags
EE_CFLAGS := -D_EE -O3 -G0 -Wall $(EE_CFLAGS)

# C++ compiler flags
EE_CXXFLAGS := -D_EE -O2 -G0 -Wall $(EE_CXXFLAGS)

# Linker flags
EE_LDFLAGS := -L$(PS2SDK)/ee/lib $(EE_LDFLAGS)

# Assembler flags
EE_ASFLAGS := -G0 $(EE_ASFLAGS)

# Link with following libraries.  This is a special case, and instead of
# allowing the user to override the library order, we always make sure
# libkernel is the last library to be linked.
EE_LIBS += -lc -lkernel

# Externally defined variables: EE_BIN, EE_OBJS, EE_LIB

# These macros can be used to simplify certain build rules.
EE_C_COMPILE = $(EE_CC) $(EE_CFLAGS) $(EE_INCS)
EE_CXX_COMPILE = $(EE_CXX) $(EE_CXXFLAGS) $(EE_INCS)

# Extra macro for disabling the automatic inclusion of the built-in CRT object(s)
ifeq ($(EE_CC_VERSION),3.2.2)
	EE_NO_CRT = -mno-crt0
else ifeq ($(EE_CC_VERSION),3.2.3)
	EE_NO_CRT = -mno-crt0
else
	EE_NO_CRT = -nostartfiles
	CRTBEGIN_OBJ := $(shell $(EE_CC) $(CFLAGS) -print-file-name=crtbegin.o)
	CRTEND_OBJ := $(shell $(EE_CC) $(CFLAGS) -print-file-name=crtend.o)
	CRTI_OBJ := $(shell $(EE_CC) $(CFLAGS) -print-file-name=crti.o)
	CRTN_OBJ := $(shell $(EE_CC) $(CFLAGS) -print-file-name=crtn.o)
endif

%.o: %.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%.o: %.cc
	$(EE_CXX) $(EE_CXXFLAGS) $(EE_INCS) -c $< -o $@

%.o: %.cpp
	$(EE_CXX) $(EE_CXXFLAGS) $(EE_INCS) -c $< -o $@

%.o: %.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%.o: %.s
	$(EE_AS) $(EE_ASFLAGS) $< -o $@

$(EE_BIN): $(EE_OBJS) $(PS2SDK)/ee/startup/crt0.o
	$(EE_CC) $(EE_NO_CRT) -T$(PS2SDK)/ee/startup/linkfile $(EE_CFLAGS) \
		-o $(EE_BIN) $(PS2SDK)/ee/startup/crt0.o $(CRTI_OBJ) $(CRTBEGIN_OBJ) $(EE_OBJS) $(CRTEND_OBJ) $(CRTN_OBJ) $(EE_LDFLAGS) $(EE_LIBS)

$(EE_ERL): $(EE_OBJS)
	$(EE_CC) $(EE_NO_CRT) -o $(EE_ERL) $(EE_OBJS) $(EE_CFLAGS) $(EE_LDFLAGS) -Wl,-r -Wl,-d
	$(EE_STRIP) --strip-unneeded -R .mdebug.eabi64 -R .reginfo -R .comment $(EE_ERL)

$(EE_LIB): $(EE_OBJS)
	$(EE_AR) cru $(EE_LIB) $(EE_OBJS)

