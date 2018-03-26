#ifndef UNES_UTILS_H_
#define UNES_UTILS_H_
#include <stdbool.h>
#include <stdint.h>


#define IS_IN_ARRAY(v, a) (is_in_array(&v, a, sizeof(v), sizeof(a)/sizeof(v)))


static inline bool is_in_array(const void* const value, const void* const array, const int size, const int nmemb)
{
	const uint8_t* const end = ((const uint8_t* const)array) + size * nmemb;
	for (const uint8_t* p = array; p < end; p += size) {
		if (memcmp(value, p, size) == 0)
			return true;
	}
	return false;
}


#endif
