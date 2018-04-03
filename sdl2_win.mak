SDL2_ROOT=C:\SDKS\SDL2-2.0.8
SDL2_INCLUDE=$(SDL2_ROOT)\include
SDL2_LIB=$(SDL2_ROOT)\lib\x64

INCLUDE_DIRS=/Isrc /Isrc/sdl2 /I$(SDL2_INCLUDE)
SRC=src\\*.c src\\sdl2\\*.c
LIBS=$(SDL2_LIB)\SDL2.lib $(SDL2_LIB)\SDL2main.lib

CC=cl
CFLAGS=/wd4028
LDFLAGS=

CFLAGS_RELEASE=/Ox $(INCLUDE_DIRS) 
LDFLAGS_RELEASE=/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup

CFLAGS+=$(CFLAGS_RELEASE)
LDFLAGS+=$(LDFLAGS_RELEASE)


all: compile copy_dll clean

compile:
	$(CC) /Febuild\unes.exe $(SRC) $(LIBS) $(CFLAGS) /link $(LDFLAGS)

copy_dll:
	copy $(SDL2_LIB)\SDL2.dll build\SDL2.dll
clean:
	del *.obj
