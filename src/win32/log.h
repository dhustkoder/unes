#ifndef UNES_LOG_H_
#define UNES_LOG_H_


extern void log_info(const char* fmtstr, ...);
extern void log_error(const char* fmtstr, ...);
#ifdef DEBUG
extern void log_debug(const char* fmtstr, ...);
#else
#define log_debug(...) ((void)0)
#endif

#endif
