#ifndef TOY_H
#define TOY_H 1

#include "../base.h"
#include "../external/chess.hpp"

#include <string>
#include <vector>

#if TAPERED
#else

namespace Toy
{
    class ToyEval
    {
    public:
        constexpr static bool includes_additional_score = false;
        static parameters_t get_initial_parameters();
        static EvalResult get_fen_eval_result(const std::string& fen);
        static EvalResult get_external_eval_result(const chess::Board& board);
        static void print_parameters(const parameters_t& parameters);
    };
}

#endif
#endif // !TOY_H
