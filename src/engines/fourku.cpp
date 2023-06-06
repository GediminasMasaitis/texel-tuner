// THIS FILE IS LICENSED MIT

#include "fourku.h"
#include "../base.h"

#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <sstream>

using namespace std;
using namespace Fourku;

using u64 = uint64_t;

enum
{
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
    None
};

struct [[nodiscard]] Position {
    array<int, 4> castling = { true, true, true, true };
    array<u64, 2> colour = { 0xFFFFULL, 0xFFFF000000000000ULL };
    array<u64, 6> pieces = { 0xFF00000000FF00ULL,
                            0x4200000000000042ULL,
                            0x2400000000000024ULL,
                            0x8100000000000081ULL,
                            0x800000000000008ULL,
                            0x1000000000000010ULL };
    u64 ep = 0x0ULL;
    int flipped = false;
};

[[nodiscard]] static u64 flip(u64 bb) {
    u64 result;
    char* bb_ptr = reinterpret_cast<char*>(&bb);
    char* result_ptr = reinterpret_cast<char*>(&result);
    for (int i = 0; i < sizeof(bb); i++)
    {
        result_ptr[i] = bb_ptr[sizeof(bb) - 1 - i];
    }
    return result;
}

[[nodiscard]] static auto lsb(const u64 bb) {
    return countr_zero(bb);
}

[[nodiscard]] static auto count(const u64 bb) {
    return popcount(bb);
}

[[nodiscard]] static auto east(const u64 bb) {
    return (bb << 1) & ~0x0101010101010101ULL;
}

[[nodiscard]] static auto west(const u64 bb) {
    return (bb >> 1) & ~0x8080808080808080ULL;
}

[[nodiscard]] static u64 north(const u64 bb) {
    return bb << 8;
}

[[nodiscard]] static u64 south(const u64 bb) {
    return bb >> 8;
}

[[nodiscard]] static u64 nw(const u64 bb) {
    return north(west(bb));
}

[[nodiscard]] static u64 ne(const u64 bb) {
    return north(east(bb));
}

[[nodiscard]] static u64 sw(const u64 bb) {
    return south(west(bb));
}

[[nodiscard]] static u64 se(const u64 bb) {
    return south(east(bb));
}

static void flip(Position& pos) {
    pos.colour[0] = flip(pos.colour[0]);
    pos.colour[1] = flip(pos.colour[1]);
    for (int i = 0; i < 6; ++i) {
        pos.pieces[i] = flip(pos.pieces[i]);
    }
    pos.ep = flip(pos.ep);
    swap(pos.colour[0], pos.colour[1]);
    swap(pos.castling[0], pos.castling[2]);
    swap(pos.castling[1], pos.castling[3]);
    pos.flipped = !pos.flipped;
}

template <typename F>
[[nodiscard]] auto ray(const int sq, const u64 blockers, F f) {
    u64 mask = f(1ULL << sq);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    return mask;
}

[[nodiscard]] u64 knight(const int sq, const u64) {
    const u64 bb = 1ULL << sq;
    return (((bb << 15) | (bb >> 17)) & 0x7F7F7F7F7F7F7F7FULL) | (((bb << 17) | (bb >> 15)) & 0xFEFEFEFEFEFEFEFEULL) |
        (((bb << 10) | (bb >> 6)) & 0xFCFCFCFCFCFCFCFCULL) | (((bb << 6) | (bb >> 10)) & 0x3F3F3F3F3F3F3F3FULL);
}

[[nodiscard]] auto bishop(const int sq, const u64 blockers) {
    return ray(sq, blockers, nw) | ray(sq, blockers, ne) | ray(sq, blockers, sw) | ray(sq, blockers, se);
}

[[nodiscard]] auto rook(const int sq, const u64 blockers) {
    return ray(sq, blockers, north) | ray(sq, blockers, east) | ray(sq, blockers, south) | ray(sq, blockers, west);
}

[[nodiscard]] u64 king(const int sq, const u64) {
    const u64 bb = 1ULL << sq;
    return (bb << 8) | (bb >> 8) | (((bb >> 1) | (bb >> 9) | (bb << 7)) & 0x7F7F7F7F7F7F7F7FULL) |
        (((bb << 1) | (bb << 9) | (bb >> 7)) & 0xFEFEFEFEFEFEFEFEULL);
}

static void set_fen(Position& pos, const string& fen) {
    // Clear
    pos.colour = {};
    pos.pieces = {};
    pos.castling = {};

    stringstream ss{ fen };
    string word;

    ss >> word;
    int i = 56;
    for (const auto c : word) {
        if (c >= '1' && c <= '8') {
            i += c - '1' + 1;
        }
        else if (c == '/') {
            i -= 16;
        }
        else {
            const int side = c == 'p' || c == 'n' || c == 'b' || c == 'r' || c == 'q' || c == 'k';
            const int piece = (c == 'p' || c == 'P') ? Pawn
                : (c == 'n' || c == 'N') ? Knight
                : (c == 'b' || c == 'B') ? Bishop
                : (c == 'r' || c == 'R') ? Rook
                : (c == 'q' || c == 'Q') ? Queen
                : King;
            pos.colour.at(side) ^= 1ULL << i;
            pos.pieces.at(piece) ^= 1ULL << i;
            i++;
        }
    }

    // Side to move
    ss >> word;
    const bool black_move = word == "b";

    // Castling permissions
    ss >> word;
    for (const auto c : word) {
        pos.castling[0] |= c == 'K';
        pos.castling[1] |= c == 'Q';
        pos.castling[2] |= c == 'k';
        pos.castling[3] |= c == 'q';
    }

    // En passant
    ss >> word;
    if (word != "-") {
        const int sq = word[0] - 'a' + 8 * (word[1] - '1');
        pos.ep = 1ULL << sq;
    }

    // Flip the board if necessary
    if (black_move) {
        flip(pos);
    }
}

struct Trace
{
    int score;

    int material[6][2]{};
    int pst_rank[6][8][2]{};
    int pst_file[6][8][2]{};
    int open_files[2][3][2]{};
    int mobilities[5][2]{};
    int pawn_protection[6][2]{};
    int passers[4][2]{};
    int pawn_doubled[2]{};
    int pawn_phalanx[2]{};
    int pawn_passed_protected[2]{};
    int pawn_passed_blocked[4][2]{};
    int pawn_passed_king_distance[2][2]{};
    int bishop_pair[2]{};
    int king_shield[2][2]{};
};

const int phases[] = { 0, 1, 1, 2, 4, 0 };
const int max_material[] = { 127, 392, 429, 744, 1344, 0, 0 };
const int material[] = { S(90, 124), S(384, 393), S(404, 431), S(515, 747), S(1174, 1348), 0 };
const int pst_rank[][8] = {
    {0, S(-6, 1), S(-5, -1), S(-1, -3), S(4, -1), S(8, 5), 0, 0},
    {S(-9, -8), S(-4, -3), 0, S(4, 7), S(9, 8), S(18, 1), S(9, -3), S(-28, -3)},
    {S(-7, -4), S(-1, -4), S(2, 0), S(3, 3), S(5, 4), S(10, 1), S(2, 0), S(-14, 0)},
    {S(-5, -2), S(-9, -4), S(-8, -4), S(-7, 1), S(0, 2), S(6, 1), S(9, 3), S(14, 3)},
    {S(-2, -14), S(0, -21), S(0, -11), S(-1, 4), S(-1, 11), S(5, 8), S(-4, 13), S(4, 9)},
    {S(-4, -7), S(-4, 1), S(-3, 2), S(-4, 5), S(0, 6), S(20, 4), S(11, 2), S(-1, -5)},
};
const int pst_file[][8] = {
    {S(-3, 1), S(-2, 1), S(-2, 0), S(1, -3), S(2, 0), S(4, 0), S(4, 1), S(-4, 0)},
    {S(-9, -9), S(-2, -2), S(1, 3), S(3, 7), S(3, 6), S(4, 3), S(2, -1), S(-3, -6)},
    {S(-5, -4), S(0, -1), S(1, 1), S(0, 2), S(0, 3), S(0, 2), S(4, 2), S(0, -4)},
    {S(-2, 0), S(-3, 1), S(-1, 2), S(1, 0), S(1, 0), S(2, 0), S(3, -1), S(0, -2)},
    {S(-5, -11), S(-1, -8), S(0, -3), S(-1, 5), S(-2, 6), S(1, 5), S(6, 1), S(3, 4)},
    {S(-1, -6), S(4, -2), S(-4, 2), S(-12, 5), S(-9, 4), S(-5, 3), S(4, 0), S(5, -7)},
};
const int open_files[][3] = {
    {S(25, 17), S(2, 27), S(-20, 3)},
    {S(52, 11), S(-1, 29), S(-54, -8)},
};
const int mobilities[] = { 0,0,0,0,0 };
const int pawn_protection[] = { S(20, 17), S(4, 20), S(1, 11), S(8, 8), S(-9, 16), S(-35, 22) };
const int passers[] = { S(-24, 17), S(-12, 49), S(7, 116), S(133, 235) };
const int pawn_passed_protected = S(15, 17);
const int pawn_doubled = S(-15, -27);
const int pawn_phalanx = S(10, 17);
const int pawn_passed_blocked[] = { S(-7, -16), S(6, -35), S(-2, -68), S(26, -106) };
const int pawn_passed_king_distance[] = { S(3, -6), S(-3, 9) };
const int bishop_pair = S(23, 69);
const int king_shield[] = { S(32, -8), S(24, -7) };
const int pawn_attacked[] = { S(-64, -14), S(-155, -142) };

#define TraceIncr(parameter) trace.parameter[color]++
#define TraceAdd(parameter, count) trace.parameter[color] += count

static Trace eval(Position& pos) {
    Trace trace{};
    int score = S(28, 10);
    int phase = 0;

    for (int c = 0; c < 2; ++c) {
        const int color = pos.flipped;

        // our pawns, their pawns
        const u64 pawns[] = { pos.colour[0] & pos.pieces[Pawn], pos.colour[1] & pos.pieces[Pawn] };
        const u64 protected_by_pawns = nw(pawns[0]) | ne(pawns[0]);
        const u64 attacked_by_pawns = se(pawns[1]) | sw(pawns[1]);
        const int kings[] = { lsb(pos.colour[0] & pos.pieces[King]), lsb(pos.colour[1] & pos.pieces[King]) };
        const u64 all_pieces = pos.colour[0] | pos.colour[1];

        // Bishop pair
        if (count(pos.colour[0] & pos.pieces[Bishop]) == 2) {
            score += bishop_pair;
            TraceIncr(bishop_pair);
        }

        // Doubled pawns
        score += pawn_doubled * count((north(pawns[0]) | north(north(pawns[0]))) & pawns[0]);
        TraceAdd(pawn_doubled, count((north(pawns[0]) | north(north(pawns[0]))) & pawns[0]));

        // Phalanx pawns
        score += pawn_phalanx * count(west(pawns[0]) & pawns[0]);
        TraceAdd(pawn_phalanx, count(west(pawns[0]) & pawns[0]));

        // For each piece type
        for (int p = 0; p < 6; ++p) {
            auto copy = pos.colour[0] & pos.pieces[p];
            while (copy) {
                const int sq = lsb(copy);
                copy &= copy - 1;

                // Material
                phase += phases[p];
                score += material[p];
                TraceIncr(material[p]);

                const int rank = sq / 8;
                const int file = sq % 8;

                // Split quantized PSTs
                if (p != Pawn || (rank != 0 && rank != 6 && rank != 7)) // Special for tuner. Rank 6 = guaranteed passer
                {
                    score += pst_rank[p][rank] * 4;
                    TraceAdd(pst_rank[p][rank], 4);
                }
                score += pst_file[p][file] * 4;
                TraceAdd(pst_file[p][file], 4);

                // Pawn protection
                const u64 piece_bb = 1ULL << sq;
                if (piece_bb & protected_by_pawns) {
                    score += pawn_protection[p];
                    TraceIncr(pawn_protection[p]);
                }

                if (p == Pawn) {
                    // Passed pawns
                    if (rank > 2 && !(0x101010101010101ULL << sq & (pawns[1] | attacked_by_pawns))) {
                        score += passers[rank - 3];
                        TraceIncr(passers[rank - 3]);

                        if (piece_bb & protected_by_pawns) {
                            score += pawn_passed_protected;
                            TraceIncr(pawn_passed_protected);
                        }

                        // Blocked passed pawns
                        if (north(piece_bb) & pos.colour[1]) {
                            score += pawn_passed_blocked[rank - 3];
                            TraceIncr(pawn_passed_blocked[rank - 3]);
                        }

                        // King defense/attack
                        // king distance to square in front of passer
                        for (int i = 0; i < 2; ++i) {
                            score += pawn_passed_king_distance[i] * (rank - 1) * max(abs((kings[i] / 8) - (rank + 1)), abs((kings[i] % 8) - file));
                            TraceAdd(pawn_passed_king_distance[i], (rank - 1) * max(abs((kings[i] / 8) - (rank + 1)), abs((kings[i] % 8) - file)));
                        }
                    }
                }
                else {
                    // Pawn attacks
                    if (piece_bb & attacked_by_pawns) {
                        // If we're to move, we'll just lose some options and our tempo.
                        // If we're not to move, we lose a piece?
                        score += pawn_attacked[c];
                    }

                    // Open or semi-open files
                    const u64 file_bb = 0x101010101010101ULL << file;
                    if (p > Bishop && !(file_bb & pawns[0])) {
                        score += open_files[!(file_bb & pawns[1])][p - 3];
                        TraceIncr(open_files[!(file_bb & pawns[1])][p - 3]);
                    }

                    u64 mobility = 0;
                    if(p == Knight) {
                        mobility = knight(sq, all_pieces);
                    }
                    else if(p == Bishop)
                    {
                        mobility = bishop(sq, all_pieces);
                    }
                    else if(p == Rook) {
                        mobility = rook(sq, all_pieces);
                    }
                    else if(p == Queen) {
                        mobility = bishop(sq, all_pieces) | rook(sq, all_pieces);
                    }
                    mobility &= ~pos.colour[0];
                    score += mobilities[p-1] * count(mobility);
                    TraceAdd(mobilities[p-1], count(mobility));

                    if (p == King && piece_bb & 0xC3D7) {
                        // C3D7 = Reasonable king squares
                        // Pawn cover is fixed in position, so it won't
                        // walk around with the king.
                        const u64 shield = file < 3 ? 0x700 : 0xE000;
                        score += count(shield & pawns[0]) * king_shield[0];
                        TraceAdd(king_shield[0], count(shield & pawns[0]));

                        score += count(north(shield) & pawns[0]) * king_shield[1];
                        TraceAdd(king_shield[1], count(north(shield) & pawns[0]));
                    }
                }
            }
        }

        flip(pos);

        score = -score;
    }

    // Tapered eval
    trace.score = ((short)score * phase + ((score + 0x8000) >> 16) * (24 - phase)) / 24;
    if (pos.flipped)
    {
        trace.score = -trace.score;
    }
    return trace;
}

#if TAPERED

static int32_t round_value(tune_t value)
{
    return static_cast<int32_t>(round(value));
}

static void print_parameter(std::stringstream& ss, const pair_t parameter)
{
    const auto mg = round_value(parameter[static_cast<int32_t>(PhaseStages::Midgame)]);
    const auto eg = round_value(parameter[static_cast<int32_t>(PhaseStages::Endgame)]);
    if (mg == 0 && eg == 0)
    {
        ss << 0;
    }
    else
    {
        ss << "S(" << mg << ", " << eg << ")";
    }
}
#else
static void print_parameter(std::stringstream& ss, const tune_t parameter)
{
    ss << round_value(std::round(parameter);
}
#endif

static void print_single(std::stringstream& ss, const parameters_t& parameters, int& index, const std::string& name)
{
    ss << "const i32 " << name << " = ";
    print_parameter(ss, parameters[index]);
    index++;

    ss << ";" << endl;
}

static void print_array(std::stringstream& ss, const parameters_t& parameters, int& index, const std::string& name, int count)
{
    ss << "const i32 " << name << "[] = {";
    for (auto i = 0; i < count; i++)
    {
        print_parameter(ss, parameters[index]);
        index++;

        if (i != count - 1)
        {
            ss << ", ";
        }
    }
    ss << "};" << endl;
}

static void print_array_2d(std::stringstream& ss, const parameters_t& parameters, int& index, const std::string& name, int count1, int count2)
{
    ss << "const i32 " << name << "[][" << count2 << "] = {\n";
    for (auto i = 0; i < count1; i++)
    {
        ss << "    {";
        for (auto j = 0; j < count2; j++)
        {
            print_parameter(ss, parameters[index]);
            index++;

            if (j != count2 - 1)
            {
                ss << ", ";
            }
        }
        ss << "},\n";
    }
    ss << "};\n";
}

static void print_max_material(std::stringstream& ss, const parameters_t& parameters)
{
    ss << "const i32 max_material[] = {";
    for (auto i = 0; i < 6; i++)
    {
        const auto mg = parameters[i][static_cast<int>(PhaseStages::Midgame)];
        const auto eg = parameters[i][static_cast<int>(PhaseStages::Endgame)];
        const auto max_material = round_value(max(mg, eg));
        ss << max_material << ", ";
    }
    ss << "0};" << endl;
}

static void rebalance_psts(parameters_t& parameters, const int32_t pst_offset, bool pawn_exclusion, const int32_t pst_size, const int32_t quantization)
{
    for (auto pieceIndex = 0; pieceIndex < 5; pieceIndex++)
    {
        const int pstStart = pst_offset + pieceIndex * pst_size;
        for (int stage = 0; stage < 2; stage++)
        {
            double sum = 0;
            for (auto i = 0; i < pst_size; i++)
            {
                if (pieceIndex == 0 && pawn_exclusion && (i == 0 || i == pst_size - 1 || i == pst_size - 2))
                {
                    continue;
                }
                const auto pstIndex = pstStart + i;
                sum += parameters[pstIndex][stage];
            }

            const auto average = sum / (pieceIndex == 0 && pawn_exclusion ? pst_size - 3 : pst_size);
            //const auto average = sum / pst_size;
            parameters[pieceIndex][stage] += average * quantization;
            for (auto i = 0; i < pst_size; i++)
            {
                if (pieceIndex == 0 && pawn_exclusion && (i == 0 || i == pst_size - 1 || i == pst_size - 2))
                {
                    continue;
                }
                const auto pstIndex = pstStart + i;
                parameters[pstIndex][stage] -= average;
            }
        }
    }
}

parameters_t FourkuEval::get_initial_parameters()
{
    parameters_t parameters;
    get_initial_parameter_array(parameters, material, 6);
    get_initial_parameter_array_2d(parameters, pst_rank, 6, 8);
    get_initial_parameter_array_2d(parameters, pst_file, 6, 8);
    get_initial_parameter_array_2d(parameters, open_files, 2, 3);
    get_initial_parameter_array(parameters, mobilities, 5);
    get_initial_parameter_array(parameters, pawn_protection, 6);
    get_initial_parameter_array(parameters, passers, 4);
    get_initial_parameter_single(parameters, pawn_passed_protected);
    get_initial_parameter_single(parameters, pawn_doubled);
    get_initial_parameter_single(parameters, pawn_phalanx);
    get_initial_parameter_array(parameters, pawn_passed_blocked, 4);
    get_initial_parameter_array(parameters, pawn_passed_king_distance, 2);
    get_initial_parameter_single(parameters, bishop_pair);
    get_initial_parameter_array(parameters, king_shield, 2);
    return parameters;
}

static coefficients_t get_coefficients(const Trace& trace)
{
    coefficients_t coefficients;
    get_coefficient_array(coefficients, trace.material, 6);
    get_coefficient_array_2d(coefficients, trace.pst_rank, 6, 8);
    get_coefficient_array_2d(coefficients, trace.pst_file, 6, 8);
    get_coefficient_array_2d(coefficients, trace.open_files, 2, 3);
    get_coefficient_array(coefficients, trace.mobilities, 5);
    get_coefficient_array(coefficients, trace.pawn_protection, 6);
    get_coefficient_array(coefficients, trace.passers, 4);
    get_coefficient_single(coefficients, trace.pawn_passed_protected);
    get_coefficient_single(coefficients, trace.pawn_doubled);
    get_coefficient_single(coefficients, trace.pawn_phalanx);
    get_coefficient_array(coefficients, trace.pawn_passed_blocked, 4);
    get_coefficient_array(coefficients, trace.pawn_passed_king_distance, 2);
    get_coefficient_single(coefficients, trace.bishop_pair);
    get_coefficient_array(coefficients, trace.king_shield, 2);
    return coefficients;
}

EvalResult FourkuEval::get_fen_eval_result(const string& fen)
{
    Position position;
    set_fen(position, fen);
    const auto trace = eval(position);
    EvalResult result;
    result.coefficients = get_coefficients(trace);
    result.score = trace.score;
    return result;
}

void FourkuEval::print_parameters(const parameters_t& parameters)
{
    parameters_t parameters_copy = parameters;
    rebalance_psts(parameters_copy, 6, true, 8, 4);
    rebalance_psts(parameters_copy, 6 + 6 * 8, false, 8, 4);

    int index = 0;
    stringstream ss;
    print_max_material(ss, parameters_copy);
    print_array(ss, parameters_copy, index, "material", 6);
    print_array_2d(ss, parameters_copy, index, "pst_rank", 6, 8);
    print_array_2d(ss, parameters_copy, index, "pst_file", 6, 8);
    print_array_2d(ss, parameters_copy, index, "open_files", 2, 3);
    print_array(ss, parameters_copy, index, "mobilities", 5);
    print_array(ss, parameters_copy, index, "pawn_protection", 6);
    print_array(ss, parameters_copy, index, "passers", 4);
    print_single(ss, parameters_copy, index, "pawn_passed_protected");
    print_single(ss, parameters_copy, index, "pawn_doubled");
    print_single(ss, parameters_copy, index, "pawn_phalanx");
    print_array(ss, parameters_copy, index, "pawn_passed_blocked", 4);
    print_array(ss, parameters_copy, index, "pawn_passed_king_distance", 2);
    print_single(ss, parameters_copy, index, "bishop_pair");
    print_array(ss, parameters_copy, index, "king_shield", 2);
    cout << ss.str() << "\n";
}