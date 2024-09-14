#include "tokenizer.h"

static token_t token(tokenizer_t *tokenizer, const token_type_t type)
{
    token_t token = (token_t){
        .type = type,
        .start = tokenizer->start,
        .length = (size_t)(tokenizer->current - tokenizer->start),
        .line = tokenizer->line};

    tokenizer->start = tokenizer->current;

    return token;
}

static bool advance_if_equals(tokenizer_t *tokenizer, const char c)
{
    if (*tokenizer->current == c)
    {
        tokenizer->current++;
        return true;
    }

    return false;
}

static bool is_digit(const char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(const char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_alpha_numeric(const char c)
{
    return is_digit(c) || is_alpha(c);
}

static token_type_t keyword(const char* name)
{
    if (strcmp(name, "and") == 0) return TOKEN_AND;
    if (strcmp(name, "class") == 0) return TOKEN_CLASS;
    if (strcmp(name, "else") == 0) return TOKEN_ELSE;
    if (strcmp(name, "false") == 0) return TOKEN_FALSE;
    if (strcmp(name, "for") == 0) return TOKEN_FOR;
    if (strcmp(name, "fun") == 0) return TOKEN_FUN;
    if (strcmp(name, "if") == 0) return TOKEN_IF;
    if (strcmp(name, "nil") == 0) return TOKEN_NIL;
    if (strcmp(name, "or") == 0) return TOKEN_OR;
    if (strcmp(name, "print") == 0) return TOKEN_PRINT;
    if (strcmp(name, "return") == 0) return TOKEN_RETURN;
    if (strcmp(name, "super") == 0) return TOKEN_SUPER;
    if (strcmp(name, "this") == 0) return TOKEN_THIS;
    if (strcmp(name, "true") == 0) return TOKEN_TRUE;
    if (strcmp(name, "var") == 0) return TOKEN_VAR;
    if (strcmp(name, "while") == 0) return TOKEN_WHILE;

    return TOKEN_ERROR;
}

static token_t number(tokenizer_t *tokenizer)
{
    while (is_digit(*tokenizer->current)) { tokenizer->current++; }
    return token(tokenizer, TOKEN_NUMBER);
}

static token_t identifier(tokenizer_t *tokenizer)
{
    while (is_alpha_numeric(*tokenizer->current)) { tokenizer->current++; }

    char name[tokenizer->current - tokenizer->start];
    strncpy(name, tokenizer->start, (size_t)(tokenizer->current - tokenizer->start + 1));
    name[tokenizer->current - tokenizer->start] = '\0';

    token_type_t kw = keyword(name);
    if (kw != TOKEN_ERROR)
    {
        return token(tokenizer, kw);
    }

    return token(tokenizer, TOKEN_IDENTIFIER);
}

static token_t string(tokenizer_t *tokenizer)
{
    while (*tokenizer->current != '\0')
    {
        if (advance_if_equals(tokenizer, '"'))
        {
            return token(tokenizer, TOKEN_STRING);
        }

        tokenizer->current++;
    }

    return token(tokenizer, TOKEN_ERROR);
}

void tokenizer_init(tokenizer_t *tokenizer, const char *source)
{
    tokenizer->start = source;
    tokenizer->current = source;
    tokenizer->line = 1;
}

const char* tokenizer_token_name(const token_type_t type)
{
    if (type > TOKEN_COUNT)
        NOT_IMPLEMENTED;

    switch(type)
    {
        case TOKEN_LEFT_PAREN:    { return "("; }
        case TOKEN_RIGHT_PAREN:   { return ")"; }
        case TOKEN_LEFT_BRACE:    { return "{"; }
        case TOKEN_RIGHT_BRACE:   { return "}"; }
        case TOKEN_COMMA:         { return ","; }
        case TOKEN_DOT:           { return "."; }
        case TOKEN_MINUS:         { return "-"; }
        case TOKEN_PLUS:          { return "+"; }
        case TOKEN_SEMICOLON:     { return ";"; }
        case TOKEN_SLASH:         { return "/"; }
        case TOKEN_STAR:          { return "*"; }
        case TOKEN_BANG:          { return "!"; }
        case TOKEN_BANG_EQUAL:    { return "!="; }
        case TOKEN_EQUAL:         { return "="; }
        case TOKEN_EQUAL_EQUAL:   { return "=="; }
        case TOKEN_GREATER:       { return "<"; }
        case TOKEN_GREATER_EQUAL: { return "<="; }
        case TOKEN_LESS:          { return ">"; }
        case TOKEN_LESS_EQUAL:    { return ">="; }
        case TOKEN_IDENTIFIER:    { return "Identifier"; }
        case TOKEN_STRING:        { return "String"; }
        case TOKEN_NUMBER:        { return "Number"; }
        case TOKEN_AND:           { return "and"; }
        case TOKEN_CLASS:         { return "class"; }
        case TOKEN_ELSE:          { return "else"; }
        case TOKEN_FALSE:         { return "false"; }
        case TOKEN_FOR:           { return "for"; }
        case TOKEN_FUN:           { return "fun"; }
        case TOKEN_IF:            { return "if"; }
        case TOKEN_NIL:           { return "nil"; }
        case TOKEN_OR:            { return "or"; }
        case TOKEN_PRINT:         { return "print"; }
        case TOKEN_RETURN:        { return "return"; }
        case TOKEN_SUPER:         { return "super"; }
        case TOKEN_THIS:          { return "this"; }
        case TOKEN_TRUE:          { return "true"; }
        case TOKEN_VAR:           { return "var"; }
        case TOKEN_WHILE:         { return "while"; }
        case TOKEN_ERROR:         { return "Error"; }
        case TOKEN_EOF:           { return "EoF"; }
        default: UNREACHABLE;
    }
}

token_t tokenizer_next(tokenizer_t *tokenizer)
{
    while (true)
    {
        const char c = *tokenizer->current++;

        switch (c)
        {
            case '\0': { return token(tokenizer, TOKEN_EOF); }

            case '\n':
                tokenizer->line++;
                continue;
            case ' ':
            case '\t':
            case '\r':
                tokenizer->start++;
                continue;

            case '(': { return token(tokenizer, TOKEN_LEFT_PAREN); }
            case ')': { return token(tokenizer, TOKEN_RIGHT_PAREN); }
            case '{': { return token(tokenizer, TOKEN_LEFT_BRACE); }
            case '}': { return token(tokenizer, TOKEN_RIGHT_BRACE); }
            case ',': { return token(tokenizer, TOKEN_COMMA); }
            case '.': { return token(tokenizer, TOKEN_DOT); }
            case '-': { return token(tokenizer, TOKEN_MINUS); }
            case '+': { return token(tokenizer, TOKEN_PLUS); }
            case ';': { return token(tokenizer, TOKEN_SEMICOLON); }
            case '*': { return token(tokenizer, TOKEN_STAR); }

            case '/':
                {
                    if (advance_if_equals(tokenizer, '/'))
                    {
                        while (*tokenizer->current != '\0' && *tokenizer->current != '\n') { tokenizer->current++; }
                        continue;
                    }

                    return token(tokenizer, TOKEN_SLASH);
                }

            case '!': { return token(tokenizer, advance_if_equals(tokenizer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG ); }
            case '=': { return token(tokenizer, advance_if_equals(tokenizer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL); }
            case '<': { return token(tokenizer, advance_if_equals(tokenizer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER); }
            case '>': { return token(tokenizer, advance_if_equals(tokenizer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS); }

            default:
            {
                if (is_digit(c))
                {
                    return number(tokenizer);
                }

                if (is_alpha(c))
                {
                    return identifier(tokenizer);
                }

                if (c == '"')
                {
                    return string(tokenizer);
                }

                return token(tokenizer, TOKEN_ERROR);
            }
        }
    }
}
