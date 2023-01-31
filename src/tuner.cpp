#include "tuner.h"
#include "base.h"
#include "config.h"
#include "threadpool.h"

#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
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
    bool white_to_move;
    tune_t initial_eval;
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

bool get_fen_color_to_move(const string& fen)
{
    return fen.find('w') != std::string::npos;
}

static void print_elapsed(high_resolution_clock::time_point start)
{
    const auto now = high_resolution_clock::now();
    const auto elapsed = now - start;
    const auto elapsed_seconds = duration_cast<seconds>(elapsed).count();
    cout << "[" << elapsed_seconds << "s] ";
}

static void get_coefficient_entries(const string& fen, vector<CoefficientEntry>& coefficient_entries)
{
    const auto coefficients = TuneEval::get_fen_coefficients(fen);
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

static tune_t linear_eval(const Entry& entry, const parameters_t& parameters)
{
    tune_t score = 0;
    for (const auto& coefficient : entry.coefficients)
    {
        score += coefficient.value * parameters[coefficient.index];
    }

    //if(!entry.white_to_move)
    //{
    //    score = -score;
    //}

    return score;
}

static void load_fens(const DataSource& source, const parameters_t& parameters, const high_resolution_clock::time_point start, vector<Entry>& entries)
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
        entry.white_to_move = get_fen_color_to_move(fen);
        get_coefficient_entries(fen, entry.coefficients);
        entry.initial_eval = linear_eval(entry, parameters);
        entries.push_back(entry);
        
        position_count++;
        if (position_count % 100000 == 0)
        {
            print_elapsed(start);
            std::cout << "Loaded " << position_count << " entries..." << std::endl;
        }
    }

    print_elapsed(start);
    std::cout << "Loaded " << position_count << " entries from " << source.path << ", " << entries.size() << " total" << std::endl;
}

static tune_t sigmoid(tune_t K, tune_t eval)
{
    return 1.0 / (1.0 + exp(-K * eval / 400.0));
}

static tune_t get_average_error(ThreadPool& thread_pool, const vector<Entry>& entries, const parameters_t& parameters, double K)
{
    array<tune_t, thread_count> thread_errors;
    for(int thread_id = 0; thread_id < thread_count; thread_id++)
    {
        thread_pool.enqueue([thread_id, &thread_errors, &entries, &parameters, K]()
        {
            const auto entries_per_thread = entries.size() / thread_count;
            const auto start = static_cast<int>(thread_id * entries_per_thread);
            const auto end = static_cast<int>((thread_id + 1) * entries_per_thread - 1);
            tune_t error = 0;
            for (int i = start; i < end; i++)
            {
                const auto& entry = entries[i];
                const auto eval = linear_eval(entry, parameters);
                const auto sig = sigmoid(K, eval);
                const auto diff = entry.wdl - sig;
                const auto entry_error = pow(diff, 2);
                error += entry_error;
            }
            thread_errors[thread_id] = error;
        });
    }

    thread_pool.wait_for_completion();

    tune_t total_error = 0;
    for (int thread_id = 0; thread_id < thread_count; thread_id++)
    {
        for (auto parameter_index = 0; parameter_index < parameters.size(); parameter_index++)
        {
            total_error += thread_errors[thread_id];
        }
    }

    const tune_t avg_error = total_error / static_cast<tune_t>(entries.size());
    return avg_error;
}

static double find_optimal_k(ThreadPool& thread_pool, const vector<Entry>& entries, const parameters_t& parameters)
{
    constexpr tune_t rate = 10;
    constexpr tune_t delta = 1e-5;
    constexpr tune_t deviation_goal = 1e-6;
    tune_t K = 2;
    tune_t deviation = 1;

    while (fabs(deviation) > deviation_goal) {
        const tune_t up = get_average_error(thread_pool, entries, parameters, K + delta);
        const tune_t down = get_average_error(thread_pool, entries, parameters, K - delta);
        deviation = (up - down) / (2 * delta);
        K -= deviation * rate;
    }

    return K;
}

static void update_single_gradient(parameters_t& gradient, const Entry& entry, const parameters_t& params, double K) {

    const tune_t eval = linear_eval(entry, params);
    const tune_t sig = sigmoid(K, eval);
    const tune_t res = (entry.wdl - sig) * sig * (1 - sig);

    for (const auto& coefficient : entry.coefficients)
    {
        gradient[coefficient.index] += res * coefficient.value;
    }
}

static void compute_gradient(ThreadPool& thread_pool, parameters_t& gradient, const vector<Entry>& entries, const parameters_t& params, double K)
{
    array<parameters_t, thread_count> thread_gradients;
    for(int thread_id = 0; thread_id < thread_count; thread_id++)
    {
        thread_pool.enqueue([thread_id, &thread_gradients, &entries, &params, K]()
        {
            const auto entries_per_thread = entries.size() / thread_count;
            const auto start = static_cast<int>(thread_id * entries_per_thread);
            const auto end = static_cast<int>((thread_id + 1) * entries_per_thread - 1);
            parameters_t gradient = parameters_t(params.size(), 0);
            for (int i = start; i < end; i++)
            {
                const auto& entry = entries[i];
                update_single_gradient(gradient, entry, params, K);
            }
            thread_gradients[thread_id] = gradient;
        });
    }

    thread_pool.wait_for_completion();

    for (int thread_id = 0; thread_id < thread_count; thread_id++)
    {
        for(auto parameter_index = 0; parameter_index < params.size(); parameter_index++)
        {
            gradient[parameter_index] += thread_gradients[thread_id][parameter_index];
        }
    }
}

void Tuner::run(const std::vector<DataSource>& sources)
{
    cout << "Starting tuning" << endl << endl;
    const auto start = high_resolution_clock::now();

    cout << "Starting thread pool..." << endl;
    ThreadPool thread_pool;
    thread_pool.start(thread_count);

    cout << "Getting initial parameters..." << endl;
    auto parameters = TuneEval::get_initial_parameters();

    vector<Entry> entries;

    // Debug entry
    //const string debug_fen = "rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQK1NR w KQkq - 0 1; 1.0";
    //Entry debug_entry;
    //debug_entry.wdl = get_fen_wdl(debug_fen);
    //debug_entry.white_to_move = get_fen_color_to_move(debug_fen);
    //get_coefficient_entries(debug_fen, debug_entry.coefficients);
    //debug_entry.initial_eval = linear_eval(debug_entry, parameters);
    //entries.push_back(debug_entry);

    for (const auto& source : sources)
    {
        load_fens(source, parameters, start, entries);
    }
    cout << "Data loading complete" << endl;

    cout << "Finding optimal K..." << endl;
    const auto K = find_optimal_k(thread_pool, entries, parameters);
    cout << "K = " << K << endl;

    const auto avg_error = get_average_error(thread_pool, entries, parameters, K);
    cout << "Initial error = " << avg_error << endl;
    cout << "Initial parameters:" << endl;
    TuneEval::print_parameters(parameters);

    const auto loop_start = high_resolution_clock::now();
    tune_t learning_rate = 0.03;
    parameters_t momentum(parameters.size(), 0);
    parameters_t velocity(parameters.size(), 0);
    for (int epoch = 1; epoch < 1000000; epoch++)
    {
        parameters_t gradient(parameters.size(), 0);
        compute_gradient(thread_pool, gradient, entries, parameters, K);

        constexpr tune_t beta1 = 0.9;
        constexpr tune_t beta2 = 0.999;

        for (int i = 0; i < parameters.size(); i++) {
            const tune_t grad = -K / 400.0 * gradient[i] / static_cast<tune_t>(entries.size());
            momentum[i] = beta1 * momentum[i] + (1 - beta1) * grad;
            velocity[i] = beta2 * velocity[i] + (1 - beta2) * pow(grad, 2);
            parameters[i] -= learning_rate * momentum[i] / (1e-8 + sqrt(velocity[i]));
        }

        if (epoch % 100 == 0)
        {
            const auto elapsed_ms = duration_cast<milliseconds>(high_resolution_clock::now() - loop_start).count();
            const auto epochs_per_second = epoch * 1000.0 / elapsed_ms;
            const tune_t error = get_average_error(thread_pool, entries, parameters, K);
            print_elapsed(start);
            cout << "Epoch " << epoch << " (" << epochs_per_second << " eps), error " << error << ", LR " << learning_rate << endl;
            TuneEval::print_parameters(parameters);
        }

        constexpr int lr_drop_interval = 10000;
        constexpr int lr_drop_ratio = 0.3;
        if(epoch == lr_drop_interval)
        {
            learning_rate *= lr_drop_ratio;
        }
    }
}
