#pragma once

#include "fun.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace FunDB
{
    class Database
    {
    private:
        const std::filesystem::path data_file;
        const std::filesystem::path index_file;
        const size_t HASH_TABLE_SIZE{};
        const uint64_t TOMBSTONE{0xFFFFFFFFFFFFFFFF};
        void save_index(const std::unordered_map<std::string, uint64_t> &index) const;
        uint64_t lookup_key(std::string_view key) const;

    public:
        explicit Database(std::string data_filename = "functions.dat", std::string index_filename = "functions.idx", size_t hash_table_size = 1 << 20);
        void clear();
        void save_function(const Function &func) const;
        std::optional<Function> load_function(std::string_view name) const;
    };

    double evaluate_stored_function(const Database &database, std::string_view search_name, const std::unordered_map<std::string, double> &values);
}