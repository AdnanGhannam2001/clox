#ifndef CLOX_TOKENIZER_H
#define CLOX_TOKENIZER_H

#include "common.h"

typedef enum token_type
{
    // Single-character tokens.
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

    // One or two character tokens.
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,

    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

    // Keywords.
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

    TOKEN_ERROR, TOKEN_EOF,

    TOKEN_COUNT
} token_type_t;

typedef struct token
{
    token_type_t type;
    const char *start;
    size_t length;
    size_t line;
} token_t;

typedef struct tokenizer
{
    const char *start;
    const char *current;
    size_t line;
} tokenizer_t;

typedef struct tokenizer_context
{
    tokenizer_t *tokenizer;
    token_t curr;
    token_t prev;
} tokenizer_context_t;

void tokenizer_init(tokenizer_t *, const char *source);
token_t tokenizer_next(tokenizer_t *);
const char* tokenizer_token_name(const token_type_t);
bool tokenizer_token_cmp(const token_t, const token_t);

#endif // CLOX_TOKENIZER_H
