#ifdef BUILD_SERVER
#include "solver_server.hpp"
#include <thread>
#define SOLVER_CLASS ratio::server::server
#else
#if defined(SEMITONE)
#include "semitonesolver.hpp"
#define SOLVER_CLASS ratio::semitonesolver
#elif defined(MathSAT)
#include "msatsolver.hpp"
#define SOLVER_CLASS ratio::msatsolver
#elif defined(Z3)
#include "z3solver.hpp"
#define SOLVER_CLASS ratio::z3solver
#endif
#include <fstream>
#endif
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

    LOG_INFO("starting oRatio server");

    SOLVER_CLASS solver;

#ifdef BUILD_SERVER
    auto srv_ft = std::async(std::launch::async, [&solver]
                             { solver.start(); });
    std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

    solver.read(prob_names);

    if (solver.solve())
    {
        LOG_INFO("hurray!! we have found a solution..");

        std::ofstream sol_file;
        sol_file.open(sol_name);
        sol_file << solver.to_json().dump();
        sol_file.close();
    }
    else
        LOG_INFO("the problem is unsolvable..");

    return 0;
}
