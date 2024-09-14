#include "value.h"
#include "program.h"

cmp_t value_cmp(value_t a, value_t b)
{
    if (a.type == VAL_NIL || b.type == VAL_NIL)
        return a.type == VAL_NIL && b.type == VAL_NIL ? CMP_EQUAL : CMP_NOT_EQUAL;

    if (a.type != b.type)
        return CMP_ERROR;

    switch (a.type)
    {
    case VAL_BOOL:
        return AS_BOOL(a) == AS_BOOL(b) ? CMP_EQUAL : CMP_NOT_EQUAL;
    case VAL_NUMBER:
        return fabs(AS_NUMBER(a) - AS_NUMBER(b)) < 0.00001 ? CMP_EQUAL : CMP_NOT_EQUAL;
    case VAL_OBJECT:
        return object_cmp(AS_OBJECT(a), AS_OBJECT(b));
    default:
        UNREACHABLE;
    }
}

bool value_addable(const value_t a, const value_t b)
{
    return a.type == b.type && (IS_NUMBER(a) || IS_STRING(a));
}

value_t value_add(value_t a, value_t b)
{
    switch(a.type)
    {
    case VAL_NUMBER:
        return NUMBER_VAL(AS_NUMBER(a) + AS_NUMBER(b));
    case VAL_OBJECT:
        return OBJECT_VAL(object_string_concat(AS_STRING(a), AS_STRING(b)));
    default:
        UNREACHABLE;
    }
}

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

void value_stack_print(const value_stack_t *stack)
{
    printf("[ ");
    for (size_t i = 0; i < stack->count; ++i)
    {
        switch(stack->items[i].type)
        {
        case VAL_BOOL:   { printf("%s ", AS_BOOL(stack->items[i]) ? "true" : "false"); } break;
        case VAL_NIL:    { printf("nil "); } break;
        case VAL_NUMBER: { printf("%g ", AS_NUMBER(stack->items[i])); } break;
        case VAL_OBJECT:
            {
                object_print(AS_OBJECT(stack->items[i]));
            } break;
        default:
            UNREACHABLE;
        }
    }
    printf("]\n");
}
