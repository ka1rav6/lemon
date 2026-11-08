/*
 * This is the file where everything begins. When you write `lem` and follow it with a command,
 * this file parses the arguments and dispatches to the correct function based on the command
 */


#include "utils/common.h"
#include "utils/arena.h"
#include "utils/dynamic_array.h"
#include "compiler/lexer.h"

#include <stdbool.h>

// The print and such debugging functions work only 
// when in debug mode
#if DEBUG_MODE == 1
ENUM_NAMES(token_type_name, TOKEN_TYPE_LIST)
#endif

void process_arguments(int, const char**);

int main(int argc, const char** argv)
{
    // just writing `lem` should print the whole help
    if (argc <= 1){
        // print_help();
        return 0;
    }
    process_arguments(argc, argv);
    return 0;
}

void process_arguments(int argc, const char** argv)
{

// just a simple macro to clean things up, checks if the first argument 
// (after `lem`) is equal to the string passed
#define ARGV1_EQ(str) \
    (strcmp(argv[1], str) == 0)

#if DEBUG_MODE == 1
    for (int i = 1; i < argc; i++)
        printf("Arg #%d\tstring: %s\n", i, argv[i]);
#endif
    if (ARGV1_EQ("build")){
        // the user wants us to compile something.
         if (argc < 3){
             // later, just `lem build` will be able to compile the whole project. 
             // But for now the focus is on single file compilation.
             fprintf(stderr, "usage: lem build <file>\n");
             return;
         }
         // this all will be actually in the `build` function once it has been created
         Arena* ar       = arena_create((size_t)1 << 30);
         const char* why = NULL;
         Lexer* lx       = lexer_init(ar, argv[2], &why);
         if (lx == NULL){
             fprintf(stderr, "lem: %s: '%s'\n", why, argv[2]);
             arena_destroy(ar);
             return;
         }
         TokenArray* toks = tokenize_file(lx);

#if DEBUG_MODE == 1
         for (size_t i = 0; i < toks->count; i++){
             Token* t = &toks->items[i];
             printf("%4zu  %-14s  %s:%" PRIu64 ":%" PRIu64, i,
                    token_type_name(t->type), t->loc.file,
                    t->loc.line, t->loc.column);
             if (t->type == TOKEN_NUMBER)
                 printf("  number %" PRIu64, t->number);
             putchar('\n');
         }
#endif

         DA_FREE(*toks, NULL);
         arena_destroy(ar);
    }
    else if (ARGV1_EQ("test")){
        LOG("Run the testing suite");
    }
    LOG("argc : %d", argc);

// this macro is no longer needed anymore
#undef ARGV1_EQ
}
