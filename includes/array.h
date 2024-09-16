#ifndef CLOX_ARRAY_H
#define CLOX_ARRAY_H

#include "common.h"
#include "memory.h"

typedef size_t array_size_t;

#define ARRAY(name, type)                           \
    typedef struct                                  \
    {                                               \
        type *items;                                \
        array_size_t count;                         \
        array_size_t capacity;                      \
    } name##_t;                                     \
    void name##_init(name##_t *array);              \
    void name##_write(name##_t *array, type value); \
    void name##_free(name##_t *array);

#define ARRAY_IMPL(name, type)                                                                                           \
    void name##_init(name##_t *array)                                                                                    \
    {                                                                                                                    \
        (array)->capacity = 0;                                                                                           \
        (array)->count = 0;                                                                                              \
        (array)->items = NULL;                                                                                           \
    }                                                                                                                    \
    void name##_write(name##_t *array, type value)                                                                       \
    {                                                                                                                    \
        if ((array)->count >= (array)->capacity)                                                                         \
        {                                                                                                                \
            (array)->capacity = GROW_CAPACITY((array)->capacity);                                                        \
            (array)->items = (type *)memory_allocate((array)->items, ((array)->capacity * sizeof(array_size_t)), false); \
        }                                                                                                                \
        (array)->items[(array)->count] = (value);                                                                        \
        (array)->count++;                                                                                                \
    }                                                                                                                    \
    void name##_free(name##_t *array)                                                                                    \
    {                                                                                                                    \
        memory_free((array)->items);                                                                                     \
        name##_init(array);                                                                                              \
    }

#endif // CLOX_ARRAY_H
