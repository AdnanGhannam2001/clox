#include "memory.h"

void *memory_allocate(void *ptr, size_t size, bool zinit)
{
    ptr = NULL;
    void *memory = realloc(ptr, size);
    assert((memory != NULL) && "Memory allocation failed: Couldn't allocate more memory");

    if (zinit)
        memset(memory, 0, size);

    return memory;
}

void memory_free(void *ptr)
{
    free(ptr);
}
