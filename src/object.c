#include "object.h"

// TODO: Consider moving this to the memory management part in the future
object_t *object_new(const object_type_t type, const size_t type_size)
{
    object_t *object = (object_t *)malloc(type_size);
    object->type = type;
    return object;
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
