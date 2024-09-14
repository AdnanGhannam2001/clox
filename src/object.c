#include "object.h"

// TODO: Consider moving this to the memory management part in the future
object_t *object_new(const object_type_t type, const size_t type_size)
{
    object_t *object = (object_t *)malloc(type_size);
    object->type = type;
    return object;
}

void object_destroy(object_t *object)
{
    free(object);
}

cmp_t object_cmp(const object_t *a, const object_t *b)
{
    if (a->type != b->type)
        return CMP_ERROR;
    
    switch(a->type)
    {
    case OBJECT_STRING:
        {
            const object_string_t* s1 = (const object_string_t*)a;
            const object_string_t* s2 = (const object_string_t*)b;
            return s1->length == s2->length && memcmp(s1->data, s2->data, s1->length) == 0
                       ? CMP_EQUAL
                       : CMP_NOT_EQUAL;
        }
    default:
        UNREACHABLE;
    }
}

void object_print(const object_t *object)
{
    switch (object->type)
    {
    case OBJECT_STRING:
        {
            const object_string_t *string = (const object_string_t *)object;
            printf("'%.*s' ", (int)string->length, string->data);
        } break;
    default:
        UNREACHABLE;
    }
}

object_string_t *object_string_new(const char *data, const size_t length)
{
    object_string_t *string = (object_string_t *)object_new(OBJECT_STRING, sizeof(object_string_t));
    string->length = length;
    string->data = (char *)malloc(sizeof(char) * length);
    memcpy(string->data, data, length);
    return string;
}

void object_string_destroy(object_string_t *object_string)
{
    free(object_string->data);
    object_destroy((object_t *)object_string);
}

object_string_t *object_string_concat(object_string_t *a, object_string_t *b)
{
    object_string_t *string = (object_string_t *)object_new(OBJECT_STRING, sizeof(object_string_t));
    string->length = a->length + b->length;
    string->data = (char *)malloc(sizeof(char) * string->length);
    memcpy(string->data, b->data, b->length);
    memcpy(string->data + b->length, a->data, a->length);

    object_string_destroy(a);
    object_string_destroy(b);

    return string;
}
