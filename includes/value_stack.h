#ifndef CLOX_VALUE_STACK_H
#define CLOX_VALUE_STACK_H

#include "common.h"
#include "program.h"

#define CLOX_VALUE_STACK_MAX UINT8_MAX

typedef struct value_stack
{
    value_t items[CLOX_VALUE_STACK_MAX];
    size_t count;
} value_stack_t;

void value_stack_init(value_stack_t *);
void value_stack_push(value_stack_t *, value_t value);
value_t value_stack_pop(value_stack_t *);
void value_stack_print(value_stack_t *);

#endif // CLOX_VALUE_STACK_H