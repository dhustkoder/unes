#ifndef UNES_LOG_H_
#define UNES_LOG_H_

enum stdhandle_index {
	UNES_LOG_STDOUT,
	UNES_LOG_STDERR
};

extern void internal_logger(enum stdhandle_index idx, const char* fmt, ...);

#define log_info(fmtstr, ...) internal_logger(UNES_LOG_STDOUT, fmtstr, __VA_ARGS__)
#define log_error(fmtstr, ...) internal_logger(UNES_LOG_STDERR, fmtstr, __VA_ARGS__)
#ifdef DEBUG
#define log_debug(fmtstr, ...) internal_logger(UNES_LOG_STDOUT, fmtstr, __VA_ARGS__)
#else
#define log_debug(...) ((void)0)
#endif

#endif
