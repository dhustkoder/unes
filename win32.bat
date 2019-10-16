
set INCLUDE_DIRS=/Isrc /Isrc/win32 /I$(SDL2_INCLUDE)
set SRC=src\\*.c src\\win32\\*.c
set LIBS=

set CC=cl
set CFLAGS=/D_CRT_SECURE_NO_WARNINGS /wd4028 /wd4214 /W4  %INCLUDE_DIRS%
set LDFLAGS=/SUBSYSTEM:WINDOWS

set CFLAGS_DEBUG=/Od /Zi /DEBUG:FULL /DDEBUG
set LDFLAGS_DEBUG=user32.lib

set CFLAGS_RELEASE=/Ox
set LDFLAGS_RELEASE=

set CFLAGS=%CFLAGS% %CFLAGS_DEBUG%
set LDFLAGS=%LDFLAGS% %LDFLAGS_DEBUG%



if not exist "build" mkdir build
%CC% %CFLAGS% %SRC% %LIBS% /Febuild\unes.exe /link %LDFLAGS%
del *.obj *.pdb
