#include "basic_solver.hpp"
#include "logging.hpp"
#include <chrono>
#include <numeric>
#include <algorithm>
#include <cassert>

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
    std::vector<bool> results;
    std::vector<std::chrono::nanoseconds> times;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        LOG_INFO("running test " + std::to_string(i + 1) + " of " + std::to_string(NUM_TESTS));
        auto start = std::chrono::high_resolution_clock::now();
        ratio::basic_solver slv;
        try
        {
            slv.read(prob_names);
            slv.solve();
            LOG_INFO("hurray!! we have found a solution..");
            results.push_back(true);
        }
        catch (const std::exception &ex)
        {
            LOG_FATAL("the problem is unsolvable: " + std::string(ex.what()));
            results.push_back(false);
        }
        auto dur = std::chrono::high_resolution_clock::now() - start;
        LOG_INFO("running time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()) + " ms");
        times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(dur));
    }
    LOG_INFO("average running time: " + std::to_string(std::accumulate(times.begin(), times.end(), std::chrono::nanoseconds(0)).count() / NUM_TESTS / 1000000) + " ms");
    assert(std::all_of(results.begin(), results.end(), [](bool b)
                       { return b; }) ||
           std::none_of(results.begin(), results.end(), [](bool b)
                        { return b; }));

    return std::all_of(results.begin(), results.end(), [](bool b)
                       { return b; })
               ? 0
               : -1;
}
