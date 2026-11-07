#include "utils/common.h"
#include "utils/arena.h"
#include "utils/dynamic_array.h"
#include "compiler/lexer.h"

#include <stdbool.h>

#if DEBUG_MODE == 1
ENUM_NAMES(token_type_name, TOKEN_TYPE_LIST)
#endif

void process_arguments(int, const char**);

int main(int argc, const char** argv)
{
    if (argc <= 1){
        // print_help();
        return 0;
    }
    process_arguments(argc, argv);

    return 0;
}

void process_arguments(int argc, const char** argv)
{
#define ARGV1_EQ(str) \
    (strcmp(argv[1], str) == 0)

#if DEBUG_MODE == 1
    for (int i = 1; i < argc; i++)
        printf("Arg #%d\tstring: %s\n", i, argv[i]);
#endif
    if (ARGV1_EQ("build")){
         if (argc < 3){
             fprintf(stderr, "usage: lem build <file>\n");
             return;
         }
         // this all will be actually in the `build` function once it has been created
         Arena* ar = arena_create(1 << 16);
         if (lexer_init(ar, argv[2]) == false){
             fprintf(stderr, "lem: cannot read '%s'\n", argv[2]);
             arena_destroy(ar);
             return;
         }
         TokenArray* toks = tokenize_file();
#if DEBUG_MODE == 1
         for (size_t i = 0; i < toks->count; i++)
             printf("%15zu\ttype %15s\tvalue %15d\tcol %15lu\n", i,
                    token_type_name(toks->items[i].type), toks->items[i].value,
                    (unsigned long)toks->items[i].data.character);
#endif
         DA_FREE(*toks, NULL);
         arena_destroy(ar);
    }
    else if (ARGV1_EQ("test")){
        LOG("Run the testing suite");
    }
    LOG("argc : %d", argc);

#undef ARGV1_EQ
}
