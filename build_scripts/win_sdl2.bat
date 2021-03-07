@echo off
set BUILD_TYPE=%1

set SDL2_ROOT=D:\sdks\SDL2-2.0.14

set INCLUDE_DIRS=/Isrc /Isrc/sdl2 /I%SDL2_ROOT%/include
set LIBRARY_DIRS=%SDL2_ROOT%\lib\x64
set SRC=src\sdl2\cunits.c
set LIBS=sdl2.lib sdl2main.lib shell32.lib


set CC=clang-cl -fsanitize=address -fsanitize=undefined
set CFLAGS=/D_CRT_SECURE_NO_WARNINGS /wd4028 /wd4214 /wd4047 /wd4210 /W4  %INCLUDE_DIRS%

set LDFLAGS=/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup /LIBPATH:%LIBRARY_DIRS%^ %LIBS%

set CFLAGS_DEBUG=/Od /Zi /DEBUG:FULL /DDEBUG
set LDFLAGS_DEBUG=

set CFLAGS_RELEASE=/Ox /DEBUG:NONE /DNDEBUG /DNDEBUG
set LDFLAGS_RELEASE=

if "%BUILD_TYPE%"=="release" goto build_release else goto build_debug


:build_debug
set CFLAGS=%CFLAGS% %CFLAGS_DEBUG%
set LDFLAGS=%LDFLAGS% %LDFLAGS_DEBUG%
goto compile

:build_release
set CFLAGS=%CFLAGS% %CFLAGS_RELEASE%
set LDFLAGS=%LDFLAGS% %LDFLAGS_RELEASE%
goto compile

:compile
if not exist "build" mkdir build

@echo on
%CC% %CFLAGS% %SRC% %LIBS% /Febuild\unes.exe /link %LDFLAGS%
@echo off

set ERROR=%errorLevel%
if not %ERROR%==0 goto EOF

:clean
del *.obj *.pdb

if "%~2"=="" goto EOF

:run
build\unes %2

:EOF
