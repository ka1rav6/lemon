#ifndef LEMON_COMMON_H
#define LEMON_COMMON_H

// defining the debug mode
#ifndef DEBUG_MODE
#define DEBUG_MODE 1
#endif

#if DEBUG_MODE
#define NDEBUG

#define LOG(fmt, ...)                                              \
    printf("Line : %d, Function : %s\n", __LINE__, __func__ );     \
    printf(fmt "\n", __VA_ARGS__)

#endif



// standard library imports
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>


#endif
