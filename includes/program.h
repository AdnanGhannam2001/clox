#ifndef CLOX_PROGRAM_H
#define CLOX_PROGRAM_H

#include <stdarg.h>
#include "common.h"
#include "array.h"

// ChunkArray
typedef enum op_code
{
    OP_CONSTANT,
    OP_RETURN,

    OP_COUNT
} op_code_t;

ARRAY(chunk_array, uint8_t)

// ValueArray
typedef double value_t;
ARRAY(value_array, value_t)

typedef struct program
{
    chunk_array_t chunks;
    value_array_t constants;
} program_t;

void program_init(program_t *program);
void program_write(program_t *program, op_code_t value, ...);
void program_free(program_t *program);

#ifdef CLOX_DEBUG

void program_disassemble(program_t *program, const char *name);
void program_instruction_disassemble(program_t *program, size_t *i);

#endif // CLOX_DEBUG
#endif // CLOX_PROGRAM_H

// TODO: Keep track of lines