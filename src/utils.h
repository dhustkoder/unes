#ifndef UNES_UTILS_H_
#define UNES_UTILS_H_
#include <stdbool.h>
#include <stdint.h>


#define IS_IN_ARRAY(v, a) (is_in_array(&v, a, sizeof(a)/sizeof(v), sizeof(v)))


static inline bool is_in_array(void* value, void* array,
                              int array_size, int element_size)
{
	uint8_t* const end = ((uint8_t*)array) + array_size * element_size;
	uint8_t* p = array;
	
	while (p < end) {
		if (memcmp(value, p, element_size) == 0)
			return true;
		p += element_size;
	}

	return false;
}


#endif
