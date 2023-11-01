#include "tuner.h"
#include "base.h"
#include "config.h"
#include "threadpool.h"
#include "external/chess.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
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
    //tune_t initial_eval;
    tune_t additional_score;
#if TAPERED
    int32_t phase;
    tune_t endgame_scale;
#endif
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

static tune_t get_fen_wdl(const string& original_fen, const bool original_white_to_move, const bool white_to_move, const bool side_to_move_wdl)
{
    tune_t wdl;
    bool marker_found = false;
    for (auto& marker : markers)
    {
        if (original_fen.find(marker.marker) != std::string::npos)
        {
            if (marker_found)
            {
                cout << "WDL marker already found on line " << original_fen << endl;
                throw std::runtime_error("WDL marker already found");
            }
            marker_found = true;
            wdl = marker.wdl;
        }
    }

    if(!marker_found)
    {
        stringstream ss(original_fen);
        while (!ss.eof())
        {
            string word;
            ss >> word;
            if (word.starts_with("0."))
            {
                wdl = stod(word);
                marker_found = true;
            }
        }
    }

    if (!marker_found)
    {
        cout << "WDL marker not found on line " << original_fen << endl;
        throw std::runtime_error("WDL marker not found");
    }

    if(!original_white_to_move && side_to_move_wdl)
    {
        wdl = 1 - wdl;
    }

    return wdl;
}   

static bool get_fen_color_to_move(const string& fen)
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

static void get_coefficient_entries(const coefficients_t& coefficients, vector<CoefficientEntry>& coefficient_entries, int32_t parameter_count)
{
    if(coefficients.size() != parameter_count)
    {
        throw runtime_error("Parameter count mismatch");
    }

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
    tune_t score = entry.additional_score;
#if TAPERED 
    tune_t midgame = 0;
    tune_t endgame = 0;
    for (const auto& coefficient : entry.coefficients)
    {
        midgame += coefficient.value * parameters[coefficient.index][static_cast<int32_t>(PhaseStages::Midgame)];
        endgame += coefficient.value * parameters[coefficient.index][static_cast<int32_t>(PhaseStages::Endgame)] * entry.endgame_scale;
    }
    score += (midgame * entry.phase + endgame * (24 - entry.phase)) / 24;
#else
    for (const auto& coefficient : entry.coefficients)
    {
        score += coefficient.value * parameters[coefficient.index];
    }
#endif

    return score;
}

static int32_t get_phase(const string& fen)
{
    int32_t phase = 0;
    auto stop = false;
    for(const char ch : fen)
    {
        if(stop)
        {
            break;
        }

        switch (ch)
        {
        case 'n':
        case 'N':
            phase += 1;
            break;
        case 'b':
        case 'B':
            phase += 1;
            break;
        case 'r':
        case 'R':
            phase += 2;
            break;
        case 'q':
        case 'Q':
            phase += 4;
            break;
        case 'p':
        case '/':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
            break;
        case ' ':
            stop = true;
            break;
        }
    }
    return phase;
}

static int32_t get_phase(const Chess::Board& board)
{
    int32_t phase = 0;

    for(Chess::Square square = static_cast<Chess::Square>(0); square < 64; ++square)
    {
        auto piece = board.pieceAt(square);
        switch (piece)
        {
        case Chess::Piece::WHITEKNIGHT:
        case Chess::Piece::WHITEBISHOP:
        case Chess::Piece::BLACKKNIGHT:
        case Chess::Piece::BLACKBISHOP:
            phase += 1;
            break;
        case Chess::Piece::WHITEROOK:
        case Chess::Piece::BLACKROOK:
            phase += 2;
            break;
        case Chess::Piece::WHITEQUEEN:
        case Chess::Piece::BLACKQUEEN:
            phase += 4;
            break;
        }
    }

    return phase;
}

static void print_statistics(const parameters_t& parameters, const vector<Entry>& entries)
{
    array<size_t, 2> wins{};
    array<size_t, 2> draws{};
    array<size_t, 2> losses{};
    array<size_t, 2> total{};
    array<tune_t, 2> wdls{};

    for(auto& entry : entries)
    {
        if(entry.wdl == 1)
        {
            wins[entry.white_to_move]++;
        }
        else if(entry.wdl == 0.5)
        {
            draws[entry.white_to_move]++;
        }
        else if (entry.wdl == 0.0)
        {
            losses[entry.white_to_move]++;
        }
        total[entry.white_to_move]++;
        wdls[entry.white_to_move] += entry.wdl;
    }

    cout << "Dataset statistics:" << endl;
    cout << "Total positions: " << entries.size() << endl;
    for(int color = 1; color >= 0; color--)
    {
        const auto color_name = color ? "White" : "Black";
        cout << color_name << ": " << total[color] << " (" << (total[color] * 100.0 / entries.size()) << "%)" << endl;
        cout << color_name << " 1.0: " << wins[color] << " (" << (wins[color] * 100.0 / entries.size()) << "%)" << endl;
        cout << color_name << " 0.5: " << draws[color] << " (" << (draws[color] * 100.0 / entries.size()) << "%)" << endl;
        cout << color_name << " 0.0: " << losses[color] << " (" << (losses[color] * 100.0 / entries.size()) << "%)" << endl;
        cout << color_name << " avg: " << wdls[color] / total[color] << endl;
    }

    cout << endl;
}

constexpr tune_t inf = 1 << 20;
struct PvEntry
{
    array<Chess::Move, 64> moves{};
    int32_t length = 0;
};
using pv_table_t = array<PvEntry, 64>;

static int32_t get_piece_value(const Chess::Piece piece)
{
    switch (piece)
    {
    case Chess::Piece::WHITEPAWN:
    case Chess::Piece::BLACKPAWN:
        return 100;
    case Chess::Piece::WHITEKNIGHT:
    case Chess::Piece::BLACKKNIGHT:
        return 300;
    case Chess::Piece::WHITEBISHOP:
    case Chess::Piece::BLACKBISHOP:
        return 300;
    case Chess::Piece::WHITEROOK:
    case Chess::Piece::BLACKROOK:
        return 500;
    case Chess::Piece::WHITEQUEEN:
    case Chess::Piece::BLACKQUEEN:
        return 900;
    case Chess::Piece::WHITEKING:
    case Chess::Piece::BLACKKING:
    case Chess::Piece::NONE:
        return 0;
        //throw std::runtime_error("Invalid piece for value");
    }
}

static int32_t mvv_lva(const Chess::Board& board, const Chess::Move move)
{
    const auto from = move.from();
    const auto to = move.to();
    const auto piece = board.pieceAt(from);
    Chess::Piece takes;
    const auto type = move.typeOf();
    if(type == Chess::Move::EN_PASSANT)
    {
        takes = board.sideToMove() == Chess::Color::WHITE ? Chess::Piece::BLACKPAWN : Chess::Piece::WHITEPAWN;
    }
    else
    {
        takes = board.pieceAt(to);
    }

    auto score = get_piece_value(takes);
    score <<= 16;
    score -= get_piece_value(piece);
    return score;
}

static tune_t quiescence(Chess::Board& board, const parameters_t& parameters, pv_table_t& pv_table, tune_t alpha, tune_t beta, const int32_t ply)
{
    pv_table[ply].length = 0;

    EvalResult eval_result;
    if constexpr (TuneEval::supports_external_chess_eval)
    {
        eval_result = TuneEval::get_external_eval_result(board);
    }
    else
    {
        auto fen = board.getFen();
        eval_result = TuneEval::get_fen_eval_result(fen);
    }

    Entry entry;
    entry.white_to_move = board.sideToMove() == Chess::Color::WHITE;
#if TAPERED
    entry.endgame_scale = eval_result.endgame_scale;
#endif
    get_coefficient_entries(eval_result.coefficients, entry.coefficients, static_cast<int32_t>(parameters.size()));
#if TAPERED
    entry.phase = get_phase(board);
#endif
    entry.additional_score = 0;
    tune_t eval = linear_eval(entry, parameters);
    if(!entry.white_to_move)
    {
        eval = -eval;
    }

    if (eval >= beta)
    {
        return eval;
    }

    if (eval > alpha)
    {
        alpha = eval;
    }

    Chess::Movelist<Chess::Move> moves;
    Chess::Movegen::legalmoves<Chess::Move, Chess::MoveGenType::CAPTURE>(moves, board);
    array<int32_t, 64> move_scores;
    for (int32_t move_index = 0; move_index < moves.size(); move_index++)
    {
        move_scores[move_index] = mvv_lva(board, moves[move_index]);
    }

    if(moves.size() == 0)
    {
        return alpha;
    }

    tune_t best_score = -inf;
    auto best_move = Chess::Move(Chess::Move::NO_MOVE);
    //for (const auto& move : movelist) {
    for(int32_t move_index = 0; move_index < moves.size(); move_index++)
    {
        int32_t best_move_score = 0;
        int32_t best_move_index = 0;
        for(auto i = move_index; i < moves.size(); i++)
        {
            if(move_scores[i] > best_move_score)
            {
                best_move_score = move_scores[i];
                best_move_index = i;
            }
        }

        const auto move = moves[best_move_index];
        moves[best_move_index] = moves[move_index];
        move_scores[best_move_index] = move_scores[move_index];

        board.makeMove(move);

        const auto child_score = -quiescence(board, parameters, pv_table, -beta, -alpha, ply + 1);
        if(child_score > best_score)
        {
            best_score = child_score;
            best_move = move;
            if (child_score > alpha)
            {
                alpha = child_score;
                if(child_score >= beta)
                {
                    board.unmakeMove(move);
                    break;
                }

                auto& this_ply = pv_table[ply];
                auto& next_ply = pv_table[ply + 1];
                this_ply.moves[0] = move;
                this_ply.length = next_ply.length + 1;
                for (int32_t nextPlyIndex = 0; nextPlyIndex < next_ply.length; nextPlyIndex++)
                {
                    this_ply.moves[nextPlyIndex + 1] = next_ply.moves[nextPlyIndex];
                }
            }
        }

        board.unmakeMove(move);
    }

    return best_score;
}

string cleanup_fen(const string& initial_fen)
{
    int space_count = 0;
    size_t pos = 0;
    for (size_t i = 0; i < initial_fen.size(); ++i) {
        if (initial_fen[i] == ' ') {
            ++space_count;
        }
        if (space_count == 4) {
            pos = i;
            break;
        }
    }
    const auto clean_fen = initial_fen.substr(0, pos);
    return clean_fen;
}

Chess::Board quiescence_root(const parameters_t& parameters, const string& initial_fen)
{
    pv_table_t pv_table {};
    const auto clean_fen = cleanup_fen(initial_fen);
    auto board = Chess::Board(clean_fen);
    auto score = quiescence(board, parameters, pv_table, -inf, inf, 0);
    if(board.sideToMove() == Chess::Color::BLACK)
    {
        score = -score;
    }
    if constexpr (print_data_entries)
    {
        if (pv_table[0].length > 0)
        {
            cout << " PV:";
            for (int32_t pv_index = 0; pv_index < pv_table[0].length; pv_index++)
            {
                cout << " " << pv_table[0].moves[pv_index];
            }
        }
        cout << " QS: " << score;
    }

    for (int32_t pv_index = 0; pv_index < pv_table[0].length; pv_index++)
    {
        board.makeMove(pv_table[0].moves[pv_index]);
    }

    return board;
}

static void parse_fen(const bool side_to_move_wdl, const parameters_t& parameters, vector<Entry>& entries, const string& original_fen)
{
    if constexpr (print_data_entries)
    {
        //cout << fen;
    }

    //string fen;
    Chess::Board board;
    if constexpr (enable_qsearch)
    {
        board = quiescence_root(parameters, original_fen);
    }
    else
    {
        const auto clean_fen = cleanup_fen(original_fen);
        board = Chess::Board(clean_fen);
    }

    EvalResult eval_result;
    if constexpr (TuneEval::supports_external_chess_eval)
    {
        eval_result = TuneEval::get_external_eval_result(board);
    }
    else
    {
        auto fen = board.getFen();
        eval_result = TuneEval::get_fen_eval_result(fen);
    }

    Entry entry;
    //entry.white_to_move = get_fen_color_to_move(fen);
    entry.white_to_move = board.sideToMove() == Chess::Color::WHITE;
#if TAPERED
    entry.endgame_scale = eval_result.endgame_scale;
#endif
    const bool original_white_to_move = get_fen_color_to_move(original_fen);
    //cout << (entry.white_to_move ? "w" : "b") << " ";
    entry.wdl = get_fen_wdl(original_fen, original_white_to_move, entry.white_to_move, side_to_move_wdl);
    get_coefficient_entries(eval_result.coefficients, entry.coefficients, static_cast<int32_t>(parameters.size()));
#if TAPERED
    entry.phase = get_phase(board);
#endif
    entry.additional_score = 0;
    if constexpr (TuneEval::includes_additional_score)
    {
        const tune_t score = linear_eval(entry, parameters);
        if constexpr (print_data_entries)
        {
            cout << " Eval: " << score << endl;
        }
        entry.additional_score = eval_result.score - score;
    }

    entries.push_back(entry);
}

static void read_fens(const DataSource& source, const high_resolution_clock::time_point start, vector<string>& fens)
{
    cout << "Reading " << source.path;
    if (source.position_limit > 0)
    {
        cout << " (" << source.position_limit << " positions)";
    }
    cout << "..." << endl;

    ifstream file(source.path);
    if (!file)
    {
        cout << "Failed to open " << source.path << endl;
        throw runtime_error("Failed to open data source");
    }

    while (!file.eof())
    {
        if (source.position_limit > 0 && fens.size() >= source.position_limit)
        {
            break;
        }

        string original_fen;
        getline(file, original_fen);
        if (original_fen.empty())
        {
            break;
        }

        fens.push_back(original_fen);
    }

    print_elapsed(start);
    std::cout << "Read " << fens.size() << " positions from " << source.path << endl;
}

static void parse_fens(ThreadPool& thread_pool, const DataSource& source, const vector<string>& fens, const parameters_t& parameters, const high_resolution_clock::time_point time_start, vector<Entry>& entries)
{
    cout << "Parsing " << fens.size() << " positions..." << endl;
    array<vector<Entry>, data_load_thread_count> thread_entries;
    const auto side_to_move_wdl = source.side_to_move_wdl;
    constexpr int batch_size = 10000;
    mutex mut;
    queue<vector<string>> batches;
    vector<string> current_batch;
    for(const auto& fen : fens)
    {
        current_batch.push_back(fen);
        if (current_batch.size() == batch_size)
        {
            batches.emplace(current_batch);
            current_batch.clear();
        }
    }
    if(!current_batch.empty())
    {
        batches.emplace(current_batch);
    }

    for (int thread_id = 0; thread_id < data_load_thread_count; thread_id++)
    {
        thread_pool.enqueue([thread_id, &thread_entries, &mut, side_to_move_wdl, parameters, &batches, time_start]()
        {
            vector<Entry> entries;

            int position_count = 0;
            while(true)
            {
                vector<string> thread_batch;
                {
                    lock_guard lock(mut);
                    if(batches.empty())
                    {
                        break;
                    }
                    thread_batch = batches.front();
                    batches.pop();
                }

                constexpr auto thread_data_load_print_interval = data_load_print_interval / data_load_thread_count;
                for(auto& fen : thread_batch)
                {
                    parse_fen(side_to_move_wdl, parameters, entries, fen);
                    position_count++;
                    if (thread_id == 0 && position_count % thread_data_load_print_interval == 0)
                    {
                        print_elapsed(time_start);
                        std::cout << "Parsed ~" << position_count * data_load_thread_count << " positions..." << endl;
                    }
                }
            }

            thread_entries[thread_id] = entries;
        });
    }

    thread_pool.wait_for_completion();

    for (int thread_id = 0; thread_id < data_load_thread_count; thread_id++)
    {
        for(const Entry& entry : thread_entries[thread_id])
        {
            entries.push_back(entry);
        }
    }
}

static void load_fens(ThreadPool& thread_pool, const DataSource& source, const parameters_t& parameters, const high_resolution_clock::time_point start, vector<Entry>& entries)
{
    vector<string> fens;
    read_fens(source, start, fens);
    parse_fens(thread_pool, source, fens, parameters, start, entries);
}

static tune_t sigmoid(const tune_t K, const tune_t eval)
{
    return static_cast<tune_t>(1) / (static_cast<tune_t>(1) + exp(-K * eval / static_cast<tune_t>(400)));
}

static tune_t get_average_error(ThreadPool& thread_pool, const vector<Entry>& entries, const parameters_t& parameters, tune_t K)
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
        total_error += thread_errors[thread_id];
    }

    const tune_t avg_error = total_error / static_cast<tune_t>(entries.size());
    return avg_error;
}

static tune_t find_optimal_k(ThreadPool& thread_pool, const vector<Entry>& entries, const parameters_t& parameters)
{
    constexpr tune_t rate = 10;
    constexpr tune_t delta = 1e-5;
    constexpr tune_t deviation_goal = 1e-6;
    tune_t K = 2.5;
    tune_t deviation = 1;

    while (fabs(deviation) > deviation_goal)
    {
        const tune_t up = get_average_error(thread_pool, entries, parameters, K + delta);
        const tune_t down = get_average_error(thread_pool, entries, parameters, K - delta);
        deviation = (up - down) / (2 * delta);
        cout << "Current K: " << K << ", up: " << up << ", down: " << down << ", deviation: " << deviation << endl;
        K -= deviation * rate;
    }

    return K;
}

static void update_single_gradient(parameters_t& gradient, const Entry& entry, const parameters_t& params, tune_t K) {

    const tune_t eval = linear_eval(entry, params);
    const tune_t sig = sigmoid(K, eval);
    const tune_t res = (entry.wdl - sig) * sig * (1 - sig);

#if TAPERED
    const auto mg_base = res * (entry.phase / static_cast<tune_t>(24));
    const auto eg_base = res - mg_base;
#endif

    for (const auto& coefficient : entry.coefficients)
    {
#if TAPERED
        gradient[coefficient.index][static_cast<int32_t>(PhaseStages::Midgame)] += mg_base * coefficient.value;
        gradient[coefficient.index][static_cast<int32_t>(PhaseStages::Endgame)] += eg_base * coefficient.value * entry.endgame_scale;
#else
        gradient[coefficient.index] += res * coefficient.value;
#endif
    }
}

static void compute_gradient(ThreadPool& thread_pool, parameters_t& gradient, const vector<Entry>& entries, const parameters_t& params, tune_t K)
{
    array<parameters_t, thread_count> thread_gradients;
    for(int thread_id = 0; thread_id < thread_count; thread_id++)
    {
        thread_pool.enqueue([thread_id, &thread_gradients, &entries, &params, K]()
        {
            const auto entries_per_thread = entries.size() / thread_count;
            const auto start = static_cast<int>(thread_id * entries_per_thread);
            const auto end = static_cast<int>((thread_id + 1) * entries_per_thread - 1);
#if TAPERED
            parameters_t gradient = parameters_t(params.size(), pair_t{});
#else
            parameters_t gradient = parameters_t(params.size(), 0);
#endif
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
#if TAPERED
            gradient[parameter_index][static_cast<int32_t>(PhaseStages::Midgame)] += thread_gradients[thread_id][parameter_index][static_cast<int32_t>(PhaseStages::Midgame)];
            gradient[parameter_index][static_cast<int32_t>(PhaseStages::Endgame)] += thread_gradients[thread_id][parameter_index][static_cast<int32_t>(PhaseStages::Endgame)];
#else
            gradient[parameter_index] += thread_gradients[thread_id][parameter_index];
#endif
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
    cout << "Got " << parameters.size() << " parameters" << endl;

    cout << "Initial parameters:" << endl;
    TuneEval::print_parameters(parameters);

    vector<Entry> entries;

    // Debug entry
    //const string debug_fen = "rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQK1NR w KQkq - 0 1; 1.0";
    //Entry debug_entry;
    //debug_entry.wdl = get_fen_wdl(debug_fen);
    //debug_entry.white_to_move = get_fen_color_to_move(debug_fen);
    //get_coefficient_entries(debug_fen, debug_entry.coefficients);
    //debug_entry.initial_eval = linear_eval(debug_entry, parameters);
    //entries.push_back(debug_entry);

    vector<string> fens;
    for (const auto& source : sources)
    {
        load_fens(thread_pool, source, parameters, start, entries);
    }
    cout << "Data loading complete" << endl << endl;

    print_statistics(parameters, entries);

    if constexpr (retune_from_zero)
    {
        for (auto& parameter : parameters)
        {
#if TAPERED
            parameter[static_cast<int>(PhaseStages::Midgame)] = static_cast<tune_t>(0);
            parameter[static_cast<int>(PhaseStages::Endgame)] = static_cast<tune_t>(0);
#else
            parameter = static_cast<tune_t>(0);
#endif            
        }
    }

    cout << "Initial parameters:" << endl;
    TuneEval::print_parameters(parameters);

    tune_t K;
    if constexpr (preferred_k <= 0)
    {
        cout << "Finding optimal K..." << endl;
        K = find_optimal_k(thread_pool, entries, parameters);
    }
    else
    {
        cout << "Using predefined K = " << preferred_k <<  endl;
        K = preferred_k;
    }
    cout << "K = " << K << endl;

    const auto avg_error = get_average_error(thread_pool, entries, parameters, K);
    cout << "Initial error = " << avg_error << endl;

    const auto loop_start = high_resolution_clock::now();
    tune_t learning_rate = initial_learning_rate;
    int32_t max_tune_epoch = max_epoch;
#if TAPERED
    parameters_t momentum(parameters.size(), pair_t{});
    parameters_t velocity(parameters.size(), pair_t{});
#else
    parameters_t momentum(parameters.size(), 0);
    parameters_t velocity(parameters.size(), 0);
#endif
    for (int32_t epoch = 1; epoch < max_tune_epoch; epoch++)
    {
#if TAPERED
        parameters_t gradient(parameters.size(), pair_t{});
#else
        parameters_t gradient(parameters.size(), 0);
#endif
        
        compute_gradient(thread_pool, gradient, entries, parameters, K);

        constexpr tune_t beta1 = 0.9;
        constexpr tune_t beta2 = 0.999;

        for (int parameter_index = 0; parameter_index < parameters.size(); parameter_index++) {
#if TAPERED
            for(int phase_stage = 0; phase_stage < 2; phase_stage++)
            {
                const tune_t grad = -K / static_cast<tune_t>(400) * gradient[parameter_index][phase_stage] / static_cast<tune_t>(entries.size());
                momentum[parameter_index][phase_stage] = beta1 * momentum[parameter_index][phase_stage] + (1 - beta1) * grad;
                velocity[parameter_index][phase_stage] = beta2 * velocity[parameter_index][phase_stage] + (1 - beta2) * pow(grad, 2);
                parameters[parameter_index][phase_stage] -= learning_rate * momentum[parameter_index][phase_stage] / (static_cast<tune_t>(1e-8) + sqrt(velocity[parameter_index][phase_stage]));
            }
#else
            const tune_t grad = -K / 400.0 * gradient[parameter_index] / static_cast<tune_t>(entries.size());
            momentum[parameter_index] = beta1 * momentum[parameter_index] + (1 - beta1) * grad;
            velocity[parameter_index] = beta2 * velocity[parameter_index] + (1 - beta2) * pow(grad, 2);
            parameters[parameter_index] -= learning_rate * momentum[parameter_index] / (1e-8 + sqrt(velocity[parameter_index]));
#endif
            
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

        if(epoch % learning_rate_drop_interval == 0)
        {
            learning_rate *= learning_rate_drop_ratio;
        }
    }

    thread_pool.stop();
}
