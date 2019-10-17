@echo off
set BUILD_TYPE=%1

set INCLUDE_DIRS=/Isrc /Isrc/win32 
set SRC=src\\*.c src\\win32\\*.c
set LIBS=user32.lib gdi32.lib winmm.lib

set CC=cl
set CFLAGS=/D_CRT_SECURE_NO_WARNINGS /wd4028 /wd4214 /W4  %INCLUDE_DIRS%
set LDFLAGS=/SUBSYSTEM:WINDOWS %LIBS%

set CFLAGS_DEBUG=/Od /Zi /DEBUG:FULL /DDEBUG
set LDFLAGS_DEBUG=

set CFLAGS_RELEASE=/Ox /DEBUG:NONE
set LDFLAGS_RELEASE=

if "%BUILD_TYPE%"=="release" goto setup_release else goto setup_debug


:setup_debug
set CFLAGS=%CFLAGS% %CFLAGS_DEBUG%
set LDFLAGS=%LDFLAGS% %LDFLAGS_DEBUG%
goto compile

:setup_release
set CFLAGS=%CFLAGS% %CFLAGS_RELEASE%
set LDFLAGS=%LDFLAGS% %LDFLAGS_RELEASE%
goto compile

:compile
if not exist "build" mkdir build

@echo on
%CC% %CFLAGS% %SRC% %LIBS% /Febuild\unes.exe /link %LDFLAGS%

:clean
@echo off
del *.obj *.pdb

if "%2"=="run" .\build\unes.exe


