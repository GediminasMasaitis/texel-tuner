#ifndef FOURKU_H
#define FOURKU_H 1

#include "../base.h"
#include <string>
#include <vector>

namespace Fourku
{
    class FourkuEval
    {
    public:
        static parameters_t get_initial_parameters();
        static coefficients_t get_fen_coefficients(const std::string& fen);
        static void print_parameters(const parameters_t& parameters);
    };
}

#endif // 4KU_H
