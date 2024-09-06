#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "common.h"
#include "program.h"
#include "value_stack.h"

typedef struct vm
{
    program_t *program;
    uint8_t *ip;
    value_stack_t stack;
} vm_t;

void vm_init(vm_t *);
interpret_result_t vm_interpret(vm_t *, program_t *program);
void vm_free(vm_t *);

#endif // CLOX_VM_H