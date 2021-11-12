#ifndef T1M_SPECIFIC_MEMORY_H
#define T1M_SPECIFIC_MEMORY_H

// Basic memory utilities that exit the game in case the system runs out of
// memory.

#include <stdint.h>

void *S_Memory_Alloc(size_t size);
void *S_Memory_Realloc(void *memory, size_t size);
void S_Memory_Free(void *memory);

#endif
