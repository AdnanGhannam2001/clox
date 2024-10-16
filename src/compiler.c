#include "compiler.h"

static program_t *executing_program(compiler_t *);

static compiler_error_t declaration(compiler_t *);
static compiler_error_t function_declaration(compiler_t *);
static compiler_error_t var_declaration(compiler_t *);
static compiler_error_t statement(compiler_t *);
static compiler_error_t statement_if(compiler_t *);
static compiler_error_t statement_return(compiler_t *);
static compiler_error_t statement_while(compiler_t *);
static compiler_error_t statement_expression(compiler_t *);
static compiler_error_t block(compiler_t *);
static compiler_error_t expression(compiler_t *, precedence_t);

static compiler_error_t variable(compiler_t *, bool);
static compiler_error_t binary(compiler_t *, UNUSED bool);
static compiler_error_t unary(compiler_t *, UNUSED bool);
static compiler_error_t grouping(compiler_t *, UNUSED bool);
static compiler_error_t literal(compiler_t *, UNUSED bool);
static compiler_error_t call(compiler_t *, UNUSED bool);
static compiler_error_t and_(compiler_t *, UNUSED bool);
static compiler_error_t or_(compiler_t *, UNUSED bool);

static void advance(compiler_t *);
static bool consume_if(compiler_t *, const token_type_t);
static compiler_error_t consume(compiler_t *, const token_type_t);
static inline token_t curr_token(compiler_t *);
static inline token_t prev_token(compiler_t *);

static void begin_scope(compiler_t *);
static void end_scope(compiler_t *);
static bool is_global_scope(compiler_t *);
static void add_local(compiler_t *, token_t);
static int get_local(compiler_t *, token_t);
static void define_variable(compiler_t *, token_t);
static void remove_local(compiler_t *);

static void patch_jump_to(compiler_t *, int, int);
static void patch_jump(compiler_t *, int);

static const rule_t rules[] =
{
  [TOKEN_LEFT_PAREN]    = {grouping, call,   PREC_CALL},
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
  [TOKEN_STRING]        = {literal,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {literal,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
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

static program_t *executing_program(compiler_t *compiler)
{
    return &compiler->context->function->program;
}

static compiler_error_t declaration(compiler_t *compiler)
{
    if (consume_if(compiler, TOKEN_FUN))
        return function_declaration(compiler);
    if (consume_if(compiler, TOKEN_VAR))
        return var_declaration(compiler);
    
    return statement(compiler);
}

static compiler_error_t function_declaration(compiler_t *compiler)
{
    compiler_error_t error;
    if ((error = consume(compiler, TOKEN_IDENTIFIER)) != 0)
        return error;

    token_t function_name = prev_token(compiler);

    char value[function_name.length + 1];
    value[function_name.length] = 0;
    strncpy(value, function_name.start, function_name.length);

    compiler_context_t *context = compiler_context_new(compiler->context, value);
    compiler->context = context;
    begin_scope(compiler);

    if ((error = consume(compiler, TOKEN_LEFT_PAREN)) != 0)
        return error;

    if (curr_token(compiler).type != TOKEN_RIGHT_PAREN)
    {
        do
        {
            if (compiler->context->function->arity++ > CLOX_LOCALS_MAX)
            {
                compiler_error(compiler, "Can't have more than %d parameters.", CLOX_LOCALS_MAX);
                return COMPILER_ERROR_UNEXPECTED_TOKEN;
            }

            if ((error = consume(compiler, TOKEN_IDENTIFIER)) != 0)
                return error;

            token_t param_name = prev_token(compiler);
            define_variable(compiler, param_name);
        } while (consume_if(compiler, TOKEN_COMMA));
    }

    compiler->context->function->name = object_string_new(function_name.start, function_name.length);

    if ((error = consume(compiler, TOKEN_RIGHT_PAREN)) != 0)
        return error;
    if ((error = consume(compiler, TOKEN_LEFT_BRACE)) != 0)
        return error;
    if ((error = block(compiler)) != 0)
        return error;

    compiler->context = context->enclosing;
    object_function_t *function = compiler_context_destroy(context);
    program_write(executing_program(compiler), OP_CONSTANT, OBJECT_VAL(function));
    define_variable(compiler, function_name);

    return error;
}

static compiler_error_t var_declaration(compiler_t *compiler)
{
    compiler_error_t error;
    if ((error = consume(compiler, TOKEN_IDENTIFIER)) != 0)
        return error;

    token_t var = prev_token(compiler);

    if (consume_if(compiler, TOKEN_EQUAL))
    {
        if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
            return error;
    }
    else
        program_write(executing_program(compiler), OP_NIL);

    if ((error = consume(compiler, TOKEN_SEMICOLON)) != 0)
        return error;

    define_variable(compiler, var);

    return error;
}

static compiler_error_t statement(compiler_t *compiler)
{
    compiler_error_t error;
    if (consume_if(compiler, TOKEN_PRINT))
    {
        error = statement_expression(compiler);
        program_write(executing_program(compiler), OP_PRINT);
    }
    else if (consume_if(compiler, TOKEN_LEFT_BRACE))
    {
        begin_scope(compiler);
            error = block(compiler);
        end_scope(compiler);
    }
    else if (consume_if(compiler, TOKEN_IF))
    {
        error = statement_if(compiler);
    }
    else if (consume_if(compiler, TOKEN_RETURN))
    {
        error = statement_return(compiler);
    }
    else if (consume_if(compiler, TOKEN_WHILE))
    {
        error = statement_while(compiler);
    }
    else
    {
        error = statement_expression(compiler);
        program_write(executing_program(compiler), OP_POP);
    }

    return error;
}

static compiler_error_t statement_if(compiler_t *compiler)
{
    compiler_error_t error = COMPILER_ERROR_NONE;

    if ((error = consume(compiler, TOKEN_LEFT_PAREN)) != 0)
        return error;

    if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
        return error;

    if ((error = consume(compiler, TOKEN_RIGHT_PAREN)) != 0)
        return error;

    int if_jump = program_write(executing_program(compiler), OP_JUMP_IF_FALSE);

    if ((error = statement(compiler)) != 0)
        return error;

    int else_jump = program_write(executing_program(compiler), OP_JUMP);
    patch_jump(compiler, if_jump);

    if (consume_if(compiler, TOKEN_ELSE))
    {
        if ((error = statement(compiler)) != 0)
            return error;
    }
    patch_jump(compiler, else_jump);
    program_write(executing_program(compiler), OP_POP);

    return error;
}

static compiler_error_t statement_return(compiler_t *compiler)
{
    if (strncmp(CLOX_MAIN_FN, compiler->context->function->name->data, compiler->context->function->name->length) == 0)
    {
        compiler_error(compiler, "Can't have a 'return statement' in the main function.");
        return COMPILER_ERROR_UNEXPECTED_TOKEN;
    }

    if (curr_token(compiler).type == TOKEN_SEMICOLON)
    {
        program_write(executing_program(compiler), OP_NIL);
        program_write(executing_program(compiler), OP_RETURN);
    }
    else
    {
        expression(compiler, PREC_ASSIGNMENT);
        program_write(executing_program(compiler), OP_RETURN);
    }

    return consume(compiler, TOKEN_SEMICOLON);
}

static compiler_error_t statement_while(compiler_t *compiler)
{
    compiler_error_t error = COMPILER_ERROR_NONE;

    if ((error = consume(compiler, TOKEN_LEFT_PAREN)) != 0)
        return error;

    int condition_ptr = (int)executing_program(compiler)->chunks.count;
    if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
        return error;

    if ((error = consume(compiler, TOKEN_RIGHT_PAREN)) != 0)
        return error;

    int while_jump = program_write(executing_program(compiler), OP_JUMP_IF_FALSE);

    if ((error = statement(compiler)) != 0)
        return error;

    program_write(executing_program(compiler), OP_POP);
    int repeat_jump = program_write(executing_program(compiler), OP_JUMP);
    patch_jump_to(compiler, repeat_jump, condition_ptr);

    patch_jump(compiler, while_jump);
    program_write(executing_program(compiler), OP_POP);

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
    while (!(closed = consume_if(compiler, TOKEN_RIGHT_BRACE)) && curr_token(compiler).type != TOKEN_EOF)
    {
        declaration(compiler);
    }

    if (!closed)
    {
        compiler_error(compiler, "Unexpected token '%s', expected '%s'",
                       tokenizer_token_name(curr_token(compiler).type),
                       tokenizer_token_name(TOKEN_RIGHT_BRACE));
        error = COMPILER_ERROR_UNEXPECTED_TOKEN;
    }

    return error;
}

static compiler_error_t expression(compiler_t *compiler, precedence_t precedence)
{
    compiler_error_t error;

    advance(compiler);
    parse_fn prefix = rules[prev_token(compiler).type].prefix;

    if (prefix == NULL)
    {
        compiler_error(compiler, "Expected expression");
        error = COMPILER_ERROR_EXPRESSION_EXPECTED;
        goto out;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    if ((error = prefix(compiler, can_assign) != 0))
        goto out;

    while (precedence <= rules[curr_token(compiler).type].precedence)
    {
        advance(compiler);
        if ((error = rules[prev_token(compiler).type].infix(compiler, can_assign)) != 0)
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
    token_t var = prev_token(compiler);

    int local_index = get_local(compiler, var);

    if (can_assign && consume_if(compiler, TOKEN_EQUAL))
    {
        if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
            return error;

        if (local_index == -1)
            program_write(executing_program(compiler),
                          OP_SET_GLOBAL,
                          OBJECT_VAL(object_string_new(var.start, var.length)));
        else
            program_write(executing_program(compiler),
                          OP_SET_LOCAL,
                          NUMBER_VAL(local_index));
    }
    else
    {
        if (local_index == -1)
            program_write(executing_program(compiler),
                          OP_GET_GLOBAL,
                          OBJECT_VAL(object_string_new(var.start, var.length)));
        else
            program_write(executing_program(compiler),
                          OP_GET_LOCAL,
                          NUMBER_VAL(local_index));
    }

    return COMPILER_ERROR_NONE;
}

static compiler_error_t binary(compiler_t *compiler, UNUSED bool can_assign)
{
    compiler_error_t error;
    token_type_t op = prev_token(compiler).type;

    rule_t rule = rules[op];
    if ((error = expression(compiler, rule.precedence + 1)) != 0)
        return error;

    switch (op)
    {
        case TOKEN_PLUS:  { program_write(executing_program(compiler), OP_ADD); } break;
        case TOKEN_MINUS: { program_write(executing_program(compiler), OP_SUB); } break;
        case TOKEN_STAR:  { program_write(executing_program(compiler), OP_MULTI); } break;
        case TOKEN_SLASH: { program_write(executing_program(compiler), OP_DIV); } break;

        case TOKEN_EQUAL_EQUAL: { program_write(executing_program(compiler), OP_EQUAL); } break;
        case TOKEN_GREATER:     { program_write(executing_program(compiler), OP_GREATER); } break;
        case TOKEN_LESS:        { program_write(executing_program(compiler), OP_LESS); } break;
        case TOKEN_BANG_EQUAL:
            {
                program_write(executing_program(compiler), OP_EQUAL);
                program_write(executing_program(compiler), OP_NOT);
            } break;
        case TOKEN_GREATER_EQUAL:
            {
                program_write(executing_program(compiler), OP_GREATER);
                program_write(executing_program(compiler), OP_NOT);
            } break;
        case TOKEN_LESS_EQUAL:
            {
                program_write(executing_program(compiler), OP_LESS);
                program_write(executing_program(compiler), OP_NOT);
            } break;

        default:
            UNREACHABLE;
    }

    return COMPILER_ERROR_NONE;
}

static compiler_error_t unary(compiler_t *compiler, UNUSED bool can_assign)
{
    compiler_error_t error;
    token_type_t op = prev_token(compiler).type;

    if ((error = expression(compiler, PREC_UNARY)) != 0)
        return error;

    switch (op)
    {
        case TOKEN_MINUS: { program_write(executing_program(compiler), OP_NEGATE); } break;
        case TOKEN_BANG:  { program_write(executing_program(compiler), OP_NOT); } break;
        default:
            UNREACHABLE;
    }

    return COMPILER_ERROR_NONE;
}

static compiler_error_t grouping(compiler_t *compiler, UNUSED bool can_assign)
{
    compiler_error_t error;
    if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
        return error;

    if ((error = consume(compiler, TOKEN_RIGHT_PAREN)) != 0)
        return error;

    return COMPILER_ERROR_NONE;
}

static compiler_error_t literal(compiler_t *compiler, UNUSED bool can_assign)
{
    switch(prev_token(compiler).type)
    {
        case TOKEN_NUMBER:
            {
                if (program_write(executing_program(compiler), OP_CONSTANT, NUMBER_VAL(strtod(prev_token(compiler).start, NULL))) < 0)
                    return COMPILER_ERROR_OUT_OF_MEMORY;
            } break;
        case TOKEN_NIL:
            {
                if (program_write(executing_program(compiler), OP_NIL))
                    return COMPILER_ERROR_OUT_OF_MEMORY;
            } break;
        case TOKEN_TRUE:
            {
                if (program_write(executing_program(compiler), OP_TRUE))
                    return COMPILER_ERROR_OUT_OF_MEMORY;
            } break;
        case TOKEN_FALSE:
            {
                if (program_write(executing_program(compiler), OP_FALSE))
                    return COMPILER_ERROR_OUT_OF_MEMORY;
            } break;
        case TOKEN_STRING:
            {
                if (program_write(executing_program(compiler),
                                  OP_CONSTANT,
                                  OBJECT_VAL(object_string_new(prev_token(compiler).start + 1, prev_token(compiler).length - 2))))
                    return COMPILER_ERROR_OUT_OF_MEMORY;
            } break;
        default:
            UNREACHABLE;
    }

    return COMPILER_ERROR_NONE;
}

static compiler_error_t call(compiler_t *compiler, UNUSED bool can_assign)
{
    compiler_error_t error;
    uint8_t args_count = 0;

    if (curr_token(compiler).type != TOKEN_RIGHT_PAREN)
    {
        do
        {
            if (compiler->context->function->arity++ > CLOX_LOCALS_MAX)
            {
                compiler_error(compiler, "Can't have more than %d parameters.", CLOX_LOCALS_MAX);
                return COMPILER_ERROR_UNEXPECTED_TOKEN;
            }

            if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
                return error;
            args_count++;
        } while (consume_if(compiler, TOKEN_COMMA));
    }

    if ((error = consume(compiler, TOKEN_RIGHT_PAREN)) != 0)
        return error;

    program_write(executing_program(compiler), OP_CALL, args_count);

    return error;
}

static compiler_error_t and_(compiler_t *compiler, UNUSED bool can_assign)
{
    compiler_error_t error = COMPILER_ERROR_NONE;
    int jump = program_write(executing_program(compiler), OP_JUMP_IF_FALSE);

    if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
        return error;

    program_write(executing_program(compiler), OP_POP);
    patch_jump(compiler, jump);

    return error;
}

static compiler_error_t or_(compiler_t *compiler, UNUSED bool can_assign)
{
    compiler_error_t error = COMPILER_ERROR_NONE;
    int jump_if_false = program_write(executing_program(compiler), OP_JUMP_IF_FALSE);
    int jump = program_write(executing_program(compiler), OP_JUMP);

    patch_jump(compiler, jump_if_false);
    program_write(executing_program(compiler), OP_POP);

    if ((error = expression(compiler, PREC_ASSIGNMENT)) != 0)
        return error;

    patch_jump(compiler, jump);

    return error;
}

static inline token_t curr_token(compiler_t *compiler)
{
    return compiler->tokenizer_context.curr;
}

static inline token_t prev_token(compiler_t *compiler)
{
    return compiler->tokenizer_context.prev;
}

static void advance(compiler_t *compiler)
{
    compiler->tokenizer_context.prev = curr_token(compiler);
    compiler->tokenizer_context.curr = tokenizer_next(compiler->tokenizer_context.tokenizer);
}

static bool consume_if(compiler_t *compiler, const token_type_t type)
{
    if (curr_token(compiler).type != type)
        return false;

    advance(compiler);
    return true;
}

static compiler_error_t consume(compiler_t *compiler, const token_type_t type)
{
    if (!consume_if(compiler, type))
    {
        compiler_error(compiler, "Unexpected token '%s', expected '%s'",
                       tokenizer_token_name(curr_token(compiler).type),
                       tokenizer_token_name(type));
        return COMPILER_ERROR_UNEXPECTED_TOKEN;
    }

    return COMPILER_ERROR_NONE;
}

static void begin_scope(compiler_t *compiler)
{
    compiler->context->locals.depth++;
}

static void end_scope(compiler_t *compiler)
{
    compiler_locals_t *locals = &compiler->context->locals;
    locals->depth--;

    while (locals->count > 0 && locals->items[locals->count - 1].depth > locals->depth)
        remove_local(compiler);
}

static bool is_global_scope(compiler_t *compiler)
{
    return compiler->context->locals.depth == 0;
}

static void add_local(compiler_t *compiler, token_t var)
{
    compiler_locals_t *locals = &compiler->context->locals;
    locals->items[locals->count++] = (compiler_local_t){var, locals->depth};
}

static int get_local(compiler_t *compiler, token_t var)
{
    for (int i = (int)compiler->context->locals.count - 1; i >= 0; --i)
        if (tokenizer_token_cmp(compiler->context->locals.items[i].token, var))
            return i;

    return -1;
}

static void define_variable(compiler_t *compiler, token_t token)
{
    if (is_global_scope(compiler))
        program_write(executing_program(compiler), OP_DEFINE_GLOBAL, OBJECT_VAL(object_string_new(token.start, token.length)));
    else
        add_local(compiler, token);
}

static void remove_local(compiler_t *compiler)
{
    compiler->context->locals.count--;
    program_write(executing_program(compiler), OP_POP);
}

static void patch_jump_to(compiler_t *compiler, int offset, int to)
{
    executing_program(compiler)->chunks.items[offset + 0] = (chunk)((to >> 8) & 0xFF);
    executing_program(compiler)->chunks.items[offset + 1] = (chunk)((to >> 0) & 0xFF);
}

static void patch_jump(compiler_t *compiler, int offset)
{
    patch_jump_to(compiler, offset, (int)executing_program(compiler)->chunks.count);
}

void compiler_init(compiler_t *compiler, tokenizer_t *tokenizer)
{
    compiler->tokenizer_context = (tokenizer_context_t){
        .tokenizer = tokenizer,
        .curr = tokenizer_next(tokenizer)};

    compiler->context = compiler_context_new(NULL, CLOX_MAIN_FN);
}

void compiler_free(compiler_t *compiler)
{
    object_function_destroy(compiler_context_destroy(compiler->context));
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

compiler_error_t compiler_run(compiler_t *compiler, const char *source)
{
    tokenizer_t tokenizer;
    tokenizer_init(&tokenizer, source);
    compiler_init(compiler, &tokenizer);

    compiler_error_t error;

    while (curr_token(compiler).type != TOKEN_EOF)
    {
        if ((error = declaration(compiler)) != 0)
            return error;
    }

    if ((error = consume(compiler, TOKEN_EOF)) != 0)
        return error;

    program_write(executing_program(compiler), OP_NIL);
    program_write(executing_program(compiler), OP_RETURN);

    return COMPILER_ERROR_NONE;
}

compiler_context_t *compiler_context_new(compiler_context_t *enclosing, const char *function_name)
{
    compiler_context_t *context = NULL;
    context = memory_allocate(context, sizeof(compiler_context_t), false);
    *context = (compiler_context_t){
        .enclosing = enclosing,
        .function = object_function_new(function_name, 0),
        .locals = (compiler_locals_t){0}};
    
    return context;
}

object_function_t *compiler_context_destroy(compiler_context_t *context)
{
    object_function_t *function = context->function;

    program_write(&function->program, OP_NIL);
    program_write(&function->program, OP_RETURN);

    memory_free(context);
    return function;
}
