#include "table.h"

static bool table_entry_empty(const entry_t *entry)
{
    return entry->key == NULL;
}

static bool table_entry_deleted(const entry_t *entry)
{
    return table_entry_empty(entry) && entry->value.type != VAL_NIL;
}

static uint32_t table_hash(const entry_key_t key)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < key->length; ++i)
    {
        hash ^= (uint8_t)key->data[i];
        hash *= 16777619;
    }
    return hash;
}

void table_move(table_t *to, entry_t *from, const size_t n)
{
    entry_t *entry;

    for (size_t i = 0; i < n; ++i)
    {
        entry = &from[i];
        if (!table_entry_empty(entry))
            table_entry_set(to, entry->key, entry->value);
    }

    memory_free(from);
}

void table_expand(table_t *table, const size_t capacity)
{
    assert(table->capacity < capacity && "Not allowed to shrink table");

    if (capacity > 0)
    {
        entry_t *old_entries = table->entries;
        table->entries = (entry_t *)memory_allocate(table->entries, GROW_CAPACITY(capacity * sizeof(entry_t)), true);
        table_move(table, old_entries, table->capacity);
        table->capacity = capacity;
    }
}

void table_init(table_t *table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;

    table_expand(table, 10);
}

entry_t *table_entry_get(const table_t *table, const entry_key_t key)
{
    entry_hash_t hash = table_hash(key) % table->capacity;
    entry_t *deleted = NULL;
    entry_t *entry;

    while (1)
    {
        entry = &table->entries[hash];

        if (table_entry_empty(entry))
            if (table_entry_deleted(entry))
                deleted = entry;
            else
                return deleted == NULL ? entry : deleted;
        else if (object_string_cmp(key, entry->key))
            return entry;

        hash = (hash + 1) % table->capacity;
    }
}

bool table_entry_set(table_t *table, const entry_key_t key, const value_t value)
{
    entry_t *entry = table_entry_get(table, key);
    if (table_entry_empty(entry))
    {
        entry->key = key;
        entry->value = value;
        table->count++;
        return true;
    }

    entry->value = value;
    return false;
}

bool table_entry_delete(table_t *table, const entry_key_t key)
{
    entry_t *entry;
    if ((entry = table_entry_get(table, key)) == NULL)
        return false;

    object_string_destroy(entry->key);
    entry->key = NULL;
    entry->value = NIL_VAL;
    table->count--;

    return true;
}

void table_free(table_t *table)
{
    memory_free(table->entries);
    table_init(table);
}

void table_print(const table_t *table)
{
    printf("[\n");
    for (size_t i = 0; i < table->capacity; ++i)
    {
        if (table->entries[i].key == NULL)
            continue;

        printf("\t%.*s: ", (int)table->entries[i].key->length, table->entries[i].key->data);
        value_print(table->entries[i].value);
        printf("\n");
    }
    printf("]\n");
}
