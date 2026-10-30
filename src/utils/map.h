#ifndef LEMON_MAP_H
#define LEMON_MAP_H
#include "./arena.h"
#include "./common.h"

typedef enum _MapKeys{
    MAP_PTR_KEYS,  // compare keys by address, as interned names are
    MAP_STR_KEYS   // compare keys by their characters
} MapKeys;

typedef struct _MapEntry{
    const char* key  ;
    void*       value;
}MapEntry;

typedef struct {
    MapEntry* entries ;
    size_t    count   ;
    size_t    capacity;
    MapKeys   keys    ;
}Map;

void  map_init(Map* m, MapKeys keys);
void  map_free(Map* m);
void* map_get(const Map* m, const char* key);

// Note: map does not copy the key, so the key has to outlive the map
void  map_put(Map* m, const char* key, void* value);

// pass NULL to start, then the entry it last gave you, until it returns NULL
MapEntry* map_next(const Map* m, MapEntry* prev);

// m has to be a MAP_STR_KEYS map
const char* map_intern(Map* m, Arena* ar, const char* s, size_t len);

#endif // LEMON_MAP_H
