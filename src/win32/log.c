#include <Windows.h>
#include "platform.h"

static char log_buffer[4096];
static HANDLE handles[LOGGER_ID_COUNT];

void internal_logger(LoggerID id, const char* fmtstr, ...)
{
	DWORD towrite, written;
	va_list valist;
	va_start(valist, fmtstr);
	towrite = vsnprintf(log_buffer, sizeof(log_buffer) - 1, fmtstr, valist);
	va_end(valist);
	
	log_buffer[towrite++] = '\n';
	
	const HANDLE stdhandle = handles[id];
	WriteConsoleA(stdhandle, log_buffer, towrite, &written, NULL);
}


void init_log_system(void)
{
	AttachConsole(ATTACH_PARENT_PROCESS);
	handles[LOGGER_ID_STDOUT] = GetStdHandle(STD_OUTPUT_HANDLE);
	handles[LOGGER_ID_STDERR] = GetStdHandle(STD_ERROR_HANDLE);
	log_info("\n");
}

void term_log_system(void)
{
	FreeConsole();
}

