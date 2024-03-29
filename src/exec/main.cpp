#include "solver.h"
#include <iostream>
#include <fstream>

int main(int argc, char const *argv[])
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

#ifdef NDEBUG
    if (std::ifstream(sol_name).good())
    {
        std::cout << "The solution file '" << sol_name << "' already exists! Please, specify a different solution file..";
        return -1;
    }
#endif

    std::cout << "starting oRatio";
#ifndef NDEBUG
    std::cout << " in debug mode";
#endif
    std::cout << "..\n";

    ratio::solver s;
    try
    {
        std::cout << "parsing input files..\n";
        s.read(prob_names);

        std::cout << "solving the problem..\n";
        if (s.solve())
            std::cout << "hurray!! we have found a solution..\n";
        else
        {
            std::cout << "the problem is unsolvable..\n";
            return 1;
        }

        std::ofstream sol_file;
        sol_file.open(sol_name);
        sol_file << to_json(s).to_string();
        sol_file.close();
    }
    catch (const std::exception &ex)
    {
        std::cout << ex.what() << '\n';
        return 1;
    }

    return 0;
}
