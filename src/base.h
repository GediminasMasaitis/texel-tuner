#ifndef BASE_H
#define BASE_H

#include <array>
#include <cstdint>
#include <limits>
#include <vector>

//#define TAPERED 1

using tune_t = double;

#if TAPERED
using pair_t = std::array<tune_t, 2>;
using parameters_t = std::vector<pair_t>;
#else
using parameters_t = std::vector<tune_t>;
#endif

using coefficients_t = std::vector<int16_t>;

#if TAPERED
// Per-parameter, per-phase tuning bounds. After each gradient step the tuner
// projects every parameter back into [lower, upper] (separately for mg/eg), so
// the unconstrained terms tune around any floors/ceilings (projected gradient
// descent) rather than converging to an unrepresentable joint optimum.
inline constexpr tune_t bound_inf = std::numeric_limits<tune_t>::infinity();

struct Bound
{
    pair_t lower;
    pair_t upper;
};
using bounds_t = std::vector<Bound>;

// Push one parameter's bounds. Defaults to the int8 range used by most terms.
inline void add_bound_single(bounds_t& bounds,
    const tune_t mg_lo = -128, const tune_t mg_hi = 127,
    const tune_t eg_lo = -128, const tune_t eg_hi = 127)
{
    bounds.push_back(Bound{ pair_t{ mg_lo, eg_lo }, pair_t{ mg_hi, eg_hi } });
}

// Push the same bounds for every element of an array term (broadcast).
inline void add_bound_array(bounds_t& bounds, const int count,
    const tune_t mg_lo = -128, const tune_t mg_hi = 127,
    const tune_t eg_lo = -128, const tune_t eg_hi = 127)
{
    for (int i = 0; i < count; i++)
    {
        add_bound_single(bounds, mg_lo, mg_hi, eg_lo, eg_hi);
    }
}

// Push the same bounds for every element of a 2d array term (broadcast).
inline void add_bound_array_2d(bounds_t& bounds, const int count1, const int count2,
    const tune_t mg_lo = -128, const tune_t mg_hi = 127,
    const tune_t eg_lo = -128, const tune_t eg_hi = 127)
{
    for (int i = 0; i < count1; i++)
    {
        add_bound_array(bounds, count2, mg_lo, mg_hi, eg_lo, eg_hi);
    }
}
#endif

struct EvalResult
{
    coefficients_t coefficients;
    tune_t score;
    tune_t endgame_scale = 1;

    coefficients_t safety_white;
    coefficients_t safety_black;
    tune_t safety_offset_white = 0;     // midgame no-queen baseline added to S_mg
    tune_t safety_offset_black = 0;
    tune_t safety_offset_white_eg = 0;  // endgame no-queen baseline added to S_eg
    tune_t safety_offset_black_eg = 0;
};

#if TAPERED
enum class PhaseStages
{
    Midgame = 0,
    Endgame = 1
};

constexpr int32_t S(const int32_t mg, const int32_t eg)
{
    return static_cast<int32_t>(static_cast<uint32_t>(eg) << 16) + mg;
}

static constexpr int32_t mg_score(int32_t score)
{
    return static_cast<int16_t>(score);
}

static constexpr int32_t eg_score(int32_t score)
{
    return static_cast<int16_t>((score + 0x8000) >> 16);
}
#else
constexpr int32_t S(const int32_t mg, const int32_t eg)
{
    return (mg + eg)/2;
}
#endif

template<typename T>
void get_initial_parameter_single(parameters_t& parameters, const T& parameter)
{
#if TAPERED
    const auto mg = mg_score(static_cast<int32_t>(parameter));
    const auto eg = eg_score(static_cast<int32_t>(parameter));
    const pair_t pair = { static_cast<tune_t>(mg), static_cast<tune_t>(eg) };
    parameters.push_back(pair);
#else
    parameters.push_back(static_cast<tune_t>(parameter));
#endif
}

template<typename T>
void get_initial_parameter_array(parameters_t& parameters, const T& parameter, const int size)
{
    for (int i = 0; i < size; i++)
    {
        get_initial_parameter_single(parameters, parameter[i]);
    }
}

template<typename T>
void get_initial_parameter_array_2d(parameters_t& parameters, const T& parameter, const int size1, const int size2)
{
    for (int i = 0; i < size1; i++)
    {
        get_initial_parameter_array(parameters, parameter[i], size2);
    }
}


template<typename T>
void get_coefficient_single(coefficients_t& coefficients, const T& trace)
{
    coefficients.push_back(static_cast<int16_t>(trace[0] - trace[1]));
}

template<typename T>
void get_coefficient_array(coefficients_t& coefficients, const T& trace, const int size)
{
    for (int i = 0; i < size; i++)
    {
        get_coefficient_single(coefficients, trace[i]);
    }
}

template<typename T>
void get_coefficient_array_2d(coefficients_t& coefficients, const T& trace, const int size1, const int size2)
{
    for (int i = 0; i < size1; i++)
    {
        get_coefficient_array(coefficients, trace[i], size2);
    }
}

#endif // !BASE_H