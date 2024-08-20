// THIS FILE IS LICENSED MIT

#include "fourkdotcpp.h"

#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <sstream>

using namespace std;
using namespace Fourkdotcpp;

using u64 = uint64_t;
using i32 = int;

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

static std::string pc_to_str[] = {"Pawn", "Knight", "Bishop", "Rook", "Queen", "King", "None"};

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

    auto operator<=>(const Position&) const = default;
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

[[nodiscard]] static auto bishop(const int sq, const u64 blockers) {
    return ray(sq, blockers, nw) | ray(sq, blockers, ne) | ray(sq, blockers, sw) | ray(sq, blockers, se);
}

[[nodiscard]] static auto rook(const int sq, const u64 blockers) {
    return ray(sq, blockers, north) | ray(sq, blockers, east) | ray(sq, blockers, south) | ray(sq, blockers, west);
}

[[nodiscard]] static u64 king(const int sq, const u64) {
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
    tune_t endgame_scale;

    int material[6][2]{};
    int pst_rank[48][2]{};
    int pst_file[48][2]{};
};

const i32 phases[] = {0, 1, 1, 2, 4, 0};
const i32 max_material[] = {147, 521, 521, 956, 1782, 0, 0};
const i32 material[] = {S(89, 147), S(350, 521), S(361, 521), S(479, 956), S(1046, 1782), 0};
const i32 pst_rank[] = {
    0,         S(-3, 0),  S(-3, -1), S(-1, -1), S(1, 0),  S(5, 3),  0,        0,          // Pawn
    S(-2, -5), S(0, -3),  S(1, -1),  S(3, 3),   S(4, 4),  S(5, 1),  S(2, 0),  S(-15, 1),  // Knight
    S(0, -2),  S(2, -1),  S(2, 0),   S(2, 0),   S(2, 0),  S(2, 0),  S(-1, 0), S(-10, 2),  // Bishop
    S(0, -3),  S(-1, -3), S(-2, -2), S(-2, 0),  S(0, 2),  S(2, 2),  S(1, 3),  S(2, 1),    // Rook
    S(2, -11), S(3, -8),  S(2, -3),  S(0, 2),   S(0, 5),  S(-1, 5), S(-4, 7), S(-2, 4),   // Queen
    S(-1, -6), S(1, -2),  S(-1, 0),  S(-4, 3),  S(-1, 5), S(5, 4),  S(5, 2),  S(5, -6),   // King
};
const i32 pst_file[] = {
    S(-1, 1),  S(-2, 1),  S(-1, 0), S(0, -1), S(1, 0),  S(2, 0),  S(2, 0),  S(-1, 0),   // Pawn
    S(-4, -3), S(-1, -1), S(0, 1),  S(2, 3),  S(2, 3),  S(2, 0),  S(1, -1), S(-1, -3),  // Knight
    S(-2, -1), 0,         S(1, 0),  S(0, 1),  S(1, 1),  S(0, 1),  S(2, 0),  S(-1, -1),  // Bishop
    S(-2, 0),  S(-1, 1),  S(0, 1),  S(1, 0),  S(2, -1), S(1, 0),  S(1, 0),  S(-1, -1),  // Rook
    S(-2, -3), S(-1, -1), S(-1, 0), S(0, 1),  S(0, 2),  S(1, 2),  S(2, 0),  S(1, -1),   // Queen
    S(-2, -5), S(2, -1),  S(-1, 1), S(-4, 2), S(-4, 2), S(-2, 2), S(2, -1), S(0, -5),   // King
};

#define TraceIncr(parameter) trace.parameter[color]++
#define TraceAdd(parameter, count) trace.parameter[color] += count

static Trace eval(Position& pos) {
    Trace trace{};
    int score = S(29, 10);
    int phase = 0;

    for (int c = 0; c < 2; ++c) {
        const int color = pos.flipped;

        // our pawns, their pawns
        const u64 pawns[] = { pos.colour[0] & pos.pieces[Pawn], pos.colour[1] & pos.pieces[Pawn] };
        const u64 protected_by_pawns = nw(pawns[0]) | ne(pawns[0]);
        const u64 attacked_by_pawns = se(pawns[1]) | sw(pawns[1]);
        const int kings[] = { lsb(pos.colour[0] & pos.pieces[King]), lsb(pos.colour[1] & pos.pieces[King]) };
        const u64 all_pieces = pos.colour[0] | pos.colour[1];

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
                    score += pst_rank[p * 8 + rank] * 1;
                    TraceAdd(pst_rank[p * 8 + rank], 1);
                }
                score += pst_file[p * 8 + file] * 1;
                TraceAdd(pst_file[p * 8 + file], 1);
            }
        }

        flip(pos);

        score = -score;
    }

#if TAPERED
    // Tapered eval with endgame scaling based on remaining pawn count of the stronger side
    int stronger_colour = score < 0;
    auto stronger_colour_pieces = pos.colour[stronger_colour];
    auto stronger_colour_pawns = stronger_colour_pieces & pos.pieces[Pawn];
    auto stronger_colour_pawn_count = count(stronger_colour_pawns);
    auto stronger_colour_pawns_missing = 8 - stronger_colour_pawn_count;
    auto scale = (128 - stronger_colour_pawns_missing * stronger_colour_pawns_missing) / static_cast<tune_t>(128);
        
    trace.endgame_scale = scale;
    trace.score = ((short)score * phase + ((score + 0x8000) >> 16) * scale * (24 - phase)) / 24;
#else
    trace.endgame_scale = 1;
    trace.score = score;
#endif

    if (pos.flipped)
    {
        trace.score = -trace.score;
    }
    return trace;
}

static int32_t round_value(tune_t value)
{
    return static_cast<int32_t>(round(value));
}

#if TAPERED

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
    ss << round_value(std::round(parameter));
}
#endif

static void print_single(std::stringstream& ss, const parameters_t& parameters, int& index, const std::string& name)
{
    ss << "const i32 " << name << " = ";
    print_parameter(ss, parameters[index]);
    index++;

    ss << ";" << endl;
}

static void print_array(std::stringstream& ss, const parameters_t& parameters, int& index, const std::string& name, const int count)
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

static void print_pst(std::stringstream& ss, const parameters_t& parameters, int& index, const std::string& name)
{
    ss << "const i32 " << name << "[] = {";
    for (auto i = 0; i < 48; i++)
    {
        print_parameter(ss, parameters[index]);
        index++;

        ss << ", ";

        if (i % 8 == 7)
        {
            ss << "// " << pc_to_str[i / 8] << "\n";
        }
    }
    ss << "};" << endl;
}

static void print_array_2d(std::stringstream& ss, const parameters_t& parameters, int& index, const std::string& name, const int count1, const int count2)
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
#if TAPERED
        const auto mg = parameters[i][static_cast<int>(PhaseStages::Midgame)];
        const auto eg = parameters[i][static_cast<int>(PhaseStages::Endgame)];
        const auto max_material = round_value(max(mg, eg));
        ss << max_material << ", ";
#else
        const auto score = round_value(parameters[i]);
        ss << score << ", ";
#endif
    }
    ss << "0};" << endl;
}

static void rebalance_psts(parameters_t& parameters, const int32_t pst_offset, bool pawn_exclusion, const int32_t pst_size, const int32_t quantization)
{
    for (auto pieceIndex = 0; pieceIndex < 5; pieceIndex++)
    {
        const int pstStart = pst_offset + pieceIndex * pst_size;
#if TAPERED
        for (int stage = 0; stage < 2; stage++)
        {
#endif
            double sum = 0;
            for (auto i = 0; i < pst_size; i++)
            {
                if (pieceIndex == 0 && pawn_exclusion && (i == 0 || i == pst_size - 1 || i == pst_size - 2))
                {
                    continue;
                }
                const auto pstIndex = pstStart + i;
#if TAPERED
                sum += parameters[pstIndex][stage];
#else
                sum += parameters[pstIndex];
#endif
            }

            const auto average = sum / (pieceIndex == 0 && pawn_exclusion ? pst_size - 3 : pst_size);
            //const auto average = sum / pst_size;
#if TAPERED
            parameters[pieceIndex][stage] += average * quantization;
#else
            parameters[pieceIndex] += average * quantization;
#endif
            for (auto i = 0; i < pst_size; i++)
            {
                if (pieceIndex == 0 && pawn_exclusion && (i == 0 || i == pst_size - 1 || i == pst_size - 2))
                {
                    continue;
                }
                const auto pstIndex = pstStart + i;
#if TAPERED
                parameters[pstIndex][stage] -= average;
#else
                parameters[pstIndex] -= average;
#endif
            }
#if TAPERED
        }
#endif
    }
}

parameters_t FourkdotcppEval::get_initial_parameters()
{
    parameters_t parameters;
    get_initial_parameter_array(parameters, material, 6);
    get_initial_parameter_array(parameters, pst_rank, 48);
    get_initial_parameter_array(parameters, pst_file, 48);
    return parameters;
}

static coefficients_t get_coefficients(const Trace& trace)
{
    coefficients_t coefficients;
    get_coefficient_array(coefficients, trace.material, 6);
    get_coefficient_array(coefficients, trace.pst_rank, 48);
    get_coefficient_array(coefficients, trace.pst_file, 48);
    return coefficients;
}

void FourkdotcppEval::print_parameters(const parameters_t& parameters)
{
    parameters_t parameters_copy = parameters;
    rebalance_psts(parameters_copy, 6, true, 8, 1);
    rebalance_psts(parameters_copy, 6 + 6 * 8, false, 8, 1);

    int index = 0;
    stringstream ss;
    print_max_material(ss, parameters_copy);
    print_array(ss, parameters_copy, index, "material", 6);
    print_pst(ss, parameters_copy, index, "pst_rank");
    print_pst(ss, parameters_copy, index, "pst_file");
    cout << ss.str() << "\n";
}

static Position get_position_from_external(const chess::Board& board)
{
    Position position;

    position.flipped = false;

    position.colour[0] = board.us(chess::Color::WHITE).getBits();
    position.colour[1] = board.them(chess::Color::WHITE).getBits();

    position.pieces[Pawn] = board.pieces(chess::PieceType::PAWN, chess::Color::WHITE).getBits() | board.pieces(chess::PieceType::PAWN, chess::Color::BLACK).getBits();
    position.pieces[Knight] = board.pieces(chess::PieceType::KNIGHT, chess::Color::WHITE).getBits() | board.pieces(chess::PieceType::KNIGHT, chess::Color::BLACK).getBits();
    position.pieces[Bishop] = board.pieces(chess::PieceType::BISHOP, chess::Color::WHITE).getBits() | board.pieces(chess::PieceType::BISHOP, chess::Color::BLACK).getBits();
    position.pieces[Rook] = board.pieces(chess::PieceType::ROOK, chess::Color::WHITE).getBits() | board.pieces(chess::PieceType::ROOK, chess::Color::BLACK).getBits();
    position.pieces[Queen] = board.pieces(chess::PieceType::QUEEN, chess::Color::WHITE).getBits() | board.pieces(chess::PieceType::QUEEN, chess::Color::BLACK).getBits();
    position.pieces[King] = board.pieces(chess::PieceType::KING, chess::Color::WHITE).getBits() | board.pieces(chess::PieceType::KING, chess::Color::BLACK).getBits();

    position.castling[0] = board.castlingRights().has(chess::Color::WHITE, chess::Board::CastlingRights::Side::KING_SIDE);
    position.castling[1] = board.castlingRights().has(chess::Color::WHITE, chess::Board::CastlingRights::Side::QUEEN_SIDE);
    position.castling[2] = board.castlingRights().has(chess::Color::BLACK, chess::Board::CastlingRights::Side::KING_SIDE);
    position.castling[3] = board.castlingRights().has(chess::Color::BLACK, chess::Board::CastlingRights::Side::QUEEN_SIDE);

    position.ep = board.enpassantSq().index();
    if(position.ep == 64)
    {
        position.ep = 0;
    }
    if(position.ep != 0)
    {
        position.ep = 1ULL << position.ep;
    }

    if (board.sideToMove() == chess::Color::BLACK)
    {
        flip(position);
    }

    //Position position2;
    //set_fen(position2, board.getFen());
    //if(position != position2)
    //{
    //    throw std::runtime_error("Position mismatch");
    //}

    return position;
}

EvalResult FourkdotcppEval::get_fen_eval_result(const string& fen)
{
    Position position;
    set_fen(position, fen);
    const auto trace = eval(position);
    EvalResult result;
    result.coefficients = get_coefficients(trace);
    result.score = trace.score;
    result.endgame_scale = trace.endgame_scale;
    return result;
}

EvalResult FourkdotcppEval::get_external_eval_result(const chess::Board& board)
{
    auto position = get_position_from_external(board);
    const auto trace = eval(position);
    EvalResult result;
    result.coefficients = get_coefficients(trace);
    result.score = trace.score;
    result.endgame_scale = trace.endgame_scale;

    return result;
}