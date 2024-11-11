#include "solver.hpp"
#include "logging.hpp"
#include "solver_api.hpp"
#include <chrono>
#include <numeric>
#include <fstream>

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        LOG_FATAL("usage: oRatio <input-file> [<input-file> ...] <output-file>");
        return -1;
    }

    // the problem files..
    std::vector<std::string> prob_names;
    for (int i = 1; i < argc - 1; i++)
        prob_names.push_back(argv[i]);

    // the solution file..
    std::string sol_name = argv[argc - 1];

    LOG_INFO("starting oRatio");
    auto start = std::chrono::high_resolution_clock::now();
    auto s = std::make_shared<ratio::solver>();
    s->init();
    try
    {
        s->read(prob_names);

        if (s->solve())
        {
            LOG_INFO("hurray!! we have found a solution..");

            std::ofstream sol_file;
            sol_file.open(sol_name);
            sol_file << to_json(*s).dump();
            sol_file.close();
        }
        else
        {
            LOG_INFO("the problem is unsolvable..");
        }
        auto dur = std::chrono::high_resolution_clock::now() - start;
        LOG_INFO("running time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()) + " ms");
    }
    catch (const std::exception &ex)
    {
        LOG_FATAL("exception: " + std::string(ex.what()));
        return 1;
    }

    return 0;
}
