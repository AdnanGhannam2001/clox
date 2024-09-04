#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "common.h"
#include "program.h"
#include "tokenizer.h"

program_t compile(const char *);

#endif // CLOX_COMPILER_H