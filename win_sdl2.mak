SDL2_ROOT=C:\SDKS\SDL2-2.0.8
SDL2_INCLUDE=$(SDL2_ROOT)\include
SDL2_LIB=$(SDL2_ROOT)\lib\x64

INCLUDE_DIRS=/Isrc /Isrc/sdl2 /I$(SDL2_INCLUDE)
SRC=src\\*.c src\\sdl2\\*.c
LIBS=$(SDL2_LIB)\SDL2.lib $(SDL2_LIB)\SDL2main.lib

CC=cl
CFLAGS=/D_CRT_SECURE_NO_WARNINGS /wd4028 /wd4214 /W4
LDFLAGS=/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup

CFLAGS_RELEASE=/Ox $(INCLUDE_DIRS) 
LDFLAGS_RELEASE=

CFLAGS+=$(CFLAGS_RELEASE)
LDFLAGS+=$(LDFLAGS_RELEASE)


all: compile copy_dll clean

compile:
	if not exist "build" mkdir build
	$(CC) $(CFLAGS) $(SRC) $(LIBS) /Febuild\unes.exe /link $(LDFLAGS)

copy_dll:
	if not exist "build\SDL2.dll" copy $(SDL2_LIB)\SDL2.dll build\SDL2.dll

clean:
	del *.obj
