#include <Windows.h>
#include <stdio.h>
#include "internal_log.h"

static char log_buffer[4096];

void init_log_system(void)
{
	AttachConsole(ATTACH_PARENT_PROCESS);
	log_info("\n");
}

void term_log_system(void)
{
	FreeConsole();
}



void log_info(const char* fmtstr, ...)
{
	DWORD towrite, written;
	va_list valist;
	va_start(valist, fmtstr);
	towrite = vsnprintf(log_buffer, sizeof(log_buffer) - 1, fmtstr, valist);
	va_end(valist);
	
	log_buffer[towrite++] = '\n';
	
	HANDLE stdhandle = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleA(stdhandle, log_buffer, towrite, &written, NULL);
}

void log_error(const char* fmtstr, ...)
{
	
}

#ifdef DEBUG
void log_debug(const char* fmtstr, ...)
{
	
}
#else
#define log_debug(...) ((void)0)
#endif