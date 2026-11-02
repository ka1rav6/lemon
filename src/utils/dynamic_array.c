#include "dynamic_array.h"


void *reallocate(void *ptr, size_t _old, size_t _new, Arena *ar)
{
  if (ar != NULL) {
    if (_new == 0)
      return NULL;
    void *result = arena_alloc(ar, _new);
    if (result == NULL)
      exit(1);
    if (ptr != NULL)
      memcpy(result, ptr, _old < _new ? _old : _new);
    return result;
  }

  (void)_old; // kept for a future allocator that needs the old size
  if (_new == 0) {
    free(ptr);
    return NULL;
  }
  void *result = realloc(ptr, _new);
  if (result == NULL)
    exit(1);
  return result;
}
