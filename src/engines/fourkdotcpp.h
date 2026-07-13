#ifndef FOURKU_H
#define FOURKU_H 1

#define TAPERED 1

#include "../base.h"
#include "../external/chess.hpp"

#include <string>
#include <vector>

namespace Fourkdotcpp
{
    class FourkdotcppEval
    {
    public:
        constexpr static bool includes_additional_score = true;
        constexpr static bool supports_external_chess_eval = true;
        constexpr static bool retune_from_zero = true;
        constexpr static tune_t preferred_k = 2.7;
        constexpr static int32_t max_epoch = 5001;
        constexpr static bool enable_qsearch = false;
        constexpr static bool filter_in_check = false;
        constexpr static tune_t initial_learning_rate = 1;
        constexpr static int32_t learning_rate_drop_interval = 1000;
        constexpr static tune_t learning_rate_drop_ratio = 0.5;
        constexpr static bool adam_bias_correction = true;

        // Convex objective for the linear model (see USE_CROSS_ENTROPY in
        // tuner.cpp). Remove or set false to fall back to squared error.
        constexpr static bool use_cross_entropy = true;

        // Decoupled AdamW weight decay per epoch, scaled by the current
        // learning rate. Breaks collinearity ridges (e.g. material vs
        // pawn-count interaction terms) by preferring the minimum-norm
        // solution. Remove or set to 0 to disable.
        constexpr static tune_t l2_lambda = 1e-4;

        // On a clamp (an active parameter bound), optionally discard the Adam
        // moment state for that coordinate so it does not "wind up" against the
        // boundary. Both false = plain projected gradient descent.
        constexpr static bool reset_momentum_on_clamp = false;
        constexpr static bool reset_velocity_on_clamp = false;

        constexpr static bool print_data_entries = false;
        constexpr static int32_t data_load_print_interval = 10000;

        static parameters_t get_initial_parameters();

        // Per-parameter, per-phase tuning bounds (projected gradient descent).
        // Defaults every term to the int8 range [-128, 127]; material is left
        // free (int16 in the engine) and constrained terms (e.g. passed pawns
        // floored at 0) override their range. See get_parameter_bounds().
        static bounds_t get_parameter_bounds();

        static EvalResult get_fen_eval_result(const std::string& fen);
        static EvalResult get_external_eval_result(const chess::Board& board);
        static void print_parameters(const parameters_t& parameters);
    };
}

#endif // 4KU_H
