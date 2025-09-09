#pragma once

#include <symengine/expression.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace FunDB
{
    struct Function
    {
        std::string name;
        std::vector<std::string> symbols;
        SymEngine::Expression expr;

        double evaluate(const std::unordered_map<std::string, double> &values) const;
        std::string serialize() const;
    };
}