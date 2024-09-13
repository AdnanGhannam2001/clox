#include "value_stack.h"

void value_stack_init(value_stack_t *stack)
{
    stack->count = 0;
    memset(stack->items, 0, CLOX_VALUE_STACK_MAX * sizeof(value_t));
}

void value_stack_push(value_stack_t *stack, value_t value)
{
    stack->items[stack->count++] = value;
}

value_t value_stack_pop(value_stack_t *stack)
{
    if (stack->count <= 0)
    {
        fprintf(stderr, "ERROR: Stack is empty\n");
        exit(1);
    }

    return stack->items[--stack->count];
}

void value_stack_print(value_stack_t *stack)
{
    printf("[ ");
    for (size_t i = 0; i < stack->count; ++i)
    {
        switch(stack->items[i].type)
        {
            case VAL_BOOL:   { printf("%s ", AS_BOOL(stack->items[i]) ? "true" : "false"); } break;
            case VAL_NIL:    { printf("nil "); } break;
            case VAL_NUMBER: { printf("%g ", AS_NUMBER(stack->items[i])); } break;
        }
    }
    printf("]\n");
}