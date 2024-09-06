#include "compiler.h"

static void expression()
{
}

void compiler_init(compiler_t *compiler, tokenizer_t *tokenizer, program_t *program)
{
    compiler->tokenizer = tokenizer;
    compiler->program = program;
    compiler->curr = tokenizer_next(tokenizer);
}

void compiler_advance(compiler_t *compiler)
{
    compiler->prev = compiler->curr;
    compiler->curr = tokenizer_next(compiler->tokenizer);
}

compiler_error_t compiler_consume(compiler_t *compiler, const token_type_t type)
{
    if (compiler->curr.type != type)
    {
        // TODO print tokens as string
        fprintf(stderr, "Unexpected token %d, expected %d\n", compiler->curr.type, type);
        return COMPILER_ERROR_UNEXPECTED_TOKEN;
    }

    compiler_advance(compiler);
    return COMPILER_ERROR_NONE;
}

compiler_error_t compiler_run(compiler_t *compiler, const char *source, program_t *program)
{
    tokenizer_t tokenizer;
    tokenizer_init(&tokenizer, source);
    compiler_init(compiler, &tokenizer, program);

    expression();

    compiler_error_t error = compiler_consume(compiler, TOKEN_EOF);
    if (error != 0)
        return error;

    program_write(compiler->program, OP_RETURN);

    return COMPILER_ERROR_NONE;
}
