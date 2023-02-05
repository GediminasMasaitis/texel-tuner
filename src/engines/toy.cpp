#include "toy.h"
#include "toy_base.h"
#include "../base.h"

#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>

#if TAPERED
#else

using namespace std;
using namespace Toy;

struct Trace
{
    int32_t material[6][2]{};
    int32_t bishop_pair[2]{};
};

constexpr std::array<int32_t, 6> material = { 100, 300, 300, 500, 900 };
constexpr int32_t bishop_pair = 25;

static Trace trace_evaluate(const Position& position)
{
    Trace trace{};
    std::array<int, 2> bishop_counts{};

    for(int i = 0; i < 64; i++)
    {
        auto piece = position.pieces[i];

        if(piece == Pieces::None)
        {
            continue;
        }

        if(piece < Pieces::BlackPawn)
        {
            const int materialIndex = static_cast<int>(piece) - static_cast<int>(Pieces::WhitePawn);
            trace.material[materialIndex][0]++;

            if(piece == Pieces::WhiteBishop)
            {
                bishop_counts[0]++;
            }
        }
        else
        {
            const int materialIndex = static_cast<int>(piece) - static_cast<int>(Pieces::BlackPawn);
            trace.material[materialIndex][1]++;

            if (piece == Pieces::BlackBishop)
            {
                bishop_counts[1]++;
            }
        }
    }

    for(int color = 0; color < 2; color++)
    {
        trace.bishop_pair[color] += bishop_counts[color] == 2;
    }

    return trace;
}

static coefficients_t get_coefficients(const Trace& trace)
{
    coefficients_t coefficients;
    get_coefficient_array(coefficients, trace.material, 6);
    get_coefficient_single(coefficients, trace.bishop_pair);
    return coefficients;
}

parameters_t ToyEval::get_initial_parameters()
{
    parameters_t parameters;
    get_initial_parameter_array(parameters, material, material.size());
    get_initial_parameter_single(parameters, bishop_pair);
    return parameters;
}

EvalResult ToyEval::get_fen_eval_result(const std::string& fen)
{
    Position position;
    parse_fen(fen, position);
    auto trace = trace_evaluate(position);
    EvalResult result;
    result.coefficients = get_coefficients(trace);
    result.score = 0;
    return result;
}

static void print_single(std::stringstream& ss, const parameters_t& parameters, int& index, const std::string& name)
{
    ss << "constexpr int " << name << " = " << parameters[index] << ";" << endl;
    index++;
}

static void print_array(std::stringstream& ss, const parameters_t& parameters, int& index, const std::string& name, int count)
{
    ss << "constexpr int " << name << "[] = {";
    for (auto i = 0; i < count; i++)
    {
        ss << parameters[index];
        index++;

        if (i != count - 1)
        {
            ss << ", ";
        }
    }
    ss << "};" << endl;
}

void ToyEval::print_parameters(const parameters_t& parameters)
{
    int index = 0;
    stringstream ss;
    print_array(ss, parameters, index, "material", 6);
    print_single(ss, parameters, index, "bishop_pair");
    cout << ss.str() << "\n";
}
#endif
