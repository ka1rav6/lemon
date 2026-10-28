#ifndef LEMON_ARENA_H
#define LEMON_ARENA_H
#include "./common.h"
#include <pthread.h>

#define ARENA_ALIGNMENT 8

typedef struct _arena {
    size_t size;
    size_t used;
    char*  start;
    pthread_mutex_t lock;   // protects 'used' (and the bump pointer) across threads 
}Arena;

/* Internal-only helper, not part of the public API */
char* ask_memory(size_t size);

Arena* createArena(size_t size);
void*  arena_alloc(Arena* ar, size_t size);
char*  arena_strdup(Arena* ar, const char* s);
void   arena_reset(Arena* ar);
void   destroyArena(Arena* ar);


#endif // LEMON_ARENA_H
