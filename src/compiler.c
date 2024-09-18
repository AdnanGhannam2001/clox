#include "compiler.h"

static compiler_error_t declaration(compiler_t *);
static compiler_error_t var_declaration(compiler_t *);
static compiler_error_t statement(compiler_t *);
static compiler_error_t statement_expression(compiler_t *);
static compiler_error_t block(compiler_t *);
static compiler_error_t expression(compiler_t *, precedence_t);

static compiler_error_t variable(compiler_t *, bool);
static compiler_error_t string(compiler_t *, bool);
static compiler_error_t binary(compiler_t *, bool);
static compiler_error_t unary(compiler_t *, bool);
static compiler_error_t grouping(compiler_t *, bool);
static compiler_error_t literal(compiler_t *, bool);

static void advance(compiler_t *);
static bool consume_if(compiler_t *, const token_type_t);
static compiler_error_t consume(compiler_t *, const token_type_t);

static void begin_scope(compiler_t *);
static void end_scope(compiler_t *);
static bool is_global_scope(compiler_t *);
static void add_local(compiler_t *, token_t);
static int get_local(compiler_t *, token_t);
static void remove_local(compiler_t *);

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
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
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

static compiler_error_t declaration(compiler_t *compiler)
{
    if (consume_if(compiler, TOKEN_VAR))
        return var_declaration(compiler);
    
    return statement(compiler);
}

static compiler_error_t var_declaration(compiler_t *compiler)
{
    compiler_error_t error;
    if ((error = consume(compiler, TOKEN_IDENTIFIER)) != 0)
        return error;

    token_t var = compiler->prev;

    if (consume_if(compiler, TOKEN_EQUAL))
    {
        if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
            return error;
    }
    else
        program_write(compiler->program, OP_NIL);

    if ((error = consume(compiler, TOKEN_SEMICOLON)) != 0)
        return error;

    if (is_global_scope(compiler))
        program_write(compiler->program, OP_DEFINE_GLOBAL, OBJECT_VAL(object_string_new(var.start, var.length)));
    else
        add_local(compiler, var);

    return error;
}

static compiler_error_t statement(compiler_t *compiler)
{
    compiler_error_t error;
    if (consume_if(compiler, TOKEN_PRINT))
    {
        error = statement_expression(compiler);
        program_write(compiler->program, OP_PRINT);
    }
    else if (consume_if(compiler, TOKEN_LEFT_BRACE))
    {
        begin_scope(compiler);
            error = block(compiler);
        end_scope(compiler);
    }
    else
    {
        error = statement_expression(compiler);
        program_write(compiler->program, OP_POP);
    }

    return error;
}

static compiler_error_t statement_expression(compiler_t *compiler)
{
    compiler_error_t error;
    if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
        return error;

    if ((error = consume(compiler, TOKEN_SEMICOLON)) != 0)
        return error;

    return error;
}

static compiler_error_t block(compiler_t *compiler)
{
    compiler_error_t error = COMPILER_ERROR_NONE;
    bool closed = false;
    while (!(closed = consume_if(compiler, TOKEN_RIGHT_BRACE)) && compiler->curr.type != TOKEN_EOF)
    {
        declaration(compiler);
    }

    if (!closed)
    {
        compiler_error(compiler, "Unexpected token '%s', expected '%s'",
                       tokenizer_token_name(compiler->curr.type),
                       tokenizer_token_name(TOKEN_RIGHT_BRACE));
        error = COMPILER_ERROR_UNEXPECTED_TOKEN;
    }

    return error;
}

static compiler_error_t expression(compiler_t *compiler, precedence_t precedence)
{
    compiler_error_t error;

    advance(compiler);
    parse_fn prefix = rules[compiler->prev.type].prefix;

    if (prefix == NULL)
    {
        compiler_error(compiler, "Expected expression");
        error = COMPILER_ERROR_EXPRESSION_EXPECTED;
        goto out;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    if ((error = prefix(compiler, can_assign) != 0))
        goto out;

    while (precedence <= rules[compiler->curr.type].precedence)
    {
        advance(compiler);
        if ((error = rules[compiler->prev.type].infix(compiler, can_assign)) != 0)
            goto out;
    }

    if (can_assign && consume_if(compiler, TOKEN_EQUAL))
    {
        compiler_error(compiler, "Invalid assignment");
        error = COMPILER_ERROR_INVALID_ASSIGNMENT;
    }

out:
    return error;
}

static compiler_error_t variable(compiler_t *compiler, bool can_assign)
{
    compiler_error_t error;
    token_t var = compiler->prev;

    int local_index = get_local(compiler, var);

    if (can_assign && consume_if(compiler, TOKEN_EQUAL))
    {
        if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
            return error;

        if (local_index == -1)
            program_write(compiler->program,
                          OP_SET_GLOBAL,
                          OBJECT_VAL(object_string_new(var.start, var.length)));
        else
            program_write(compiler->program,
                          OP_SET_LOCAL,
                          NUMBER_VAL(local_index));
    }
    else
    {
        if (local_index == -1)
            program_write(compiler->program,
                          OP_GET_GLOBAL,
                          OBJECT_VAL(object_string_new(var.start, var.length)));
        else
            program_write(compiler->program,
                          OP_GET_LOCAL,
                          NUMBER_VAL(local_index));
    }

    return COMPILER_ERROR_NONE;
}

static compiler_error_t string(compiler_t *compiler, bool can_assign __attribute__((unused)))
{
    program_write(compiler->program,
        OP_CONSTANT,
        OBJECT_VAL(object_string_new(compiler->prev.start + 1, compiler->prev.length - 2)));
    return COMPILER_ERROR_NONE;
}

static compiler_error_t binary(compiler_t *compiler, bool can_assign __attribute__((unused)))
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

static compiler_error_t unary(compiler_t *compiler, bool can_assign __attribute__((unused)))
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

static compiler_error_t grouping(compiler_t *compiler, bool can_assign __attribute__((unused)))
{
    compiler_error_t error;
    if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
        return error;

    if ((error = consume(compiler, TOKEN_RIGHT_PAREN)) != 0)
        return error;

    return COMPILER_ERROR_NONE;
}

static compiler_error_t literal(compiler_t *compiler, bool can_assign __attribute__((unused)))
{
    switch(compiler->prev.type)
    {
        case TOKEN_NUMBER:
            {
                if (program_write(compiler->program, OP_CONSTANT, NUMBER_VAL(strtod(compiler->prev.start, NULL))) < 0)
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

static void advance(compiler_t *compiler)
{
    compiler->prev = compiler->curr;
    compiler->curr = tokenizer_next(compiler->tokenizer);
}

static bool consume_if(compiler_t *compiler, const token_type_t type)
{
    if (compiler->curr.type != type)
        return false;

    advance(compiler);
    return true;
}

static compiler_error_t consume(compiler_t *compiler, const token_type_t type)
{
    if (!consume_if(compiler, type))
    {
        compiler_error(compiler, "Unexpected token '%s', expected '%s'",
                       tokenizer_token_name(compiler->curr.type),
                       tokenizer_token_name(type));
        return COMPILER_ERROR_UNEXPECTED_TOKEN;
    }

    return COMPILER_ERROR_NONE;
}

static void begin_scope(compiler_t *compiler)
{
    compiler->locals.depth++;
}

static void end_scope(compiler_t *compiler)
{
    compiler->locals.depth--;

    while (compiler->locals.count > 0 && compiler->locals.items[compiler->locals.count - 1].depth > compiler->locals.depth)
        remove_local(compiler);
}

static bool is_global_scope(compiler_t *compiler)
{
    return compiler->locals.depth == 0;
}

static void add_local(compiler_t *compiler, token_t var)
{
    compiler->locals.items[compiler->locals.count++] = (compiler_local_t){var, compiler->locals.depth};
}

static int get_local(compiler_t *compiler, token_t var)
{
    for (int i = (int)compiler->locals.count - 1; i >= 0; --i)
        if (tokenizer_token_cmp(compiler->locals.items[i].token, var))
            return i;

    return -1;
}

static void remove_local(compiler_t *compiler)
{
    compiler->locals.count--;
    program_write(compiler->program, OP_POP);
}

void compiler_init(compiler_t *compiler, tokenizer_t *tokenizer, program_t *program)
{
    compiler->tokenizer = tokenizer;
    compiler->program = program;
    compiler->curr = tokenizer_next(tokenizer);
    compiler->locals = (compiler_locals_t){0};
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

    while (compiler->curr.type != TOKEN_EOF)
    {
        if ((error = declaration(compiler)) != 0)
            return error;
    }

    if ((error = consume(compiler, TOKEN_EOF)) != 0)
        return error;

    program_write(compiler->program, OP_RETURN);

    return COMPILER_ERROR_NONE;
}
