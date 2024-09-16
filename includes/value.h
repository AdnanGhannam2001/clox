#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

typedef enum value_type
{
    VAL_NIL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_OBJECT,

    VAL_COUNT
} value_type_t;

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

cmp_t value_cmp(value_t, value_t);
bool value_addable(const value_t, const value_t);
value_t value_add(value_t, value_t);
void value_print(const value_t);

// ValueStack
#define CLOX_VALUE_STACK_MAX UINT8_MAX

typedef struct value_stack
{
    value_t items[CLOX_VALUE_STACK_MAX];
    size_t count;
} value_stack_t;

void value_stack_init(value_stack_t *);
void value_stack_push(value_stack_t *, value_t value);
value_t value_stack_pop(value_stack_t *);
void value_stack_print(const value_stack_t *);

#endif // CLOX_VALUE_H
