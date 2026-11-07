#include "../greatest.h"

#include "../../src/compiler/lexer.h"
#include "../../src/utils/arena.h"
#include "../../src/utils/dynamic_array.h"


TEST lexer_init_to_a_file_that_does_not_exist_fails(void)
{
    Arena* a = arena_create(1024);
    ASSERT_FALSE(lexer_init(a, NULL));
    ASSERT_FALSE(lexer_init(a, "non-existent.lem"));
    arena_destroy(a);
    PASS();
}

TEST lexer_works_on_the_test_file_1(void)
{
    Arena* a = arena_create(1024);
    bool err = lexer_init(a, "base/test_file_1.txt");
    ASSERT(err == true);
    TokenArray* ta = tokenize_file();
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
    ASSERT(ta->items[1].value == 2);
    ASSERT(ta->items[3].value == 3);
    ASSERT(ta->items[5].value == 10);
    ASSERT(ta->items[8].value == 5);

    DA_FREE(*ta, NULL); 
    arena_destroy(a);
    PASS();
}
SUITE(lexer_suit)
{
    RUN_TEST(lexer_works_on_the_test_file_1);
    RUN_TEST(lexer_init_to_a_file_that_does_not_exist_fails);
}
GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(lexer_suit);
    GREATEST_MAIN_END();
}
