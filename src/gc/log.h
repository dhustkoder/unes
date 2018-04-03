#ifndef UNES_LOG_H_
#define UNES_LOG_H_
#include <stdio.h>

#define loginfo(...)  (printf(__VA_ARGS__))
#define logerror(...) (fprintf(stderr, __VA_ARGS__))
#ifdef DEBUG
#define logdebug(...) (printf(__VA_ARGS__))
#else
#define logdebug(...) ((void)0)
#endif


#endif
