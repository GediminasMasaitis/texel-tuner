#ifndef TOY_TAPERED_H
#define TOY_TAPERED_H 1

#include "../base.h"

#include <string>
#include <vector>

#if TAPERED
namespace Toy
{
    class ToyEvalTapered
    {
    public:
        static parameters_t get_initial_parameters();
        static coefficients_t get_fen_coefficients(const std::string& fen);
        static void print_parameters(const parameters_t& parameters);
    };
}
#endif

#endif // !TOY_TAPERED_H
