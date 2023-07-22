#ifndef BASE_H
#define BASE_H

#include <array>
#include <vector>

#define TAPERED 1

using tune_t = double;

#if TAPERED
using pair_t = std::array<tune_t, 2>;
using parameters_t = std::vector<pair_t>;
#else
using parameters_t = std::vector<tune_t>;
#endif

using coefficients_t = std::vector<int16_t>;

struct EvalResult
{
    coefficients_t coefficients;
    tune_t score;
    tune_t endgame_scale = 1;
};

#if TAPERED
enum class PhaseStages
{
    Midgame = 0,
    Endgame = 1
};

constexpr int32_t S(const int32_t mg, const int32_t eg)
{
    //return (eg << 16) + mg;
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
    const pair_t pair = { mg, eg };
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