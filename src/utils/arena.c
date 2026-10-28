#include "arena.h"
#include "common.h"
// aligns the allocator memory to align up 
static inline size_t align_up(size_t n, size_t alignment)
{
  return (n + alignment - 1) & ~(alignment - 1);
}

// asks the system for memory of size "size"
char *ask_memory(size_t size)
{
  void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED)
    return NULL;
  return (char *)ptr;
}

Arena *createArena(size_t size)
{
  if (size == 0)
    return NULL;
  Arena *ar = (Arena *)malloc(sizeof(Arena));

  if (!ar)
    return NULL;
  ar->start = ask_memory(size);

  if (!ar->start) {
    free(ar);
    return NULL  ;
  }
  ar->used = 0   ;
  ar->size = size;
  if (pthread_mutex_init(&ar->lock, NULL) != 0) {
    munmap(ar->start, ar->size);
    free(ar);
    return NULL;
  }
  return ar;
}

void *arena_alloc(Arena *ar, size_t size) 
{
  if (!ar || size == 0)
    return NULL;
  pthread_mutex_lock(&ar->lock);

  size_t aligned = align_up(ar->used, ARENA_ALIGNMENT);
  if (size > ar->size - aligned) {
    pthread_mutex_unlock(&ar->lock);
    return NULL;
  }
  void *loc = ar->start + aligned;
  ar->used = aligned + size;

  pthread_mutex_unlock(&ar->lock);
  return loc;
}

// the same function as normal strdup() but it allocates the new string inside
// the arena and then duplicates the string using memcpy and stores it in the
// arena
char *arena_strdup(Arena *ar, const char *s)
{
  size_t len = strlen(s) + 1;
  // arena_alloc already takes the lock so there is no extra locking needed here.
  char *copy = (char *)arena_alloc(ar, len);
  assert(copy != NULL);
  memcpy(copy, s, len);
  return copy;
}

// resets the whole arena so it is basically cleared and can be used again
void arena_reset(Arena *ar)
{
  if (!ar)
    return;

  pthread_mutex_lock(&ar->lock);
  // resetting the pointer to be at the start of the arena
  ar->used = 0; 
  pthread_mutex_unlock(&ar->lock);
}

void destroyArena(Arena *ar) {
  if (!ar)
    return;
  munmap(ar->start, ar->size);
  pthread_mutex_destroy(&ar->lock);
  free(ar);
}
