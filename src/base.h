#ifndef BASE_H
#define BASE_H

#include <vector>

using tune_t = double;
using parameters_t = std::vector<tune_t>;
using coefficients_t = std::vector<int16_t>;


template<typename T>
void get_initial_parameter_single(parameters_t& parameters, const T& parameter)
{
    parameters.push_back(static_cast<tune_t>(parameter));
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

#endif // !BASE_H