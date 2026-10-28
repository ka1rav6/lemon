#include "utils/common.h"
#include "utils/strings.h"

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
    LOG("argc : %d", argc);
}
