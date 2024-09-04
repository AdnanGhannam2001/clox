#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define NOT_IMPLEMENTED assert(0 && "Not Implemented")
#define UNREACHABLE     assert(0 && "Unreachable")

#endif // CLOX_COMMON_H