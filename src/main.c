#include "utils/common.h"

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
         LOG("Handing it over to the compiler");
    }
    else if (ARGV1_EQ("test")){
        LOG("Run the testing suite");
    }
    LOG("argc : %d", argc);

#undef ARGV1_EQ
}
