#include <chrono>
#include <numeric>
#include "solver.hpp"
#include "logging.hpp"

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
    std::vector<std::chrono::nanoseconds> times;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        LOG_INFO("running test " + std::to_string(i + 1) + " of " + std::to_string(NUM_TESTS));
        auto start = std::chrono::high_resolution_clock::now();
        auto s = std::make_shared<ratio::solver>();
        s->init();
        try
        {
            s->read(prob_names);

            if (s->solve())
            {
                LOG_INFO("hurray!! we have found a solution..");
            }
            else
            {
                LOG_INFO("the problem is unsolvable..");
            }
            auto dur = std::chrono::high_resolution_clock::now() - start;
            LOG_INFO("running time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()) + " ms");
            times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(dur));
        }
        catch (const std::exception &ex)
        {
            LOG_FATAL("exception: " + std::string(ex.what()));
            return 1;
        }
    }

    LOG_INFO("average running time: " + std::to_string(std::accumulate(times.begin(), times.end(), std::chrono::nanoseconds(0)).count() / NUM_TESTS / 1000000) + " ms");

    return 0;
}
