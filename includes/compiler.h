#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "common.h"
#include "program.h"
#include "tokenizer.h"

typedef enum compiler_error
{
    COMPILER_ERROR_NONE,
    COMPILER_ERROR_UNEXPECTED_TOKEN,
    COMPILER_ERROR_EXPRESSION_EXPECTED,
    COMPILER_ERROR_OUT_OF_MEMORY,

    COMPILER_ERROR_COUNT
} compiler_error_t;

typedef struct compiler
{
    tokenizer_t *tokenizer;
    program_t *program;
    token_t curr;
    token_t prev;
} compiler_t;

typedef enum precedence
{
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} precedence_t;

typedef compiler_error_t (*parse_fn)(compiler_t *);

typedef struct rule
{
    parse_fn prefix;
    parse_fn infix;
    precedence_t precedence;
} rule_t;

void compiler_init(compiler_t *compiler, tokenizer_t *tokenizer, program_t *program);
void compiler_advance(compiler_t *);
compiler_error_t compiler_consume(compiler_t *, const token_type_t);
compiler_error_t compiler_run(compiler_t *, const char *, program_t *program);

#endif // CLOX_COMPILER_H