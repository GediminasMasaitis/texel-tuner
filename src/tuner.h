#ifndef TUNER_H
#define TUNER_H

#include <cstdint>
#include <string>
#include <vector>

namespace Tuner
{
    struct DataSource
    {
        std::string path;
        int64_t position_limit;
    };

    void run(const std::vector<DataSource>& sources);
}

#endif // !TUNER_H
