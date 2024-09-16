#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
void *memory_allocate(void *, size_t, bool zinit);
void memory_free(void *);

#endif // CLOX_MEMORY_H
