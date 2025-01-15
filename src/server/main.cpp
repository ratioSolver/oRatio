#include "solver_server.hpp"
#include "logging.hpp"
#include <thread>

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

    ratio::server::server server;

    auto srv_ft = std::async(std::launch::async, [&server]
                             { server.start(); });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.read(prob_names);

    if (server.solve())
    {
        LOG_INFO("hurray!! we have found a solution..");

        std::ofstream sol_file;
        sol_file.open(sol_name);
        sol_file << server.to_json().dump();
        sol_file.close();
    }
    else
        LOG_INFO("the problem is unsolvable..");

    return 0;
}
