#ifdef ORATIO_BUILD_SERVER
#ifdef SOLVER_BASIC
#include "basic_server.hpp"
#endif
#include <thread>
#else
#ifdef SOLVER_BASIC
#include "basic_solver.hpp"
#endif
#include <fstream>
#endif
#include "logging.hpp"
#include "solver.hpp"

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
    {
        LOG_DEBUG("adding problem file: " + std::string(argv[i]));
        prob_names.push_back(argv[i]);
    }

    // the solution file..
    std::string sol_name = argv[argc - 1];
    LOG_DEBUG("setting solution file: " + sol_name);

#ifdef ORATIO_BUILD_SERVER
    ratio::basic_server solver;
    LOG_INFO("starting oRatio server");
    auto srv_ft = std::async(std::launch::async, [&solver]
                             { solver.start(); });
    std::this_thread::sleep_for(std::chrono::seconds(1));
#else
    ratio::basic_solver solver;
#endif

    solver.read(prob_names);

    try
    {
        solver.solve();
        LOG_INFO("hurray!! we have found a solution..");

        std::ofstream sol_file;
        sol_file.open(sol_name);
        sol_file << solver.to_json().dump();
        sol_file.close();
    }
    catch (const std::exception &ex)
    {
        LOG_FATAL("the problem is unsolvable: " + std::string(ex.what()));
        return -1;
    }

    return 0;
}