#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include "common.h"
#include "memory.h"
#include "value.h"
#include "object.h"

typedef uint32_t entry_hash_t;
typedef object_string_t *entry_key_t;

typedef struct entry
{
    entry_key_t key;
    value_t value;
} entry_t;

typedef struct table
{
    entry_t *entries;
    size_t count;
    size_t capacity;
} table_t;

void table_move(table_t *to, entry_t *from, const size_t n);
void table_expand(table_t *, const size_t);
void table_init(table_t *);
entry_t *table_entry_get(const table_t *, const entry_key_t);
bool table_entry_set(table_t *, const entry_key_t, const value_t);
bool table_entry_delete(table_t *, const entry_key_t);
void table_free(table_t *);
void table_print(const table_t *);

#endif // CLOX_TABLE_H
