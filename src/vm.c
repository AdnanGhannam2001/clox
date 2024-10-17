#include "vm.h"

static inline bool callable(value_t value)
{
    return IS_OBJECT(value) && IS_FUNCTION(value);
}

static void call(vm_t *vm, object_function_t *function, uint8_t args_count)
{
    call_frame_t *frame = &vm->frames.items[vm->frames.count++];
    frame->function = function;
    frame->ip = function->program.chunks.items;
    frame->fp = vm->stack.items - args_count - 1;
}

void vm_init(vm_t *vm)
{
    value_stack_init(&vm->stack);
    table_init(&vm->globals);
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
    call_frame_t *frame = &vm->frames.items[vm->frames.count - 1];

#define READ_INSTRUCTION() (*(frame)->ip++)
#define READ_CONSTANT() (frame->function->program.constants.items[READ_INSTRUCTION()])
#define READ_STRING() (AS_STRING(READ_CONSTANT()))
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
#define PEEK(vm, distance) vm->stack.items[vm->stack.count - distance - 1]

    while (true)
    {
        chunk instruction = READ_INSTRUCTION();
        switch(instruction)
        {
            case OP_CONSTANT:
                {
                    value_t value = READ_CONSTANT();
                    value_stack_push(&vm->stack, value);
                } break;
            case OP_NIL:   { value_stack_push(&vm->stack, NIL_VAL); } break;
            case OP_TRUE:  { value_stack_push(&vm->stack, BOOL_VAL(true)); } break;
            case OP_FALSE: { value_stack_push(&vm->stack, BOOL_VAL(false)); } break;

            case OP_POP: { value_stack_pop(&vm->stack); } break;

            case OP_DEFINE_GLOBAL:
                {
                    object_string_t *name = READ_STRING();
                    table_entry_set(&vm->globals, name, value_stack_pop(&vm->stack));
                } break;
            case OP_GET_GLOBAL:
                {
                    object_string_t *name = READ_STRING();
                    if (table_entry_get(&vm->globals, name)->key == NULL)
                    {
                        vm_error(vm, "Used of undefined variable: '%.*s'", (int)name->length, name->data);
                        return INTERPRET_RESULT_RUNTIME_ERROR;
                    }

                    value_stack_push(&vm->stack, table_entry_get(&vm->globals, name)->value);
                } break;
            case OP_SET_GLOBAL:
                {
                    object_string_t *name = READ_STRING();
                    if (table_entry_get(&vm->globals, name)->key == NULL)
                    {
                        vm_error(vm, "Used of undefined variable: '%.*s'", (int)name->length, name->data);
                        return INTERPRET_RESULT_RUNTIME_ERROR;
                    }
                    table_entry_set(&vm->globals, name, value_stack_top(&vm->stack));
                } break;

            case OP_GET_LOCAL:
                {
                    value_t index_value = READ_CONSTANT();
                    value_stack_push(&vm->stack, frame->fp[(int)AS_NUMBER(index_value)]);
                } break;

            case OP_SET_LOCAL:
                {
                    value_t index_value = READ_CONSTANT();
                    value_t top = value_stack_pop(&vm->stack);
                    frame->fp[(int)AS_NUMBER(index_value)] = top;
                } break;

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

            case OP_NOT:
                {
                    value_t top = value_stack_pop(&vm->stack);
                    if (!IS_TRUTHY(top))
                    {
                        vm_error(vm, "Operand after '!' must be truthy");
                        return INTERPRET_RESULT_RUNTIME_ERROR;
                    }
                    value_stack_push(&vm->stack, BOOL_VAL(IS_NIL(top) || !AS_BOOL(top)));
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

            case OP_PRINT:
                {
                    value_t top = value_stack_pop(&vm->stack);
                    value_print(top);
                    printf("\n");
                } break;

            case OP_JUMP_IF_FALSE:
                {
                    value_t top = value_stack_top(&vm->stack);
                    if (!IS_TRUTHY(top))
                    {
                        vm_error(vm, "Condition should be boolean");
                        return INTERPRET_RESULT_RUNTIME_ERROR;
                    }

                    if (AS_TRUTHY(top))
                    {
                        frame->ip += 2;
                        break;
                    }
                }
            case OP_JUMP:
                {
                    frame->ip = frame->function->program.chunks.items + ((READ_INSTRUCTION() << 8) | READ_INSTRUCTION());
                } break;

            case OP_CALL:
                {
                    uint8_t args_count = READ_INSTRUCTION();

                    value_t callee = PEEK(vm, args_count);
                    if (!callable(callee))
                    {
                        vm_error(vm, "Value is not callable");
                        return INTERPRET_RESULT_RUNTIME_ERROR;
                    }

                    call(vm, AS_FUNCTION(callee), args_count);

                    frame = &vm->frames.items[vm->frames.count - 1];
                } break;
            case OP_RETURN:
                {
                    value_t result = value_stack_pop(&vm->stack);
                    if (--vm->frames.count > 0)
                    {
                        value_stack_pop(&vm->stack);
                        return INTERPRET_RESULT_OK;
                    }

                    value_stack_push(&vm->stack, result);
                    frame = &vm->frames.items[vm->frames.count - 1];
                }
        }

#ifdef CLOX_DEBUG_PRINT
        value_stack_print(&vm->stack);
#endif // CLOX_DEBUG_PRINT
    }

#undef PEEK
#undef BINARY_OP
#undef READ_INSTRUCTION
#undef READ_STRING
#undef READ_CONSTANT
}

interpret_result_t vm_interpret(vm_t *vm, object_function_t *function)
{
    vm->frames.count = 1;
    vm->frames.items[0].function = function;
    vm->frames.items[0].ip = function->program.chunks.items;
    vm->frames.items[0].fp = vm->stack.items;
    value_stack_push(&vm->stack, OBJECT_VAL(function));
    call(vm, function, 0);

    return vm_run(vm);
}

void vm_free(vm_t *vm)
{
    value_stack_init(&vm->stack);
    table_free(&vm->globals);
}
