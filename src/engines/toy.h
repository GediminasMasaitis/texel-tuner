#ifndef TOY_H
#define TOY_H 1

#include "../base.h"

#include <array>
#include <string>
#include <vector>

namespace Toy
{
    class ToyEval
    {
    public:
        static parameters_t get_initial_parameters();
        static coefficients_t get_fen_coefficients(const std::string& fen);
        static void print_parameters(const parameters_t& parameters);
    };
}

#endif // !TOY_H
