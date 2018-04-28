#ifndef UNES_LOG_H_
#define UNES_LOG_H_
#include <stdio.h>

#define log_info(...)  (printf(__VA_ARGS__))
#define log_error(...) (fprintf(stderr, __VA_ARGS__))
#ifdef DEBUG
#define log_debug(...) (printf(__VA_ARGS__))
#else
#define log_debug(...) ((void)0)
#endif


#endif
