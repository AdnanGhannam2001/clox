#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "common.h"
#include "tokenizer.h"
#include "program.h"
#include "object.h"

#ifndef CLOX_LOCALS_MAX
#define CLOX_LOCALS_MAX (UINT8_MAX + 1)
#endif // CLOX_LOCALS_MAX

#ifndef CLOX_MAIN_FN
#define CLOX_MAIN_FN "main"
#endif // CLOX_MAIN_FN

typedef enum compiler_error
{
    COMPILER_ERROR_NONE,
    COMPILER_ERROR_UNEXPECTED_TOKEN,
    COMPILER_ERROR_EXPRESSION_EXPECTED,
    COMPILER_ERROR_INVALID_ASSIGNMENT,
    COMPILER_ERROR_OUT_OF_MEMORY,

    COMPILER_ERROR_COUNT
} compiler_error_t;

typedef struct compiler_local
{
    token_t token;
    size_t depth;
} compiler_local_t;

typedef struct compiler_locals
{
    compiler_local_t items[CLOX_LOCALS_MAX];
    size_t count;
    size_t depth;
} compiler_locals_t;

typedef struct compiler
{
    tokenizer_t *tokenizer;
    object_function_t *function;
    token_t curr;
    token_t prev;

    compiler_locals_t locals;
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

typedef compiler_error_t (*parse_fn)(compiler_t *, bool);

typedef struct rule
{
    parse_fn prefix;
    parse_fn infix;
    precedence_t precedence;
} rule_t;

void compiler_init(compiler_t *, tokenizer_t *);
void compiler_free(compiler_t *);
void compiler_error(compiler_t *, const char *fmt, ...);
compiler_error_t compiler_run(compiler_t *, const char *);

#endif // CLOX_COMPILER_H

// TODO: Add a global compiler_error_t so compiler_run can return object_function_t* instead
