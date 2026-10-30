#include "map.h"
#include "dynamic_array.h"
#include <stdbool.h>

// grow once the table is three quarters full
#define TOO_FULL(m) ((m)->count + 1 > (m)->capacity * 3 / 4)

// FNV-1a over raw bytes. a string key hashes its characters,
// a pointer key hashes the bytes of the pointer
static uint64_t hash_bytes(const void *bytes, size_t len)
{
  const unsigned char *p = (const unsigned char *)bytes;
  uint64_t h = 0xcbf29ce484222325u;
  for (size_t i = 0; i < len; i++) {
    h ^= p[i];
    h *= 0x100000001b3u;
  }
  return h;
}

static uint64_t key_hash(const Map *m, const char *key)
{
  if (m->keys == MAP_STR_KEYS)
    return hash_bytes(key, strlen(key));
  return hash_bytes(&key, sizeof key);
}

static int key_eq(const Map *m, const char *a, const char *b)
{
  if (m->keys == MAP_STR_KEYS)
    return strcmp(a, b) == 0;
  return a == b;
}

static MapEntry *find_slot(const Map *m, const char *key)
{
  size_t i = (size_t)key_hash(m, key) & (m->capacity - 1);
  while(true) {
    MapEntry *e = &m->entries[i];
    if (e->key == NULL || key_eq(m, e->key, key))
      return e;
    i = (i + 1) & (m->capacity - 1);
  }
}

static void grow(Map *m)
{
  Map bigger;
  map_init(&bigger, m->keys);
  bigger.capacity = GROW_CAPACITY(m->capacity);
  bigger.entries  = GROW_ARRAY(MapEntry, NULL, 0, bigger.capacity);
  memset(bigger.entries, 0, sizeof(MapEntry) * bigger.capacity);

  for (size_t i = 0; i < m->capacity; i++) {
    MapEntry *e = &m->entries[i];
    if (e->key == NULL)
      continue;
    *find_slot(&bigger, e->key) = *e;
    bigger.count++;
  }

  FREE_ARRAY(MapEntry, m->entries, m->capacity);
  *m = bigger;
}

void map_init(Map *m, MapKeys keys)
{
  m->entries  = NULL;
  m->count    = 0;
  m->capacity = 0;
  m->keys     = keys;
}

void map_free(Map *m)
{
  FREE_ARRAY(MapEntry, m->entries, m->capacity);
  m->entries  = NULL;
  m->count    = 0;
  m->capacity = 0;
}

void *map_get(const Map *m, const char *key)
{
  if (m->count == 0)
    return NULL;
  MapEntry *e = find_slot(m, key);
  return e->key == NULL ? NULL : e->value;
}

void map_put(Map *m, const char *key, void *value)
{
  if (TOO_FULL(m))
    grow(m);

  MapEntry *e = find_slot(m, key);
  if (e->key == NULL)
    m->count++;
  e->key   = key;
  e->value = value;
}

MapEntry *map_next(const Map *m, MapEntry *prev)
{
  if (m->entries == NULL)
    return NULL;
  for (MapEntry *e = prev ? prev + 1 : m->entries;
       e < m->entries + m->capacity; e++)
    if (e->key != NULL)
      return e;
  return NULL;
}

const char *map_intern(Map *m, Arena *ar, const char *s, size_t len)
{
  assert(m->keys == MAP_STR_KEYS);
  if (TOO_FULL(m))
    grow(m);

  size_t i = (size_t)hash_bytes(s, len) & (m->capacity - 1);
  while (true) {
    MapEntry *e = &m->entries[i];
    if (e->key == NULL) {
      char *copy = (char *)arena_alloc(ar, len + 1);
      assert(copy != NULL);
      memcpy(copy, s, len);
      copy[len] = '\0';
      e->key    = copy;
      e->value  = copy;  
      m->count++;
      return copy;
    }
    if (strlen(e->key) == len && memcmp(e->key, s, len) == 0)
      return e->key;
    i = (i + 1) & (m->capacity - 1);
  }
}
