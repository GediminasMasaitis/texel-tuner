#ifndef TOY_BASE_H
#define TOY_BASE_H 1

#include <array>
#include <string>
#include <stdexcept>

namespace Toy
{
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

    static void parse_fen(const std::string& fen, Position& position)
    {
        int flipped_square = 0;
        for (char ch : fen)
        {
            if (ch == ' ')
            {
                break;
            }

            if (ch == '/')
            {
                continue;
            }

            if (ch >= '1' && ch <= '8')
            {
                flipped_square += ch - 0x30;
            }
            else
            {
                if (flipped_square >= 64)
                {
                    throw std::runtime_error("FEN parsing ran off-board");
                }

                position.pieces[flipped_square ^ 56] = char_to_piece(ch);
                flipped_square++;
            }
        }

        if (flipped_square != 64)
        {
            throw std::runtime_error("FEN parsing didn't complete board");
        }

        position.white_to_move = fen.find('w') != std::string::npos;
    }
}

#endif // !TOY_BASE_H
