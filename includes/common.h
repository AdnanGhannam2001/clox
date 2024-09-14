#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define NOT_IMPLEMENTED assert(0 && "Not Implemented")
#define UNREACHABLE     assert(0 && "Unreachable")

typedef enum value_type
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJECT,

    VAL_COUNT
} value_type_t;

typedef struct object object_t;
typedef struct object_string object_string_t;

typedef struct value
{
    value_type_t type;
    union
    {
        bool boolean;
        double number;
        object_t *object;
    } as;
} value_t;

#define BOOL_VAL(value)   ((value_t){VAL_BOOL, {.boolean = (value)}})
#define NIL_VAL           ((value_t){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((value_t){VAL_NUMBER, {.number = (value)}})
#define OBJECT_VAL(value) ((value_t){VAL_OBJECT, {.object = (object_t *)(value)}})

#define AS_BOOL(value)   ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJECT(value) ((value).as.object)

#define IS_BOOL(value)   ((value).type == VAL_BOOL)
#define IS_NIL(value)    ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJECT(value) ((value).type == VAL_OBJECT)

typedef enum interpret_result
{
    INTERPRET_RESULT_OK,
    INTERPRET_RESULT_COMPILE_ERROR,
    INTERPRET_RESULT_RUNTIME_ERROR,
} interpret_result_t;

#endif // CLOX_COMMON_H
