#include "toy.h"

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
};

Pieces char_to_piece(char ch)
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

void parse_fen(const string& fen, Position& position)
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
}

constexpr std::array<int, 6> material{ 100, 300, 300, 500, 900 };

int evaluate(const Position& position)
{
    int score = 0;
    for(int i = 0; i < 64; i++)
    {
        auto piece = position.pieces[i];

        if(piece == Pieces::None)
        {
            continue;
        }

        if(piece >= Pieces::BlackPawn)
        {
            score -= material[static_cast<int>(piece) - static_cast<int>(Pieces::BlackPawn)];
        }
        else
        {
            score += material[static_cast<int>(piece) - static_cast<int>(Pieces::WhitePawn)];
        }
    }
    return score;
}

int ToyEval::evaluate_fen(const std::string& fen)
{
    Position position;
    parse_fen(fen, position);
    return evaluate(position);
}
