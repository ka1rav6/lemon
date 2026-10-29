#include "dynamic_array.h"
 

void *reallocate(void *ptr, size_t _old, size_t _new) 
{
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



