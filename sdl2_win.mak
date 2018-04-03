CC=cl
SDL2_ROOT=C:\SDKS\SDL2-2.0.8
SDL2_INCLUDE=$(SDL2_ROOT)\include
SDL2_LIB=$(SDL2_ROOT)\lib\x64

all: compile copy_dll clean

compile:
	$(CC)  /Isrc /Isrc/SDL2 /I$(SDL2_INCLUDE) $(SDL2_LIB)\SDL2.lib $(SDL2_LIB)\SDL2main.lib  src\\*.c src\\SDL2\\*.c  /Febuild\unes.exe /link /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup

copy_dll:
	copy $(SDL2_LIB)\SDL2.dll build\SDL2.dll
clean:
	del *.obj
