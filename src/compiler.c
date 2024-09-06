#include "compiler.h"

static compiler_error_t expression(compiler_t *compiler, precedence_t precedence);
static compiler_error_t number(compiler_t *compiler);
static compiler_error_t unary(compiler_t *compiler);
static compiler_error_t binary(compiler_t *compiler);
static compiler_error_t grouping(compiler_t *compiler);

static const rule_t rules[] =
{
  [TOKEN_LEFT_PAREN]    = {grouping, /*call*/NULL,   PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     /*dot*/NULL,    PREC_CALL},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {/*variable*/NULL, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {/*string*/NULL,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     /*and_*/NULL,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {/*literal*/NULL,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {/*literal*/NULL,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     /*or_*/NULL,    PREC_OR},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {/*super_*/NULL,   NULL,   PREC_NONE},
  [TOKEN_THIS]          = {/*this_*/NULL,    NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {/*literal*/NULL,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static compiler_error_t expression(compiler_t *compiler, precedence_t precedence)
{
    compiler_error_t error = COMPILER_ERROR_NONE;

    compiler_advance(compiler);
    parse_fn prefix = rules[compiler->prev.type].prefix;

    if (prefix == NULL)
    {
        fprintf(stderr, "Expected expression\n");
        error = COMPILER_ERROR_EXPRESSION_EXPECTED;
        goto out;
    }

    if ((error = prefix(compiler) != 0))
        goto out;

    while (precedence < rules[compiler->curr.type].precedence)
    {
        compiler_advance(compiler);
        if ((error = rules[compiler->prev.type].infix(compiler)) != 0)
            goto out;
    }

out:
    return error;
}

static compiler_error_t number(compiler_t *compiler)
{
    if (program_write(compiler->program, OP_CONSTANT, strtod(compiler->prev.start, NULL)) < 0)
    {
        return COMPILER_ERROR_OUT_OF_MEMORY;
    }

    return COMPILER_ERROR_NONE;
}

static compiler_error_t unary(compiler_t *compiler)
{
    compiler_error_t error;

    token_type_t op = compiler->prev.type;

    if ((error = expression(compiler, PREC_UNARY)) != 0)
        return error;

    switch (op)
    {
        case TOKEN_MINUS: 
            {
                program_write(compiler->program, OP_NEGATE);
                return COMPILER_ERROR_NONE;
            }
        default:
            UNREACHABLE;
    }
}

static compiler_error_t binary(compiler_t *compiler)
{
    compiler_error_t error;

    token_type_t op = compiler->prev.type;

    rule_t rule = rules[op];
    if ((error = expression(compiler, rule.precedence)) != 0)
        return error;

    switch (op)
    {
        case TOKEN_PLUS: 
            {
                program_write(compiler->program, OP_ADD);
                return COMPILER_ERROR_NONE;
            }
        case TOKEN_MINUS: 
            {
                program_write(compiler->program, OP_SUB);
                return COMPILER_ERROR_NONE;
            }
        case TOKEN_STAR: 
            {
                program_write(compiler->program, OP_MULTI);
                return COMPILER_ERROR_NONE;
            }
        case TOKEN_SLASH: 
            {
                program_write(compiler->program, OP_DIV);
                return COMPILER_ERROR_NONE;
            }
        default:
            UNREACHABLE;
    }
}

static compiler_error_t grouping(compiler_t *compiler)
{
    compiler_error_t error;
    if ((error = expression(compiler, PREC_CALL)) != 0)
        return error;

    if ((error = compiler_consume(compiler, TOKEN_EOF)) != 0)
        return error;

    return COMPILER_ERROR_NONE;
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

    compiler_error_t error;

    if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
        return error;

    if ((error = compiler_consume(compiler, TOKEN_EOF)) != 0)
        return error;

    program_write(compiler->program, OP_RETURN);

    return COMPILER_ERROR_NONE;
}
