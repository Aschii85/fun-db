#define CATCH_CONFIG_MAIN
#include "../inc/fun.h"
#include "../inc/db.h"
#include <symengine/expression.h> // Include this for Expression class
#include <catch2/catch_all.hpp>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cmath>

// The old custom test macros and functions have been removed to avoid conflicts.

// Test case 1: Test FunDB::Function::evaluate
TEST_CASE("Function evaluation with Catch2", "[Function]")
{
    FunDB::Function func;
    func.name = "test_func";
    func.symbols = {"x", "y"};
    // Use the Expression constructor to build the expression correctly
    func.expr = SymEngine::Expression("2*x + 3*y");

    std::unordered_map<std::string, double> values = {{"x", 10.0}, {"y", 5.0}};
    double result = func.evaluate(values);
    REQUIRE(result == Catch::Approx(35.0));
}

// Test case 2: Test Database::save_function and Database::load_function
TEST_CASE("Database save and load", "[Database]")
{
    FunDB::Database db{"test_db.dat", "test_db.idx"};
    db.clear();

    FunDB::Function func_to_save;
    func_to_save.name = "test_save_load";
    func_to_save.symbols = {"a", "b"};
    // Use the Expression constructor to build the expression correctly
    func_to_save.expr = SymEngine::Expression("a*b");

    db.save_function(func_to_save);

    std::optional<FunDB::Function> loaded_func = db.load_function("test_save_load");

    REQUIRE(loaded_func.has_value());
    REQUIRE(loaded_func->name == func_to_save.name);
    REQUIRE(loaded_func->symbols == func_to_save.symbols);
}

// Test case 3: Test FunDB::evaluate_stored_function
TEST_CASE("Stored function evaluation", "[Database]")
{
    FunDB::Database db{"test_eval_stored.dat", "test_eval_stored.idx"};
    db.clear();

    FunDB::Function func_to_save;
    func_to_save.name = "eval_test";
    func_to_save.symbols = {"x", "y"};
    // Use the Expression constructor to build the expression correctly
    func_to_save.expr = SymEngine::Expression("x + y");

    db.save_function(func_to_save);

    std::unordered_map<std::string, double> values = {{"x", 100.0}, {"y", 200.0}};
    double result = FunDB::evaluate_stored_function(db, "eval_test", values);
    REQUIRE(result == Catch::Approx(300.0));
}

// Test case 4: Test that a missing function returns std::nullopt
TEST_CASE("Loading a non-existent function", "[Database]")
{
    FunDB::Database db{"non_existent.dat", "non_existent.idx"};
    db.clear();
    std::optional<FunDB::Function> loaded_func = db.load_function("non_existent_func");
    REQUIRE_FALSE(loaded_func.has_value());
}

// Test case 5: Test that evaluating a function with missing values throws an exception
TEST_CASE("Evaluating with missing values", "[Function]")
{
    FunDB::Function func;
    func.name = "test_missing_vals";
    func.symbols = {"a", "b"};
    func.expr = SymEngine::Expression("a + b");

    std::unordered_map<std::string, double> values = {{"a", 1.0}};

    REQUIRE_THROWS_AS(func.evaluate(values), std::runtime_error);
}
