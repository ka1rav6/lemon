#ifndef LEMON_DYNAMIC_ARRAY_H
#define LEMON_DYNAMIC_ARRAY_H

// inspiration: crafting interpreters

#include <stdlib.h>
#include "common.h"

// if initial capacity is < 8, make it 8
// else, double it
#define GROW_CAPACITY(capacity)                                                \
    ((capacity) < 8 ? 8 : (capacity) * 2)

// manual arrays: for buffers that resize on their own terms, like a hash
// table rebuilding its buckets
#define GROW_ARRAY(type, ptr, oldCount, newCount)                              \
    (type *)reallocate(ptr, sizeof(type) * (oldCount),                         \
                     sizeof(type) * (newCount))
#define FREE_ARRAY(type, ptr, _old)                                            \
    reallocate(ptr, sizeof(type) * _old, 0)

// Push arrays: any struct with `count`, `capacity` and an `items` pointer,
#define DA_PUSH(da, value)                                                     \
    do {                                                                       \
        if ((da).capacity < (da).count + 1) {                                  \
            size_t _old   = (da).capacity;                                     \
            (da).capacity = GROW_CAPACITY(_old);                               \
            (da).items    = reallocate((da).items,                             \
                                    sizeof(*(da).items) * _old,                \
                                    sizeof(*(da).items) * (da).capacity);      \
        }                                                                      \
        (da).items[(da).count++] = (value);                                    \
    } while (0)

#define DA_FREE(da)                                                            \
    do {                                                                       \
        reallocate((da).items, sizeof(*(da).items) * (da).capacity, 0);        \
        (da).items    = NULL;                                                  \
        (da).count    = 0;                                                     \
        (da).capacity = 0;                                                     \
    } while (0)

// helper for the macros
void *reallocate(void *ptr, size_t _old, size_t _new); 




#endif // LEMON_DYNAMIC_ARRAY_H
