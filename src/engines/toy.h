#ifndef TOY_H
#define TOY_H 1

#include <array>
#include <string>

namespace Toy
{
    class ToyEval
    {
    public:
        static int evaluate_fen(const std::string& fen);
    };
}

#endif // !TOY_H
