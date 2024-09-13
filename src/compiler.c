#include "compiler.h"

static compiler_error_t expression(compiler_t *compiler, precedence_t precedence);
static compiler_error_t literal(compiler_t *compiler);
static compiler_error_t unary(compiler_t *compiler);
static compiler_error_t binary(compiler_t *compiler);
static compiler_error_t grouping(compiler_t *compiler);
static void advance(compiler_t *);
static compiler_error_t consume(compiler_t *, const token_type_t);

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
  [TOKEN_NUMBER]        = {literal,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     /*and_*/NULL,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     /*or_*/NULL,    PREC_OR},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {/*super_*/NULL,   NULL,   PREC_NONE},
  [TOKEN_THIS]          = {/*this_*/NULL,    NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static compiler_error_t expression(compiler_t *compiler, precedence_t precedence)
{
    compiler_error_t error = COMPILER_ERROR_NONE;

    advance(compiler);
    parse_fn prefix = rules[compiler->prev.type].prefix;

    if (prefix == NULL)
    {
        compiler_error(compiler, "Expected expression");
        error = COMPILER_ERROR_EXPRESSION_EXPECTED;
        goto out;
    }

    if ((error = prefix(compiler) != 0))
        goto out;

    while (precedence <= rules[compiler->curr.type].precedence)
    {
        advance(compiler);
        if ((error = rules[compiler->prev.type].infix(compiler)) != 0)
            goto out;
    }

out:
    return error;
}

static compiler_error_t literal(compiler_t *compiler)
{
    switch(compiler->prev.type)
    {
        case TOKEN_NUMBER:
            {
                if (program_write(compiler->program, OP_CONSTANT, strtod(compiler->prev.start, NULL)) < 0)
                    return COMPILER_ERROR_OUT_OF_MEMORY;
            } break;
        case TOKEN_NIL:
            {
                if (program_write(compiler->program, OP_NIL))
                    return COMPILER_ERROR_OUT_OF_MEMORY;
            } break;
        case TOKEN_TRUE:
            {
                if (program_write(compiler->program, OP_TRUE))
                    return COMPILER_ERROR_OUT_OF_MEMORY;
            } break;
        case TOKEN_FALSE:
            {
                if (program_write(compiler->program, OP_FALSE))
                    return COMPILER_ERROR_OUT_OF_MEMORY;
            } break;
        default:
            UNREACHABLE;
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
        case TOKEN_MINUS: { program_write(compiler->program, OP_NEGATE); } break;
        case TOKEN_BANG:  { program_write(compiler->program, OP_NOT); } break;
        default:
            UNREACHABLE;
    }

    return COMPILER_ERROR_NONE;
}

static compiler_error_t binary(compiler_t *compiler)
{
    compiler_error_t error;
    token_type_t op = compiler->prev.type;

    rule_t rule = rules[op];
    if ((error = expression(compiler, rule.precedence + 1)) != 0)
        return error;

    switch (op)
    {
        case TOKEN_PLUS:  { program_write(compiler->program, OP_ADD); } break;
        case TOKEN_MINUS: { program_write(compiler->program, OP_SUB); } break;
        case TOKEN_STAR:  { program_write(compiler->program, OP_MULTI); } break;
        case TOKEN_SLASH: { program_write(compiler->program, OP_DIV); } break;

        case TOKEN_EQUAL_EQUAL: { program_write(compiler->program, OP_EQUAL); } break;
        case TOKEN_GREATER:     { program_write(compiler->program, OP_GREATER); } break;
        case TOKEN_LESS:        { program_write(compiler->program, OP_LESS); } break;
        case TOKEN_BANG_EQUAL:
            {
                program_write(compiler->program, OP_EQUAL);
                program_write(compiler->program, OP_NOT);
            } break;
        case TOKEN_GREATER_EQUAL:
            {
                program_write(compiler->program, OP_GREATER);
                program_write(compiler->program, OP_NOT);
            } break;
        case TOKEN_LESS_EQUAL:
            {
                program_write(compiler->program, OP_LESS);
                program_write(compiler->program, OP_NOT);
            } break;

        default:
            UNREACHABLE;
    }

    return COMPILER_ERROR_NONE;
}

static compiler_error_t grouping(compiler_t *compiler)
{
    compiler_error_t error;
    if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
        return error;

    if ((error = consume(compiler, TOKEN_RIGHT_PAREN)) != 0)
        return error;

    return COMPILER_ERROR_NONE;
}

static void advance(compiler_t *compiler)
{
    compiler->prev = compiler->curr;
    compiler->curr = tokenizer_next(compiler->tokenizer);
}

static compiler_error_t consume(compiler_t *compiler, const token_type_t type)
{
    if (compiler->curr.type != type)
    {
        compiler_error(compiler, "Unexpected token '%s', expected '%s'",
                       tokenizer_token_name(compiler->curr.type),
                       tokenizer_token_name(type));
        return COMPILER_ERROR_UNEXPECTED_TOKEN;
    }

    advance(compiler);
    return COMPILER_ERROR_NONE;
}

void compiler_init(compiler_t *compiler, tokenizer_t *tokenizer, program_t *program)
{
    compiler->tokenizer = tokenizer;
    compiler->program = program;
    compiler->curr = tokenizer_next(tokenizer);
}

void compiler_error(compiler_t *compiler, const char *fmt, ...)
{
    (void) compiler;

    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[COMPILER] ERROR: ");
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}

compiler_error_t compiler_run(compiler_t *compiler, const char *source, program_t *program)
{
    tokenizer_t tokenizer;
    tokenizer_init(&tokenizer, source);
    compiler_init(compiler, &tokenizer, program);

    compiler_error_t error;

    if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
        return error;

    if ((error = consume(compiler, TOKEN_EOF)) != 0)
        return error;

    program_write(compiler->program, OP_RETURN);

    return COMPILER_ERROR_NONE;
}
