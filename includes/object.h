#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "common.h"

typedef enum object_type
{
    OBJECT_STRING,

    OBJECT_COUNT
} object_type_t;

struct object
{
    object_type_t type;
};

struct object_string
{
    object_t object;
    size_t length;
    char* data;
};

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)
#define IS_STRING(value) (IS_OBJECT(value) && (OBJECT_VAL(value)->type) == OBJECT_STRING)
#define AS_STRING(value) ((object_string_t *)AS_OBJECT(value))

object_t *object_new(const object_type_t, const size_t);
void object_print(const object_t *);
object_string_t *object_string_new(const char *, const size_t);

#endif // CLOX_OBJECT_H
