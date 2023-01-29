#include "tuner.h"
#include "base.h"
#include "engines/toy.h"

#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;
using namespace std::chrono;
using namespace Tuner;

struct WdlMarker
{
    string marker;
    tune_t wdl;
};

struct CoefficientEntry
{
    int16_t value;
    int16_t index;
};

struct Entry
{
    vector<CoefficientEntry> coefficients;
    tune_t wdl;
};

static const array<WdlMarker, 6> markers
{
    WdlMarker{"1.0", 1},
    WdlMarker{"0.5", 0.5},
    WdlMarker{"0.0", 0},

    WdlMarker{"1-0", 1},
    WdlMarker{"1/2-1/2", 0.5},
    WdlMarker{"0-1", 0}
};

tune_t get_fen_wdl(const string& fen)
{
    tune_t result;
    bool marker_found = false;
    for (auto& marker : markers)
    {
        if (fen.find(marker.marker) != std::string::npos)
        {
            if (marker_found)
            {
                cout << "WDL marker already found on line " << fen << endl;
                throw std::runtime_error("WDL marker already found");
            }
            marker_found = true;
            result = marker.wdl;
        }
    }

    if (!marker_found)
    {
        cout << "WDL marker not found on line " << fen << endl;
        throw std::runtime_error("WDL marker not found");
    }

    return result;
}

static void print_elapsed(high_resolution_clock::time_point start)
{
    const auto now = high_resolution_clock::now();
    const auto elapsed = now - start;
    const auto elapsed_seconds = std::chrono::duration_cast<seconds>(elapsed).count();
    cout << "[" << elapsed_seconds << "s] ";
}

static void get_coefficient_entries(const string& fen, vector<CoefficientEntry>& coefficient_entries)
{
    const auto coefficients = Toy::ToyEval::get_fen_coefficients(fen);
    for (int16_t i = 0; i < coefficients.size(); i++)
    {
        if (coefficients[i] == 0)
        {
            continue;
        }

        const auto coefficient_entry = CoefficientEntry{coefficients[i], i};
        coefficient_entries.push_back(coefficient_entry);
    }
}

static void load_fens(const DataSource& source, const high_resolution_clock::time_point start, vector<Entry>& entries)
{
    std::cout << "Loading " << source.path << std::endl;

    std::ifstream file(source.path);
    int64_t position_count = 0;
    std::string fen;
    while (!file.eof())
    {
        if (position_count >= source.position_limit)
        {
            break;
        }

        std::getline(file, fen);
        if (fen.empty())
        {
            break;
        }

        Entry entry;
        entry.wdl = get_fen_wdl(fen);
        get_coefficient_entries(fen, entry.coefficients);
        entries.push_back(entry);
        
        position_count++;
        if (position_count % 100000 == 0)
        {
            print_elapsed(start);
            std::cout << "Loaded " << position_count << " entries..." << std::endl;
        }
    }
    std::cout << "Loaded " << position_count << " entries from " << source.path << ", " << entries.size() << " total" << std::endl;
}

void Tuner::run(const std::vector<DataSource>& sources)
{
    cout << "Starting texel tuning" << endl << endl;
    const auto start = high_resolution_clock::now();

    vector<Entry> entries;
    for (const auto& source : sources)
    {
        load_fens(source, start, entries);
    }

    cout << "Data loading complete";
}
