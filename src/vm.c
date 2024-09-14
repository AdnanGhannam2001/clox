#include "vm.h"

void vm_init(vm_t *vm)
{
    value_stack_init(&vm->stack);
}

void vm_error(vm_t *vm, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[INTERPRETER] ERROR: ");
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);

    value_stack_init(&vm->stack);
}

static interpret_result_t vm_run(vm_t *vm)
{
#define READ_INSTRUCTION(vm) (*(vm)->ip++)
#define READ_CONSTANT(vm) ((vm)->program->constants.items[READ_INSTRUCTION(vm)])
#define BINARY_OP(vm, cast, op)                                                  \
    do                                                                           \
    {                                                                            \
        value_t right = value_stack_pop(&vm->stack);                             \
        value_t left = value_stack_pop(&vm->stack);                              \
        if (!IS_NUMBER(right) || !IS_NUMBER(left))                               \
        {                                                                        \
            vm_error(vm, "Operands for '+', '-', '*' and '/' must be numbers");  \
            return INTERPRET_RESULT_RUNTIME_ERROR;                               \
        }                                                                        \
        value_stack_push(&vm->stack, cast(AS_NUMBER(left) op AS_NUMBER(right))); \
    } while (0);

    while (true)
    {
        uint8_t instruction = READ_INSTRUCTION(vm);
        switch(instruction)
        {
            case OP_CONSTANT:
            case OP_STRING:
                {
                    value_t value = READ_CONSTANT(vm);
                    value_stack_push(&vm->stack, value);
                } break;
            case OP_NIL:   { value_stack_push(&vm->stack, NIL_VAL); } break;
            case OP_TRUE:  { value_stack_push(&vm->stack, BOOL_VAL(true)); } break;
            case OP_FALSE: { value_stack_push(&vm->stack, BOOL_VAL(false)); } break;

            case OP_ADD:
                {
                    value_t right = value_stack_pop(&vm->stack);
                    value_t left = value_stack_pop(&vm->stack);

                    if (!value_addable(right, left))
                    {
                        vm_error(vm, "Values can't be added");
                        return INTERPRET_RESULT_RUNTIME_ERROR;
                    }

                    value_stack_push(&vm->stack, value_add(right, left));
                } break;
            case OP_SUB:   { BINARY_OP(vm, NUMBER_VAL, -); } break;
            case OP_MULTI: { BINARY_OP(vm, NUMBER_VAL, *); } break;
            case OP_DIV:   { BINARY_OP(vm, NUMBER_VAL, /); } break;

            case OP_GREATER: { BINARY_OP(vm, BOOL_VAL, <); } break;
            case OP_LESS:    { BINARY_OP(vm, BOOL_VAL, >); } break;
            case OP_EQUAL: 
                {
                    value_t right = value_stack_pop(&vm->stack);
                    value_t left = value_stack_pop(&vm->stack);

                    cmp_t cmp;
                    if ((cmp = value_cmp(right, left)) == CMP_ERROR)
                    {
                        vm_error(vm, "Can't compare two different types");
                        return INTERPRET_RESULT_RUNTIME_ERROR;
                    }

                    value_stack_push(&vm->stack, BOOL_VAL(cmp == CMP_EQUAL ? true : false));
                } break;

            case OP_NEGATE:
                {
                    value_t top = value_stack_pop(&vm->stack);
                    if (!IS_NUMBER(top))
                    {
                        vm_error(vm, "Operand after '-' must be a number");
                        return INTERPRET_RESULT_RUNTIME_ERROR;
                    }
                    value_stack_push(&vm->stack, NUMBER_VAL(-AS_NUMBER(top)));
                } break;
            case OP_NOT:
                {
                    value_t top = value_stack_pop(&vm->stack);
                    if (!IS_BOOL(top) && !IS_NIL(top))
                    {
                        vm_error(vm, "Operand after '!' must be truthy");
                        return INTERPRET_RESULT_RUNTIME_ERROR;
                    }
                    value_stack_push(&vm->stack, BOOL_VAL(IS_NIL(top) || !AS_BOOL(top)));
                } break;
            case OP_RETURN:
                {
                    return INTERPRET_RESULT_OK;
                }
        }

#ifdef CLOX_DEBUG
        value_stack_print(&vm->stack);
#endif // CLOX_DEBUG
    }

#undef BINARY_OP
#undef READ_INSTRUCTION
#undef READ_CONSTANT
}

interpret_result_t vm_interpret(vm_t *vm, program_t *program)
{
    vm->program = program;
    vm->ip = program->chunks.items;

    return vm_run(vm);
}

void vm_free(vm_t *vm)
{
    value_stack_init(&vm->stack);
}
