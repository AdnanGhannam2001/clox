#include "vm.h"

void vm_init(vm_t *vm)
{
    value_stack_init(&vm->stack);
}

static interpret_result_t vm_run(vm_t *vm)
{
#define READ_INSTRUCTION(vm) (*(vm)->ip++)
#define READ_CONSTANT(vm) ((vm)->program->constants.items[READ_INSTRUCTION(vm)])
#define BINARY_OP(vm, op)                            \
    do                                               \
    {                                                \
        value_t right = value_stack_pop(&vm->stack); \
        value_t left = value_stack_pop(&vm->stack);  \
        value_stack_push(&vm->stack, left op right); \
    } while (0);

    while (true)
    {
        uint8_t instruction = READ_INSTRUCTION(vm);
        switch(instruction)
        {
            case OP_CONSTANT:
                {
                    value_t value = READ_CONSTANT(vm);
                    value_stack_push(&vm->stack, value);
                } break;
            case OP_ADD: { BINARY_OP(vm, +); } break;
            case OP_SUB: { BINARY_OP(vm, -); } break;
            case OP_MULTI: { BINARY_OP(vm, *); } break;
            case OP_DIV: { BINARY_OP(vm, /); } break;
            case OP_NEGATE:
                {
                    value_stack_push(&vm->stack, -value_stack_pop(&vm->stack));
                } break;
            case OP_RETURN:
                {
                    value_stack_pop(&vm->stack);
                    return INTERPRET_RESULT_OK;
                }
        }

#ifdef CLOX_DEBUG
        value_stack_print(&vm->stack);
#endif // DEBUG
    }

#undef BINARY_OP
#undef READ_INSTRUCTION
#undef READ_CONSTANT
}

interpret_result_t vm_interpret(vm_t *vm, program_t *program)
{
    vm->program = program;
    vm->ip = program->chunks.items;

    NOT_IMPLEMENTED;

    return vm_run(vm);
}

void vm_free(vm_t *vm) { (void)vm; }