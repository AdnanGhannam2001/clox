#include "program.h"

ARRAY_IMPL(chunk_array, uint8_t)
ARRAY_IMPL(value_array, value_t)

void program_init(program_t *program)
{
    chunk_array_init(&program->chunks);
    value_array_init(&program->constants);
}

int program_write(program_t *program, op_code_t value, ...)
{
    assert(value < OP_COUNT);

    chunk_array_write(&program->chunks, (uint8_t)value);

    va_list args;
    va_start(args, value);

    switch (value)
    {
    case OP_CONSTANT:
    case OP_DEFINE_GLOBAL:
    case OP_GET_GLOBAL:
    case OP_SET_GLOBAL:
        {
            if (program->constants.count > UINT8_MAX)
            {
                fprintf(stderr, "Too many constants, max is: %d\n", UINT8_MAX);
                return -1;
            }

            value_array_write(&program->constants, va_arg(args, value_t));
            chunk_array_write(&program->chunks, (uint8_t)program->constants.count - 1);
        } break;
    default: {}
    }

    va_end(args);
    return 0;
}

void program_free(program_t *program)
{
    chunk_array_free(&program->chunks);
    value_array_free(&program->constants);
}

void program_disassemble(program_t *program, const char *name)
{
    printf("=== %s ===\n", name);

    for (size_t i = 0; i < program->chunks.count; ++i)
    {
        printf("%04zu\t", i);
        program_instruction_disassemble(program, &i);
    }
}

void program_instruction_disassemble(program_t *program, size_t *i)
{
    switch (program->chunks.items[*i])
    {
    case OP_CONSTANT:
        {
            printf("OP_CONSTANT\t");
            value_print(program->constants.items[program->chunks.items[++(*i)]]);
            printf("\n");
        } break;
    case OP_NIL:           { printf("OP_NIL\n"); } break;
    case OP_TRUE:          { printf("OP_TRUE\n"); } break;
    case OP_FALSE:         { printf("OP_FALSE\n"); } break;
    case OP_POP:           { printf("OP_POP\n"); } break;

    case OP_DEFINE_GLOBAL:
    case OP_GET_GLOBAL:
    case OP_SET_GLOBAL:
        {
            printf("OP_GLOBAL\t");
            value_print(program->constants.items[program->chunks.items[++(*i)]]);
            printf("\n");
        } break;

    case OP_ADD:           { printf("OP_ADD\n"); } break;
    case OP_SUB:           { printf("OP_SUB\n"); } break;
    case OP_MULTI:         { printf("OP_MULTI\n"); } break;
    case OP_DIV:           { printf("OP_DIV\n"); } break;
    case OP_NOT:           { printf("OP_NOT\n"); } break;
    case OP_NEGATE:        { printf("OP_NEGATE\n"); } break;
    case OP_PRINT:         { printf("OP_PRINT\n"); } break;
    case OP_RETURN:        { printf("OP_RETURN\n"); } break;
    default:
        fprintf(stderr, "Unknown instruction %u\n", program->chunks.items[*i]);
    }
}
