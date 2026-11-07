#ifndef LEMON_DYNAMIC_ARRAY_H
#define LEMON_DYNAMIC_ARRAY_H

// inspiration: crafting interpreters

/* ----- Standard Library Includes ----- */
#include <stdlib.h>

/* ----- Custom File Includes ----- */
#include "arena.h"
#include "common.h"

// if initial capacity is < 8, make it 8
// else, double it
#define GROW_CAPACITY(capacity)                                                \
    ((capacity) < 8 ? 8 : (capacity) * 2)

// manual arrays: for buffers that resize on their own terms, like a hash
// table rebuilding its buckets
#define GROW_ARRAY(type, ptr, oldCount, newCount)                              \
    (type *)reallocate(ptr, sizeof(type) * (oldCount),                         \
                     sizeof(type) * (newCount), NULL)
#define FREE_ARRAY(type, ptr, _old)                                            \
    reallocate(ptr, sizeof(type) * _old, 0, NULL)

// Push arrays: any struct with `count`, `capacity` and an `items` pointer.
// `arena` NULL means the heap and needs a DA_FREE; an arena array lives until
// the arena dies, and DA_FREE then only zeroes the struct. Pass the same
// `arena` to both macros, or DA_FREE will free() memory the heap never gave.

#define DA_PUSH(da, value, arena)                                              \
    do {                                                                       \
        if ((da).capacity < (da).count + 1) {                                  \
            size_t _old   = (da).capacity;                                     \
            (da).capacity = GROW_CAPACITY(_old);                               \
            (da).items    = reallocate((da).items,                             \
                                    sizeof(*(da).items) * _old,                \
                                    sizeof(*(da).items) * (da).capacity,       \
                                    (arena)                                    \
                                       );                                      \
        }                                                                      \
        (da).items[(da).count++] = (value);                                    \
    } while (0)

#define DA_FREE(da, arena)                                                     \
    do {                                                                       \
        reallocate((da).items, sizeof(*(da).items) * (da).capacity, 0,         \
                   (arena));                                                   \
        (da).items    = NULL;                                                  \
        (da).count    = 0;                                                     \
        (da).capacity = 0;                                                     \
    } while (0)

// helper for the macros
void *reallocate(void *ptr, size_t _old, size_t _new, Arena* ar); 




#endif // LEMON_DYNAMIC_ARRAY_H
