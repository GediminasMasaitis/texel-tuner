#include "toy.h"
#include "../base.h"

#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;
using namespace Toy;

enum class Pieces
{
    None,
    WhitePawn,
    WhiteKnight,
    WhiteBishop,
    WhiteRook,
    WhiteQueen,
    WhiteKing,
    BlackPawn,
    BlackKnight,
    BlackBishop,
    BlackRook,
    BlackQueen,
    BlackKing
};

struct Position
{
    std::array<Pieces, 64> pieces{ Pieces::None };
    bool white_to_move = true;
};

static Pieces char_to_piece(const char ch)
{
    switch (ch)
    {
    case 'P': return Pieces::WhitePawn;
    case 'N': return Pieces::WhiteKnight;
    case 'B': return Pieces::WhiteBishop;
    case 'R': return Pieces::WhiteRook;
    case 'Q': return Pieces::WhiteQueen;
    case 'K': return Pieces::WhiteKing;
    case 'p': return Pieces::BlackPawn;
    case 'n': return Pieces::BlackKnight;
    case 'b': return Pieces::BlackBishop;
    case 'r': return Pieces::BlackRook;
    case 'q': return Pieces::BlackQueen;
    case 'k': return Pieces::BlackKing;
    default: throw std::runtime_error("Invalid FEN");
    }
}

static void parse_fen(const string& fen, Position& position)
{
    int flipped_square = 0;
    for(char ch : fen)
    {
        if(ch == ' ')
        {
            break;
        }

        if(ch == '/')
        {
            continue;
        }

        if(ch >= '1' && ch <= '8')
        {
            flipped_square += ch - 0x30;
        }
        else
        {
            if(flipped_square >= 64)
            {
                throw std::runtime_error("FEN parsing ran off-board");
            }

            position.pieces[flipped_square ^ 56] = char_to_piece(ch);
            flipped_square++;
        }
    }

    if(flipped_square != 64)
    {
        throw std::runtime_error("FEN parsing didn't complete board");
    }

    position.white_to_move = fen.find('w') != std::string::npos;
}

struct Trace
{
    int32_t material[6][2]{};
    int32_t bishop_pair[2]{};
};

constexpr std::array<int32_t, 6> material = { 100, 300, 300, 500, 900 };
constexpr int32_t bishop_pair = 25;

static Trace trace_evaluate(const Position& position)
{
    Trace trace;
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

coefficients_t get_coefficients(const Trace& trace)
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

coefficients_t ToyEval::get_fen_coefficients(const std::string& fen)
{
    Position position;
    parse_fen(fen, position);
    auto trace = trace_evaluate(position);
    auto coefficients = get_coefficients(trace);
    return coefficients;
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
        ss << parameters[i];
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
