#include "../inc/db.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string_view>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <functional>

namespace FunDB
{
    Database::Database(std::string data_filename, std::string index_filename, size_t hash_table_size)
        : data_file(data_filename), index_file(index_filename), HASH_TABLE_SIZE(hash_table_size)
    {
    }

    void Database::clear()
    {
        std::filesystem::remove(this->data_file);
        std::filesystem::remove(this->index_file);
    }

    // --- Save index and data to files for O(1) lookup ---
    void Database::save_index(const std::unordered_map<std::string, uint64_t> &index) const
    {
        // Write data to the data file
        std::ofstream data_stream(this->data_file, std::ios::binary | std::ios::trunc);
        if (!data_stream)
        {
            throw std::runtime_error("Could not open data file for writing.");
        }

        std::vector<uint64_t> hash_table(HASH_TABLE_SIZE, TOMBSTONE);
        std::hash<std::string> hasher;

        for (const auto &pair : index)
        {
            // Get the key, value, and calculate hash
            const std::string &key = pair.first;
            const uint64_t offset_value = pair.second;

            // Write the key-value pair to the data file and get its offset
            uint64_t data_offset = data_stream.tellp();
            uint32_t ksize = key.size();
            data_stream.write(reinterpret_cast<const char *>(&ksize), sizeof(ksize));
            data_stream.write(key.data(), ksize);
            data_stream.write(reinterpret_cast<const char *>(&offset_value), sizeof(offset_value));

            // Calculate hash index and resolve collisions using linear probing
            size_t hash_index = hasher(key) % HASH_TABLE_SIZE;
            while (hash_table[hash_index] != TOMBSTONE)
            {
                hash_index = (hash_index + 1) % HASH_TABLE_SIZE;
            }
            hash_table[hash_index] = data_offset;
        }

        // Write the hash table (offsets) to the index file
        std::ofstream idx_stream(this->index_file, std::ios::binary | std::ios::trunc);
        if (!idx_stream)
        {
            throw std::runtime_error("Could not open index file for writing.");
        }
        idx_stream.write(reinterpret_cast<const char *>(hash_table.data()), HASH_TABLE_SIZE * sizeof(uint64_t));
    }

    // --- Load data using the index for O(1) lookup ---
    uint64_t Database::lookup_key(std::string_view key) const
    {
        std::ifstream idx_stream(this->index_file, std::ios::binary);
        if (!idx_stream)
        {
            return TOMBSTONE; // Index file doesn't exist yet
        }

        // Read the entire hash table into memory
        std::vector<uint64_t> hash_table(HASH_TABLE_SIZE);
        idx_stream.read(reinterpret_cast<char *>(hash_table.data()), HASH_TABLE_SIZE * sizeof(uint64_t));

        std::hash<std::string_view> hasher;
        size_t hash_index = hasher(key) % HASH_TABLE_SIZE;

        std::ifstream data_stream(this->data_file, std::ios::binary);
        if (!data_stream)
        {
            return TOMBSTONE; // Data file doesn't exist yet
        }

        // Probe the hash table until a match is found or we find an empty slot
        size_t start_index = hash_index;
        while (hash_table[hash_index] != TOMBSTONE)
        {
            uint64_t data_offset = hash_table[hash_index];
            data_stream.seekg(data_offset);

            uint32_t ksize = 0;
            data_stream.read(reinterpret_cast<char *>(&ksize), sizeof(ksize));

            std::string stored_key(ksize, '\0');
            data_stream.read(&stored_key[0], ksize);

            if (stored_key == key)
            {
                return data_offset;
            }

            // Collision, move to the next slot
            hash_index = (hash_index + 1) % HASH_TABLE_SIZE;
            if (hash_index == start_index)
            {
                // We've looped through the entire table
                break;
            }
        }
        return TOMBSTONE;
    }

    void Database::save_function(const Function &func) const
    {
        // Append the new function to the data file
        std::ofstream data_stream(this->data_file, std::ios::binary | std::ios::app);
        if (!data_stream)
        {
            throw std::runtime_error("Could not open data file for writing.");
        }

        uint64_t new_data_offset = data_stream.tellp();

        std::string serialized_data = func.serialize();

        uint32_t key_size = func.name.size();
        uint32_t data_size = serialized_data.size();

        data_stream.write(reinterpret_cast<const char *>(&key_size), sizeof(key_size));
        data_stream.write(func.name.data(), key_size);
        data_stream.write(reinterpret_cast<const char *>(&data_size), sizeof(data_size));
        data_stream.write(serialized_data.data(), data_size);

        // Read the index file into memory for modification
        std::vector<uint64_t> hash_table(HASH_TABLE_SIZE, TOMBSTONE);
        if (std::ifstream idx_in(this->index_file, std::ios::binary); idx_in)
        {
            idx_in.read(reinterpret_cast<char *>(hash_table.data()), HASH_TABLE_SIZE * sizeof(uint64_t));
        }

        // Find the correct slot for the new function using linear probing
        std::hash<std::string> hasher;
        size_t hash_index = hasher(func.name) % HASH_TABLE_SIZE;
        size_t start_index = hash_index;

        while (true)
        {
            if (hash_table[hash_index] == TOMBSTONE)
            {
                hash_table[hash_index] = new_data_offset;
                break;
            }

            // Check if the key at the existing slot is a match to handle updates
            std::ifstream data_check(this->data_file, std::ios::binary);
            if (!data_check)
            {
                hash_table[hash_index] = new_data_offset;
                break;
            }

            data_check.seekg(hash_table[hash_index]);
            uint32_t ksize;
            data_check.read(reinterpret_cast<char *>(&ksize), sizeof(ksize));
            std::string stored_key(ksize, '\0');
            data_check.read(&stored_key[0], ksize);

            if (stored_key == func.name)
            {
                hash_table[hash_index] = new_data_offset;
                break;
            }

            // Move to the next slot for linear probing
            hash_index = (hash_index + 1) % HASH_TABLE_SIZE;

            // Prevent infinite loop if the table is full
            if (hash_index == start_index)
            {
                throw std::runtime_error("Hash table is full.");
            }
        }

        // Write the modified hash table back to the index file
        std::ofstream idx_out(this->index_file, std::ios::binary | std::ios::trunc);
        if (!idx_out)
        {
            throw std::runtime_error("Could not open index file for writing.");
        }
        idx_out.write(reinterpret_cast<const char *>(hash_table.data()), HASH_TABLE_SIZE * sizeof(uint64_t));
    }

    // Lookup a function by name
    std::optional<Function> Database::load_function(std::string_view name) const
    {
        uint64_t offset = this->lookup_key(name);
        if (offset == TOMBSTONE)
        {
            return std::nullopt;
        }

        std::ifstream file(this->data_file, std::ios::binary);
        if (!file)
        {
            return std::nullopt;
        }

        file.seekg(offset);
        uint32_t key_size = 0, value_size = 0;
        file.read(reinterpret_cast<char *>(&key_size), sizeof(key_size));
        std::string key(key_size, '\0');
        file.read(&key[0], key_size);
        file.read(reinterpret_cast<char *>(&value_size), sizeof(value_size));
        std::string value(value_size, '\0');
        file.read(&value[0], value_size);

        // Parse value
        size_t sep = value.find('|');
        if (sep != std::string::npos)
        {
            std::string sym_str = value.substr(0, sep);
            std::istringstream iss(sym_str);
            std::string sym;
            std::vector<std::string> symbols;
            while (std::getline(iss, sym, ','))
                symbols.push_back(sym);
            SymEngine::Expression expr(value.substr(sep + 1));
            return Function{key, symbols, expr};
        }
        return std::nullopt;
    }

    double evaluate_stored_function(const Database &database, std::string_view search_name, const std::unordered_map<std::string, double> &values)
    {
        std::optional<Function> func = database.load_function(search_name);
        if (func)
        {
            return (*func).evaluate(values);
        }
        throw std::runtime_error("Function '" + std::string(search_name) + "' not found in database.");
    }
}