#ifndef CLOX_PROGRAM_H
#define CLOX_PROGRAM_H

#include <stdarg.h>
#include "common.h"
#include "value.h"
#include "array.h"
#include "object.h"

// ChunkArray
typedef enum op_code
{
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUB,
    OP_MULTI,
    OP_DIV,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_RETURN,

    OP_COUNT
} op_code_t;

ARRAY(chunk_array, uint8_t)

// ValueArray
ARRAY(value_array, value_t)

typedef struct program
{
    chunk_array_t chunks;
    value_array_t constants;
} program_t;

void program_init(program_t *program);
int program_write(program_t *program, op_code_t value, ...);
void program_free(program_t *program);
void program_disassemble(program_t *program, const char *name);
void program_instruction_disassemble(program_t *program, size_t *i);
#endif // CLOX_PROGRAM_H

// TODO: Keep track of lines
