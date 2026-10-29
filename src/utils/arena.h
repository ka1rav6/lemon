#ifndef LEMON_ARENA_H
#define LEMON_ARENA_H
#include "./common.h"

#define ARENA_ALIGNMENT 8

typedef struct _arena {
    size_t size ;
    size_t used ;
    char*  start;
}Arena;

char* ask_memory(size_t size);

// constructors and destructors 
Arena* arena_create(size_t size);
void   arena_destroy(Arena* ar);

void*  arena_alloc(Arena* ar, size_t size);
char*  arena_strdup(Arena* ar, const char* s);
void   arena_reset(Arena* ar);


#endif // LEMON_ARENA_H
