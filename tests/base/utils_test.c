#include "../greatest.h"

#include "../../src/utils/arena.h"
#include "../../src/utils/dynamic_array.h"


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

GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(arena_suite);
    RUN_SUITE(dynamic_array_suite);
    GREATEST_MAIN_END();
}
