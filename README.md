# FunDB

> This code is a work in progress 

Welcome to FunDB - a small database for storing functions.

Instead of storing discrete function values as is the case for most databases, FunDB stores functions in a compact, on-disk format, powered by the SymEngine symbolic manipulation library. This allows you to store and retrieve symbolic functions, which can then be evaluated at runtime with different variable values.

## Project Structure
- `inc/`: Contains all public header files, including fun.h (for the Function class) and db.h (for the Database class).
- `src/`: Contains the C++ source code files. main.cpp demonstrates the usage of the database, while fun.cpp and db.cpp contain the implementations for the Function and Database classes, respectively.
- `CMakeLists.txt`: The CMake build configuration file.


## How It Works

FunDB stores data in two separate files: a data file (.dat) and an index file (.idx).

- Data File: This is a simple append-only file that stores the serialized function data, including the function's name, symbols, and the expression itself as a string compatible by SymEngine.
- Index File: This file acts as a persistent hash table for fast lookups. The index uses a hash of the function's name to find the byte offset of the corresponding function in the data file. To handle hash collisions, it employs linear probing.

This design allows for fast key-value lookups while keeping the data storage simple and efficient for symbolic functions.

## Usage

Here is a simple example of how to use the FunDB::Database class in your own `c++` code to save and load a function.

```c++
#include <fundb/fun.h>
#include <fundb/db.h>
#include <iostream>
#include <unordered_map>
#include <symengine/expression.h>
#include <symengine/symbol.h>

int main() {
    FunDB::Database db{};
    db.clear(); // Start with a clean database

    // Create a new function
    SymEngine::Expression expr = SymEngine::symbol("x") * 2 + SymEngine::symbol("y") * 3;
    FunDB::Function my_func{"linear_fn", {"x", "y"}, expr};

    // Save the function to the database
    db.save_function(my_func);

    // Look up and evaluate the stored function
    std::unordered_map<std::string, double> values = {{"x", 10.0}, {"y", 5.0}};
    float result = FunDB::evaluate_stored_function(db, "linear_fn", values);

    std::cout << "Evaluation result: " << result << std::endl; // Expected output: 35
    
    return 0;
}
```

## Build and Run main.cpp

To build the project and run the main.cpp example, you will use Conan for dependency management and CMake for the build system.

First, ensure you have Conan and CMake installed. Then, follow these steps:

```bash
conan install . --output-folder=build --build=missing
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
./my_app
```