#include "tuner.h"
#include "engines/toy.h"

#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace std::chrono;
using namespace Tuner;

struct WdlMarker
{
    string marker;
    tune_t wdl;
};

static void print_elapsed(high_resolution_clock::time_point start)
{
    const auto now = high_resolution_clock::now();
    const auto elapsed = now - start;
    const auto elapsed_seconds = std::chrono::duration_cast<seconds>(elapsed).count();
    cout << "[" << elapsed_seconds << "s] ";
}

static void load_fens(const DataSource& source, const high_resolution_clock::time_point start)
{
    std::cout << "Loading " << source.path << std::endl;

    const array<WdlMarker, 6> markers
    {
        WdlMarker{"1.0", 1},
        WdlMarker{"0.5", 0.5},
        WdlMarker{"0.0", 0},

        WdlMarker{"1-0", 1},
        WdlMarker{"1/2-1/2", 0.5},
        WdlMarker{"0-1", 0}
    };

    std::ifstream file(source.path);
    int64_t positions = 0;
    std::string line;
    while (!file.eof())
    {
        if (positions >= source.position_limit)
        {
            break;
        }

        std::getline(file, line);
        if (line.empty())
        {
            break;
        }

        bool marker_found = false;
        tune_t result;
        for (auto& marker : markers)
        {
            if (line.find(marker.marker) != std::string::npos)
            {
                if (marker_found)
                {
                    cout << "WDL marker already found on line " << line << endl;
                    throw std::runtime_error("WDL marker already found");
                }
                marker_found = true;
                result = marker.wdl;
            }
        }

        if (!marker_found)
        {
            cout << "WDL marker not found on line " << line << endl;
            throw std::runtime_error("WDL marker not found");
        }

        auto coefficients = Toy::ToyEval::get_fen_coefficients(line);

        positions++;
        if (positions % 100000 == 0)
        {
            print_elapsed(start);
            std::cout << "Loaded " << positions << " entries..." << std::endl;
        }
    }
    std::cout << "Loaded " << positions << " entries from " << source.path << std::endl;
}

void Tuner::run(const std::vector<DataSource>& sources)
{
    cout << "Starting texel tuning" << std::endl;
    const auto start = high_resolution_clock::now();

    for (const auto& source : sources)
    {
        load_fens(source, start);
    }
}
