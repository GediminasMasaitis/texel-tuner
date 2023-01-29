#include "tuner.h"
#include "engines/toy.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace Tuner;

int main(int argc, char** argv) {
    if(argc == 1)
    {
        cout << "Please provide at least one data file " << endl;
        return -1;
    }

    auto parameters = Toy::ToyEval::get_initial_parameters();
    auto coefficients = Toy::ToyEval::get_fen_coefficients("rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQK1NR w KQkq - 0 1");
    int score = 0;
    for(int i = 0; i < parameters.size(); i++)
    {
        score += parameters[i] * coefficients[i];
    }
    std::cout << score;

    vector<DataSource> sources;
    for(int i = 1; i < argc; i++)
    {
        DataSource source { string(argv[i]), 1LL << 60 };
        sources.emplace_back(source);
    }

    run(sources);

    return 0;
}