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
    for(int i = 1; i < argc; i += 2)
    {
        DataSource source
        {
            string(argv[i]),
            0
        };

        if(i + 1 < argc)
        {
            int64_t position_limit;
            try
            {
                position_limit = stoll(argv[i + 1]);
            }
            catch (const std::invalid_argument&)
            {
                cout << string(argv[i + 1]) << " is not a valid position limit";
                return -1;
            }

            source.position_limit = position_limit;
        }

        sources.emplace_back(source);
    }

    run(sources);

    return 0;
}