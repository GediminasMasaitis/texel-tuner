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

    vector<DataSource> sources;
    for(int i = 1; i < argc; i++)
    {
        DataSource source { string(argv[i]), 1LL << 60 };
        sources.emplace_back(source);
    }

    run(sources);

    return 0;
}