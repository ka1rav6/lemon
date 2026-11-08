#include "../greatest.h"

#include "../../src/compiler/lexer.h"
#include "../../src/utils/arena.h"
#include "../../src/utils/dynamic_array.h"


TEST lexer_init_to_a_file_that_does_not_exist_fails(void)
{
    Arena* a = arena_create(1024);
    const char* why = NULL;

    ASSERT(lexer_init(a, NULL, &why) == NULL);
    ASSERT(why != NULL);

    why = NULL;
    ASSERT(lexer_init(a, "non-existent.lem", &why) == NULL);
    ASSERT(why != NULL);

    ASSERT(lexer_init(a, "non-existent.lem", NULL) == NULL);  // why is optional

    arena_destroy(a);
    PASS();
}

// a source bigger than the arena is a different failure from a missing file
TEST lexer_init_reports_a_full_arena_separately(void)
{
    Arena* small    = arena_create(8);
    const char* why = NULL;
    ASSERT(lexer_init(small, "base/test_file_1.txt", &why) == NULL);
    ASSERT(why != NULL);
    ASSERT(strcmp(why, "cannot open file") != 0);
    arena_destroy(small);
    PASS();
}

TEST lexer_works_on_the_test_file_1(void)
{
    Arena* a = arena_create(1024);
    Lexer* l = lexer_init(a, "base/test_file_1.txt", NULL);
    ASSERT(l != NULL);
    TokenArray* ta = tokenize_file(l);
    ASSERT(ta->count == 10);
    ASSERT(ta->capacity == 16);
    // "(2 + 3 - 10) * 5" : a digit run is one token, and EOF closes the stream
    ASSERT(ta->items[0].type == TOKEN_LPAREN);
    ASSERT(ta->items[1].type == TOKEN_NUMBER);
    ASSERT(ta->items[2].type == TOKEN_PLUS);
    ASSERT(ta->items[3].type == TOKEN_NUMBER);
    ASSERT(ta->items[4].type == TOKEN_MINUS);
    ASSERT(ta->items[5].type == TOKEN_NUMBER);
    ASSERT(ta->items[6].type == TOKEN_RPAREN);
    ASSERT(ta->items[7].type == TOKEN_STAR);
    ASSERT(ta->items[8].type == TOKEN_NUMBER);
    ASSERT(ta->items[9].type == TOKEN_EOF);
    // checking if the numbers have the correct values
    ASSERT(ta->items[1].number == 2);
    ASSERT(ta->items[3].number == 3);
    ASSERT(ta->items[5].number == 10);
    ASSERT(ta->items[8].number == 5);
    // one line, so every token is on line 1 and the column is its own
    ASSERT(ta->items[0].loc.line == 1);
    ASSERT(ta->items[0].loc.column == 1);
    ASSERT(ta->items[2].loc.column == 4);  // the '+' in "(2 + ..."

    DA_FREE(*ta, NULL);
    arena_destroy(a);
    PASS();
}

// the one thing a single-line lexer got wrong: a line break has to bump the
// line and restart the column, and a literal too big for u64 must not wrap
TEST lexer_tracks_lines_and_columns_across_a_file(void)
{
    Arena* a = arena_create(1024);
    Lexer* l = lexer_init(a, "base/test_file_2.txt", NULL);
    ASSERT(l != NULL);
    TokenArray* ta = tokenize_file(l);

    // "1 +" / " 22 *" / "333" / "99999999999999999999999999 + 7"
    ASSERT(ta->count == 9);
    ASSERT(ta->items[0].type == TOKEN_NUMBER && ta->items[0].number == 1);
    ASSERT(ta->items[0].loc.line == 1 && ta->items[0].loc.column == 1);
    ASSERT(ta->items[1].type == TOKEN_PLUS);
    ASSERT(ta->items[1].loc.line == 1 && ta->items[1].loc.column == 3);
    ASSERT(ta->items[2].type == TOKEN_NUMBER && ta->items[2].number == 22);
    ASSERT(ta->items[2].loc.line == 2 && ta->items[2].loc.column == 2);
    ASSERT(ta->items[3].type == TOKEN_STAR);
    ASSERT(ta->items[3].loc.line == 2 && ta->items[3].loc.column == 5);
    ASSERT(ta->items[4].type == TOKEN_NUMBER && ta->items[4].number == 333);
    ASSERT(ta->items[4].loc.line == 3 && ta->items[4].loc.column == 1);
    ASSERT(ta->items[5].type == TOKEN_NUMBER && ta->items[5].number == 0);
    ASSERT(ta->items[5].loc.line == 4 && ta->items[5].loc.column == 1);
    ASSERT(ta->items[6].type == TOKEN_PLUS);
    ASSERT(ta->items[6].loc.line == 4 && ta->items[6].loc.column == 28);
    ASSERT(ta->items[7].type == TOKEN_NUMBER && ta->items[7].number == 7);
    ASSERT(ta->items[7].loc.line == 4 && ta->items[7].loc.column == 30);
    ASSERT(ta->items[8].type == TOKEN_EOF);
    ASSERT(ta->items[8].loc.line == 5);
    // every token knows which file it came from
    ASSERT_STR_EQ("base/test_file_2.txt", ta->items[0].loc.file);
    DA_FREE(*ta, NULL);
    arena_destroy(a);
    PASS();
}

SUITE(lexer_suit)
{
    RUN_TEST(lexer_works_on_the_test_file_1);
    RUN_TEST(lexer_tracks_lines_and_columns_across_a_file);
    RUN_TEST(lexer_init_to_a_file_that_does_not_exist_fails);
    RUN_TEST(lexer_init_reports_a_full_arena_separately);
}
GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(lexer_suit);
    GREATEST_MAIN_END();
}
