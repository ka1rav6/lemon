#include "../greatest.h"

#include "../../src/utils/arena.h"
#include "../../src/utils/dynamic_array.h"
#include "../../src/utils/map.h"


TEST arena_refuses_an_allocation_past_the_end(void)
{
    Arena *ar = arena_create(100);
    ASSERT(ar != NULL);
    ASSERT(arena_alloc(ar, 97) != NULL);
    ASSERTm("served an allocation past the end of the mapping",
            arena_alloc(ar, 1) == NULL);
    arena_destroy(ar);
    PASS();
}

TEST arena_aligns_consecutive_allocations(void)
{
    Arena *ar = arena_create(64);
    ASSERT(ar != NULL);

    char *a = arena_alloc(ar, 1);
    char *b = arena_alloc(ar, 1);
    ASSERT(a != NULL && b != NULL);
    ASSERT_EQ_FMT((size_t)ARENA_ALIGNMENT, (size_t)(b - a), "%zu");

    arena_destroy(ar);
    PASS();
}

TEST arena_reset_rewinds_to_the_start(void)
{
    Arena *ar = arena_create(64);
    ASSERT(ar != NULL);
    ASSERT(arena_alloc(ar, 32) != NULL);

    arena_reset(ar);
    ASSERT_EQ_FMT((size_t)0, ar->used, "%zu");

    arena_destroy(ar);
    PASS();
}

TEST arena_strdup_copies_the_string(void)
{
    Arena *ar = arena_create(64);
    ASSERT(ar != NULL);
    ASSERT_STR_EQ("lemon", arena_strdup(ar, "lemon"));
    arena_destroy(ar);
    PASS();
}

TEST arena_create_rejects_a_zero_size(void)
{
    ASSERT(arena_create(0) == NULL);
    PASS();
}

SUITE(arena_suite)
{
    RUN_TEST(arena_refuses_an_allocation_past_the_end);
    RUN_TEST(arena_aligns_consecutive_allocations);
    RUN_TEST(arena_reset_rewinds_to_the_start);
    RUN_TEST(arena_strdup_copies_the_string);
    RUN_TEST(arena_create_rejects_a_zero_size);
}


typedef struct {
    size_t count   ;
    size_t capacity;
    int*   items   ;
} Ints;

TEST da_first_push_takes_the_floor_capacity(void)
{
    Ints xs = {0};
    DA_PUSH(xs, 42);
    ASSERT_EQ_FMT((size_t)8, xs.capacity, "%zu");
    ASSERT_EQ(42, xs.items[0]);
    DA_FREE(xs);
    PASS();
}

TEST da_push_grows_past_the_initial_capacity(void)
{
    Ints xs = {0};
    for (int i = 0; i < 100; i++)
        DA_PUSH(xs, i);

    ASSERT_EQ_FMT((size_t)100, xs.count, "%zu");
    ASSERT(xs.capacity >= xs.count);
    ASSERT_EQ(0, xs.items[0]);
    ASSERT_EQ(99, xs.items[99]);

    DA_FREE(xs);
    PASS();
}

TEST da_free_clears_the_array(void)
{
    Ints xs = {0};
    DA_PUSH(xs, 1);
    DA_FREE(xs);

    ASSERT(xs.items == NULL);
    ASSERT_EQ_FMT((size_t)0, xs.count, "%zu");
    ASSERT_EQ_FMT((size_t)0, xs.capacity, "%zu");
    PASS();
}

SUITE(dynamic_array_suite)
{
    RUN_TEST(da_first_push_takes_the_floor_capacity);
    RUN_TEST(da_push_grows_past_the_initial_capacity);
    RUN_TEST(da_free_clears_the_array);
}

TEST map_get_finds_what_put_stored(void)
{
    char value[] = "fruit";
    Map  m;
    map_init(&m, MAP_PTR_KEYS);

    map_put(&m, "lemon", value);
    ASSERT_STR_EQ("fruit", map_get(&m, "lemon"));
    ASSERT_EQ_FMT((size_t)1, m.count, "%zu");

    map_free(&m);
    PASS();
}

TEST map_get_misses_an_absent_key(void)
{
    Map m;
    map_init(&m, MAP_PTR_KEYS);

    ASSERTm("looked up a key in an empty map", map_get(&m, "lemon") == NULL);
    map_put(&m, "lemon", "fruit");
    ASSERT(map_get(&m, "lime") == NULL);

    map_free(&m);
    PASS();
}

TEST map_put_replaces_the_value_of_a_key(void)
{
    char first[]  = "fruit";
    char second[] = "sour";
    Map  m;
    map_init(&m, MAP_PTR_KEYS);

    map_put(&m, "lemon", first);
    map_put(&m, "lemon", second);

    ASSERT_STR_EQ("sour", map_get(&m, "lemon"));
    ASSERTm("a replaced value added an entry", m.count == 1);

    map_free(&m);
    PASS();
}

TEST map_string_keys_compare_by_content(void)
{
    char here[]  = "lemon";
    char there[] = "lemon";  // same word, different address
    char value[] = "fruit";
    Map  m;
    map_init(&m, MAP_STR_KEYS);

    map_put(&m, here, value);
    ASSERT_STR_EQ("fruit", map_get(&m, there));
    ASSERT_EQ_FMT((size_t)1, m.count, "%zu");

    map_free(&m);
    PASS();
}

TEST map_keeps_every_key_as_the_table_grows(void)
{
    static char keys[100];
    static int  values[100];
    Map m;
    map_init(&m, MAP_PTR_KEYS);

    for (int i = 0; i < 100; i++) {
        values[i] = i;
        map_put(&m, &keys[i], &values[i]);
    }
    ASSERT_EQ_FMT((size_t)100, m.count, "%zu");
    ASSERT(m.capacity >= m.count);

    for (int i = 0; i < 100; i++)
        ASSERT(map_get(&m, &keys[i]) == &values[i]);

    map_free(&m);
    PASS();
}

TEST map_next_walks_every_live_entry(void)
{
    static char keys[3];
    Map    m;
    size_t seen = 0;
    map_init(&m, MAP_PTR_KEYS);

    ASSERTm("walked an empty map", map_next(&m, NULL) == NULL);
    for (int i = 0; i < 3; i++)
        map_put(&m, &keys[i], &keys[i]);

    for (MapEntry *e = map_next(&m, NULL); e != NULL; e = map_next(&m, e)) {
        ASSERT(e->key == e->value);
        seen++;
    }
    ASSERT_EQ_FMT((size_t)3, seen, "%zu");

    map_free(&m);
    PASS();
}

TEST intern_returns_one_pointer_per_word(void)
{
    Arena *ar = arena_create(4096);
    Map    m;
    map_init(&m, MAP_STR_KEYS);
    ASSERT(ar != NULL);

    const char *count   = map_intern(&m, ar, "count", 5);
    const char *again   = map_intern(&m, ar, "count", 5);
    const char *counter = map_intern(&m, ar, "counter", 7);

    ASSERTm("the same word interned to two pointers", count == again);
    ASSERTm("a longer word sharing a prefix interned to the same pointer",
            count != counter);
    ASSERT_STR_EQ("count", count);
    ASSERT_EQ_FMT((size_t)2, m.count, "%zu");

    map_free(&m);
    arena_destroy(ar);
    PASS();
}

TEST intern_copies_only_the_slice_it_is_given(void)
{
    const char *source = "int main(void)";
    Arena      *ar     = arena_create(4096);
    Map         m;
    map_init(&m, MAP_STR_KEYS);
    ASSERT(ar != NULL);

    // "main", with no NUL after it, as the lexer will hand it over
    const char *word = map_intern(&m, ar, source + 4, 4);
    ASSERT_STR_EQ("main", word);
    ASSERT(word == map_intern(&m, ar, "main", 4));

    map_free(&m);
    arena_destroy(ar);
    PASS();
}

TEST intern_keeps_words_unique_as_the_table_grows(void)
{
    const char *first[50];
    char        word[16];
    Arena      *ar = arena_create(1 << 16);
    Map         m;
    map_init(&m, MAP_STR_KEYS);
    ASSERT(ar != NULL);

    for (int i = 0; i < 50; i++) {
        int len = snprintf(word, sizeof word, "name_%d", i);
        first[i] = map_intern(&m, ar, word, (size_t)len);
    }
    ASSERT_EQ_FMT((size_t)50, m.count, "%zu");

    for (int i = 0; i < 50; i++) {
        int len = snprintf(word, sizeof word, "name_%d", i);
        ASSERT(first[i] == map_intern(&m, ar, word, (size_t)len));
    }

    map_free(&m);
    arena_destroy(ar);
    PASS();
}

SUITE(map_suite)
{
    RUN_TEST(map_get_finds_what_put_stored);
    RUN_TEST(map_get_misses_an_absent_key);
    RUN_TEST(map_put_replaces_the_value_of_a_key);
    RUN_TEST(map_string_keys_compare_by_content);
    RUN_TEST(map_keeps_every_key_as_the_table_grows);
    RUN_TEST(map_next_walks_every_live_entry);
    RUN_TEST(intern_returns_one_pointer_per_word);
    RUN_TEST(intern_copies_only_the_slice_it_is_given);
    RUN_TEST(intern_keeps_words_unique_as_the_table_grows);
}


GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(arena_suite);
    RUN_SUITE(dynamic_array_suite);
    RUN_SUITE(map_suite);
    GREATEST_MAIN_END();
}
