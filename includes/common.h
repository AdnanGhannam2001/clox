#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define NOT_IMPLEMENTED assert(0 && "Not Implemented")
#define UNREACHABLE     assert(0 && "Unreachable")
#define UNUSED          __attribute__((unused))

typedef struct object object_t;
typedef struct object_string object_string_t;

typedef enum cmp
{
    CMP_EQUAL,
    CMP_NOT_EQUAL,
    CMP_ERROR,

    CMP_COUNT
} cmp_t;

typedef enum interpret_result
{
    INTERPRET_RESULT_OK,
    INTERPRET_RESULT_COMPILE_ERROR,
    INTERPRET_RESULT_RUNTIME_ERROR,
} interpret_result_t;

#endif // CLOX_COMMON_H
