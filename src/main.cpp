#include "../inc/db.h"

#include <random>
#include <chrono>
#include <iostream>

int main()
{
    // Set to true to regenerate and store functions
    const bool RERUN = true;

    // Database instance
    FunDB::Database database{};
    // Random number generator
    std::mt19937 rng(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count()));

    if (RERUN)
        database.clear();

    // Generate and store 10,000 random functions
    std::uniform_int_distribution<int> dist(1, 1000);
    std::vector<std::string> keys;
    const int total_funcs = 10'000;
    const int progress_width = 100;
    for (int i = 0; i < total_funcs; ++i)
    {
        std::string fname = "func_" + std::to_string(i);
        if (RERUN)
        {
            int a = dist(rng);
            int b = dist(rng);
            SymEngine::Expression x2(SymEngine::symbol("x"));
            SymEngine::Expression y2(SymEngine::symbol("y"));
            SymEngine::Expression expr = a * pow(x2, 2) + b * pow(y2, 2) + a * b * x2 * y2;
            database.save_function({fname, {"x", "y"}, expr});
        }
        keys.push_back(fname);

        // Progress bar
        if (i % (total_funcs / progress_width) == 0 || i == total_funcs - 1)
        {
            double progress = static_cast<double>(i + 1) / total_funcs;
            int pos = static_cast<int>(progress * progress_width);
            std::cout << "\r[";
            for (int j = 0; j < progress_width; ++j)
            {
                if (j < pos)
                    std::cout << "=";
                else if (j == pos)
                    std::cout << ">";
                else
                    std::cout << " ";
            }
            std::cout << "] " << int(progress * 100.0) << "%";
            std::cout.flush();
        }
    }
    std::cout << std::endl;

    // Look up and evaluate several random functions, measure time
    std::uniform_int_distribution<int> key_dist(0, keys.size() - 1);
    const int num_lookups = 100;
    std::vector<std::string> lookup_keys;
    for (int i = 0; i < num_lookups; ++i)
    {
        lookup_keys.push_back(keys[key_dist(rng)]);
    }
    auto t_start = std::chrono::high_resolution_clock::now();
    for (const auto &random_key : lookup_keys)
    {
        try
        {
            auto eval_result = FunDB::evaluate_stored_function(database, random_key, {{"x", 2.5}, {"y", 0.02}});
            std::cout << "Evaluated loaded function with x=2.5, y=0.02: " << eval_result << " (Key: " << random_key << ")" << std::endl;
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error evaluating function '" << random_key << "': " << e.what() << std::endl;
        }
    }
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
    std::cout << "\nLookup of " << num_lookups << " random functions took " << duration << " ms." << std::endl;
    return 0;
}