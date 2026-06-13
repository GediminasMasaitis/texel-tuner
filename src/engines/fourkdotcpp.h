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
        constexpr static int32_t learning_rate_drop_interval = 10000;
        constexpr static tune_t learning_rate_drop_ratio = 1;
        constexpr static bool adam_bias_correction = false;
        constexpr static bool print_data_entries = false;
        constexpr static int32_t data_load_print_interval = 10000;

        // Quantization-aware clamping. Parameters at or after this index are
        // stored as int8 in the engine, so the tuner projects them back into
        // [quantized_min, quantized_max] after every gradient step (projected
        // gradient descent), letting the other terms tune around the clamped,
        // representable value. Parameters [0, start) are the int16 material and
        // are left free.
        constexpr static int32_t quantized_parameter_start = 6;
        constexpr static tune_t quantized_min = -128;
        constexpr static tune_t quantized_max = 127;

        // Non-linear king-safety term (Andrew Grant's method). The safety weights
        // (king_attacks, Pawn..Queen) occupy [safety_parameter_start, +count) in the
        // parameter vector. The tuner recomputes S = weights . per-side-counts (+ a
        // fixed offset) each epoch and applies the finalizer max(S,0)*S / safety_divisor
        // (midgame only), differentiating through it analytically. Index 107 = after
        // material(6) + pst_rank(48) + pst_file(48) + mobilities(5).
        constexpr static int32_t safety_parameter_start = 107;
        constexpr static int32_t safety_parameter_count = 5;
        constexpr static tune_t safety_divisor = 160;

        static parameters_t get_initial_parameters();
        static EvalResult get_fen_eval_result(const std::string& fen);
        static EvalResult get_external_eval_result(const chess::Board& board);
        static void print_parameters(const parameters_t& parameters);
    };
}

#endif // 4KU_H
