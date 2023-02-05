// THIS FILE IS LICENSED MIT

#include "fourku.h"
#include "../base.h"

#include <array>
#include <bit>
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
    int psts[6][4][2]{};
    int centralities[6][2]{};
    int outside_files[6][2]{};
    int pawn_protection[6][2]{};
    int passers[6][2]{};
    int pawn_doubled[2]{};
    int pawn_passed_blocked[6][2]{};
    int pawn_passed_king_distance[2][2]{};
    int pawn_passed_protected[2]{};
    int bishop_pair[2]{};
    int rook_open[2]{};
    int rook_semi_open[2]{};
    int rook_rank78[2]{};
    int king_shield[3][2]{};
};

const int phases[] = { 0, 1, 1, 2, 4, 0 };
const int max_material[] = { 162, 381, 411, 725, 1288, 0, 0 };
const int material[] = { S(79, 162), S(376, 381), S(363, 411), S(502, 725), S(1090, 1288), 0 };
const int psts[][4] = {
    {S(-23, -2), S(-10, 10), S(-0, 6), S(5, 22)},
    {S(-14, -3), S(-4, 1), S(15, 6), S(22, 13)},
    {S(5, 0), S(1, 1), S(9, 9), S(15, 15)},
    {S(-19, 7), S(-8, 1), S(4, 23), S(42, 11)},
    {S(57, 17), S(60, 22), S(31, 84), S(85, 120)},
    {S(-30, 0), S(-18, -1), S(31, -4), S(3, 9)},
};
const int centralities[] = { S(13, -19), S(15, 16), S(21, 9), S(-6, 1), S(0, 17), S(-7, 14) };
const int outside_files[] = { S(5, -14), S(-1, -6), S(8, -3), S(-1, -2), S(-1, -3), S(8, 0) };
const int pawn_protection[] = { S(10, 18), S(6, 25), S(-7, 17), S(-1, 13), S(-7, 25), S(-55, 28) };
const int passers[] = { S(-6, 9), S(-12, -3), S(-11, 22), S(-1, 54), S(35, 148), S(115, 230) };
const int pawn_doubled = S(-25, -26);
const int pawn_passed_blocked[] = { S(0, -38), S(-19, -9), S(-8, -24), S(3, -32), S(6, -73), S(52, -115) };
const int pawn_passed_king_distance[] = { S(3, -7), S(-5, 10) };
const int bishop_pair = S(26, 67);
const int rook_open = S(55, 10);
const int rook_semi_open = S(25, 16);
const int rook_rank78 = S(20, 11);
const int king_shield[] = { S(22, -6), S(11, -8), S(-93, 25) };
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

        // Bishop pair
        if (count(pos.colour[0] & pos.pieces[Bishop]) == 2) {
            score += bishop_pair;
            TraceIncr(bishop_pair);
        }

        // For each piece type
        for (int p = 0; p < 6; ++p) {
            auto copy = pos.colour[0] & pos.pieces[p];
            while (copy) {
                phase += phases[p];

                const int sq = lsb(copy);
                copy &= copy - 1;
                const int rank = sq / 8;
                const int file = sq % 8;
                const int centrality = (7 - abs(7 - rank - file) - abs(rank - file)) / 2;

                // Material
                score += material[p];
                TraceIncr(material[p]);

                // Centrality
                score += centrality * centralities[p];
                TraceAdd(centralities[p], centrality);

                // Closeness to outside files
                score += abs(file - 3) * outside_files[p];
                TraceAdd(outside_files[p], abs(file - 3));

                // Quadrant PSTs
                score += psts[p][(rank / 4) * 2 + file / 4];
                TraceIncr(psts[p][(rank / 4) * 2 + file / 4]);

                // Pawn protection
                const u64 piece_bb = 1ULL << sq;
                if (piece_bb & protected_by_pawns) {
                    score += pawn_protection[p];
                    TraceIncr(pawn_protection[p]);
                }

                if (~pawns[0] & piece_bb & attacked_by_pawns) {
                    // If we're to move, we'll just lose some options and our tempo.
                    // If we're not to move, we lose a piece?
                    score += pawn_attacked[c];
                }

                if (p == Pawn) {
                    // Passed pawns
                    u64 blockers = 0x101010101010101ULL << sq;
                    blockers |= nw(blockers) | ne(blockers);
                    if (!(blockers & pawns[1])) {
                        score += passers[rank - 1];
                        TraceIncr(passers[rank - 1]);

                        // Blocked passed pawns
                        if (north(piece_bb) & pos.colour[1]) {
                            score += pawn_passed_blocked[rank - 1];
                            TraceIncr(pawn_passed_blocked[rank - 1]);
                        }

                        // King defense/attack
                        // king distance to square in front of passer
                        for (int i = 0; i < 2; ++i) {
                            score += pawn_passed_king_distance[i] * (rank - 1) * max(abs((kings[i] / 8) - (rank + 1)), abs((kings[i] % 8) - file));
                            TraceAdd(pawn_passed_king_distance[i], (rank - 1) * max(abs((kings[i] / 8) - (rank + 1)), abs((kings[i] % 8) - file)));
                        }
                    }

                    // Doubled pawns
                    if ((north(piece_bb) | north(north(piece_bb))) & pawns[0]) {
                        score += pawn_doubled;
                        TraceIncr(pawn_doubled);
                    }
                }
                else if (p == Rook) {
                    // Rook on open or semi-open files
                    const u64 file_bb = 0x101010101010101ULL << file;
                    if (!(file_bb & pawns[0])) {
                        if (!(file_bb & pawns[1])) {
                            score += rook_open;
                            TraceIncr(rook_open);
                        }
                        else {
                            score += rook_semi_open;
                            TraceIncr(rook_semi_open);
                        }
                    }

                    // Rook on 7th or 8th rank
                    if (rank >= 6) {
                        score += rook_rank78;
                        TraceIncr(rook_rank78);
                    }
                }
                else if (p == King && piece_bb & 0xE7) {
                    const u64 shield = file < 3 ? 0x700 : 0xE000;
                    score += count(shield & pawns[0]) * king_shield[0];
                    TraceAdd(king_shield[0], count(shield & pawns[0]));

                    score += count(north(shield) & pawns[0]) * king_shield[1];
                    TraceAdd(king_shield[1], count(north(shield) & pawns[0]));

                    // C3D7 = Reasonable king squares
                    score += !(piece_bb & 0xC3D7) * king_shield[2];
                    TraceAdd(king_shield[2], !(piece_bb & 0xC3D7));
                }
            }
        }

        flip(pos);

        score = -score;
    }

    // Tapered eval
    trace.score = ((short)score * phase + ((score + 0x8000) >> 16) * (24 - phase)) / 24;
    if(pos.flipped)
    {
        trace.score = -trace.score;
    }
    return trace;
}

parameters_t FourkuEval::get_initial_parameters()
{
    parameters_t parameters;
    get_initial_parameter_array(parameters, material, 6);
    get_initial_parameter_array_2d(parameters, psts, 6, 4);
    get_initial_parameter_array(parameters, centralities, 6);
    get_initial_parameter_array(parameters, outside_files, 6);
    get_initial_parameter_array(parameters, pawn_protection, 6);
    get_initial_parameter_array(parameters, passers, 6);
    get_initial_parameter_single(parameters, pawn_doubled);
    get_initial_parameter_array(parameters, pawn_passed_blocked, 6);
    get_initial_parameter_array(parameters, pawn_passed_king_distance, 2);
    get_initial_parameter_single(parameters, bishop_pair);
    get_initial_parameter_single(parameters, rook_open);
    get_initial_parameter_single(parameters, rook_semi_open);
    get_initial_parameter_single(parameters, rook_rank78);
    get_initial_parameter_array(parameters, king_shield, 3);
    return parameters;
}

static coefficients_t get_coefficients(const Trace& trace)
{
    coefficients_t coefficients;
    get_coefficient_array(coefficients, trace.material, 6);
    get_coefficient_array_2d(coefficients, trace.psts, 6, 4);
    get_coefficient_array(coefficients, trace.centralities, 6);
    get_coefficient_array(coefficients, trace.outside_files, 6);
    get_coefficient_array(coefficients, trace.pawn_protection, 6);
    get_coefficient_array(coefficients, trace.passers, 6);
    get_coefficient_single(coefficients, trace.pawn_doubled);
    get_coefficient_array(coefficients, trace.pawn_passed_blocked, 6);
    get_coefficient_array(coefficients, trace.pawn_passed_king_distance, 2);
    get_coefficient_single(coefficients, trace.bishop_pair);
    get_coefficient_single(coefficients, trace.rook_open);
    get_coefficient_single(coefficients, trace.rook_semi_open);
    get_coefficient_single(coefficients, trace.rook_rank78);
    get_coefficient_array(coefficients, trace.king_shield, 3);
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

#if TAPERED
static void print_parameter(std::stringstream& ss, const pair_t parameter)
{
    const auto mg = static_cast<int32_t>(std::round(parameter[static_cast<int32_t>(PhaseStages::Midgame)]));
    const auto eg = static_cast<int32_t>(std::round(parameter[static_cast<int32_t>(PhaseStages::Endgame)]));
    if(mg == 0 && eg == 0)
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
    ss << static_cast<int32_t>(std::round(parameter));
}
#endif

static void print_single(std::stringstream& ss, const parameters_t& parameters, int& index, const std::string& name)
{
    ss << "const int " << name << " = ";
    print_parameter(ss, parameters[index]);
    index++;

    ss << ";" << endl;
}

static void print_array(std::stringstream& ss, const parameters_t& parameters, int& index, const std::string& name, int count)
{
    ss << "const int " << name << "[] = {";
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
    ss << "const int " << name << "[][" << count2 << "] = {\n";
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

            if (j % 8 == 7)
            {
                ss << "\n";
            }
        }
        ss << " },\n";
    }
    ss << "};\n";
}

void FourkuEval::print_parameters(const parameters_t& parameters)
{
    int index = 0;
    stringstream ss;
    print_array(ss, parameters, index, "material", 6);
    print_array_2d(ss, parameters, index, "psts", 6, 4);
    print_array(ss, parameters, index, "centralities", 6);
    print_array(ss, parameters, index, "outside_files", 6);
    print_array(ss, parameters, index, "pawn_protection", 6);
    print_array(ss, parameters, index, "passers", 6);
    print_single(ss, parameters, index, "pawn_doubled");
    print_array(ss, parameters, index, "pawn_passed_blocked", 6);
    print_array(ss, parameters, index, "pawn_passed_king_distance", 2);
    print_single(ss, parameters, index, "bishop_pair");
    print_single(ss, parameters, index, "rook_open");
    print_single(ss, parameters, index, "rook_semi_open");
    print_single(ss, parameters, index, "rook_rank78");
    print_array(ss, parameters, index, "king_shield", 3);
    cout << ss.str() << "\n";
}
    