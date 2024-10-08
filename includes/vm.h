#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "common.h"
#include "program.h"
#include "value.h"
#include "table.h"

#ifndef CLOX_FRAMES_MAX
#define CLOX_FRAMES_MAX 64
#endif // CLOX_FRAMES_MAX

typedef struct call_frame
{
    object_function_t *function;
    uint8_t *ip;
    value_t *fp;
} call_frame_t;

typedef struct call_frames
{
    call_frame_t items[CLOX_FRAMES_MAX];
    size_t count;
} call_frames_t;

typedef struct vm
{
    call_frames_t frames;
    value_stack_t stack;
    table_t globals;
} vm_t;

void vm_init(vm_t *);
void vm_error(vm_t *, const char *fmt, ...);
interpret_result_t vm_interpret(vm_t *, object_function_t *);
void vm_free(vm_t *);

#endif // CLOX_VM_H
