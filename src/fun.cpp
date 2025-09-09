#include "../inc/fun.h"
#include <symengine/symbol.h>
#include <iostream>
#include <unordered_map>

namespace FunDB
{
    double Function::evaluate(const std::unordered_map<std::string, double> &values) const
    {
        // Check if all required symbols are present in values
        for (const auto &symbol : this->symbols)
        {
            if (values.find(symbol) == values.end())
            {
                throw std::runtime_error("Missing value for symbol '" + symbol + "' in evaluation map.");
            }
        }
        SymEngine::map_basic_basic subs;
        for (const auto &pair : values)
        {
            subs[SymEngine::symbol(pair.first)] = SymEngine::real_double(pair.second);
        }
        SymEngine::Expression evaluated_expr = this->expr.subs(subs);
        return static_cast<double>(evaluated_expr);
    }

    std::string Function::serialize() const
    {
        std::ostringstream oss;
        for (size_t i = 0; i < this->symbols.size(); ++i)
        {
            oss << this->symbols[i];
            if (i + 1 < this->symbols.size())
            {
                oss << ",";
            }
        }
        oss << "|" << this->expr;
        return oss.str();
    }
}