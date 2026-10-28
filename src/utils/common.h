#ifndef LEMON_COMMON_H
#define LEMON_COMMON_H

// defining the debug mode
#ifndef DEBUG_MODE
#define DEBUG_MODE 1
#endif

#if DEBUG_MODE
// format string lives in __VA_ARGS__ so LOG("msg") needs no trailing comma
#define LOG(...) do {                                               \
        printf("Line : %d, Function : %s\n", __LINE__, __func__);   \
        printf(__VA_ARGS__);                                        \
        putchar('\n');                                              \
    } while (0)
#else
#define NDEBUG
#define LOG(...) ((void)0)
#endif



// standard library imports
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>


#endif
