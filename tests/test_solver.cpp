#include "solver.h"
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "usage: oRatio <input-file> [<input-file> ...] <output-file>\n";
        return -1;
    }

    // the problem files..
    std::vector<std::string> prob_names;
    for (int i = 1; i < argc - 1; i++)
        prob_names.push_back(argv[i]);

    // the solution file..
    std::string sol_name = argv[argc - 1];

    std::cout << "starting oRatio";
#ifndef NDEBUG
    std::cout << " in debug mode";
#endif
    std::cout << "..\n";

    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        ratio::solver::solver s;
    }
}