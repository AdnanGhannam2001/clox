#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "common.h"
#include "value.h"
#include "program.h"

typedef enum object_type
{
    OBJECT_STRING,
    OBJECT_FUNCTION,

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

// TODO: Add a way to seperate 'main' & 'native' & 'defined' functions
struct object_function
{
    object_t obj;
    object_string_t *name;
    size_t arity;
    program_t program;
};

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)

#define IS_STRING(value) (IS_OBJECT(value) && (AS_OBJECT(value)->type) == OBJECT_STRING)
#define IS_FUNCTION(value) (IS_OBJECT(value) && (AS_OBJECT(value)->type) == OBJECT_FUNCTION)

#define AS_STRING(value) ((object_string_t *)AS_OBJECT(value))
#define AS_FUNCTION(value) ((object_function_t *)AS_OBJECT(value))

object_t *object_new(const object_type_t, const size_t);
void object_destroy(object_t *);
cmp_t object_cmp(const object_t *, const object_t *);
void object_print(const object_t *);

object_string_t *object_string_new(const char *, const size_t);
void object_string_destroy(object_string_t *);
object_string_t *object_string_concat(object_string_t *, object_string_t *);
bool object_string_cmp(const object_string_t *, const object_string_t *);

object_function_t *object_function_new(const char *, const size_t);
void object_function_destroy(object_function_t *);

#endif // CLOX_OBJECT_H
