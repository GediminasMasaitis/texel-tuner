#include "chal.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;
using namespace Chal;

// Piece types matching chal.c (1-based: PAWN=1..KING=6)
enum { EMPTY = 0, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum { WHITE = 0, BLACK = 1 };

static const string pc_to_str[] = { "empty", "pawn", "knight", "bishop", "rook", "queen", "king" };

// Phase contribution per piece type (indexed 0..6, 1-based piece types)
static const int phase_inc[7] = { 0, 0, 1, 1, 2, 4, 0 };

// --- Initial parameter values from chal.c ---

static int mg_pst[6][64] = {
    { 82, 82, 82, 82, 82, 82, 82, 82,  47, 76, 57, 60, 67,100,107, 56,  56, 71, 78, 74, 87, 87,104, 70,  53, 77, 78, 96, 99, 88, 90, 55,  66, 94, 90,105,107, 96,100, 57,  76, 90,101,105,120,141,108, 63, 162,158,131,136,132,139,108, 79,  82, 82, 82, 82, 82, 82, 82, 82},
    {231,318,281,306,322,311,317,315, 310,286,327,336,338,357,325,320, 312,330,347,349,358,356,364,319, 325,343,353,349,367,357,360,330, 330,355,355,392,372,407,354,360, 290,398,374,400,422,465,411,379, 266,297,411,374,360,401,342,322, 172,250,305,290,400,241,322,232},
    {333,364,353,346,354,351,326,344, 370,384,382,365,374,388,401,367, 363,382,380,378,377,393,384,373, 361,380,376,392,398,375,374,370, 362,368,384,417,400,400,370,362, 347,403,408,403,400,417,402,361, 339,382,348,353,397,425,385,318, 338,370,283,329,342,325,373,358},
    {460,466,479,492,491,486,438,452, 434,462,459,467,476,490,473,405, 433,454,461,460,478,479,474,443, 439,453,464,474,486,471,483,452, 455,467,482,502,499,512,469,457, 471,496,501,511,492,523,538,493, 502,507,533,537,555,542,501,519, 510,519,508,526,539,488,510,522},
    {1023,1008,1018,1037,1012,1002,996,976, 991,1016,1036,1029,1035,1042,1024,1028, 1009,1025,1012,1021,1018,1025,1038,1030, 1014,997,1014,1013,1021,1019,1026,1020, 996,996,1007,1007,1022,1040,1022,1024, 1014,1006,1030,1031,1054,1083,1072,1082, 1002,984,1020,1028,1008,1084,1054,1081, 999,1026,1056,1038,1086,1071,1070,1072},
    { -17, 36, 14,-56,  6,-26, 26, 12,   1,  8, -6,-66,-45,-14, 11,  7, -13,-12,-20,-48,-46,-28,-13,-25, -48,  1,-25,-41,-48,-42,-32,-53, -16,-18,-10,-29,-31,-25,-13,-35,  -7, 26,  4,-17,-22,  8, 24,-24,  30,  1,-18, -5,-10, -2,-36,-28, -66, 24, 18,-14,-58,-32,  3, 13}
};

static int eg_pst[6][64] = {
    { 94, 94, 94, 94, 94, 94, 94, 94, 108,100,102,102,106, 92, 94, 85,  96, 99, 86, 94, 94, 89, 91, 84, 105,101, 89, 85, 85, 84, 95, 91, 124,116,105, 97, 90, 96,109,109, 166,163,140,119,118,125,146,156, 189,186,180,156,159,182,187,218,  94, 94, 94, 94, 94, 94, 94, 94},
    {254,232,260,268,261,265,233,219, 241,263,273,278,281,263,260,239, 260,279,280,297,293,279,263,261, 265,277,299,308,298,298,287,265, 266,286,305,305,305,293,291,261, 259,263,290,291,278,270,262,239, 258,275,257,281,272,254,255,231, 225,245,270,255,251,256,220,183},
    {276,290,276,294,290,283,294,282, 285,281,292,298,302,290,284,271, 287,296,307,307,312,299,292,284, 293,301,311,317,304,306,295,290, 296,308,310,306,311,305,301,301, 301,290,297,297,297,303,299,303, 291,295,306,287,295,286,293,285, 285,278,288,291,292,290,282,275},
    {506,517,518,512,510,502,519,495, 509,509,515,517,506,506,504,512, 511,515,510,514,508,503,507,499, 518,520,523,516,509,509,507,504, 519,518,528,513,513,516,513,517, 522,522,520,517,517,512,510,512, 522,524,524,522,508,514,519,514, 528,524,533,526,525,527,523,520},
    {904,910,916,896,934,906,919,897, 917,916,909,922,922,916,903,906, 923,911,952,944,947,955,949,944, 920,967,956,985,967,973,976,962, 941,960,960,983,996,977,996,975, 918,943,946,987,986,974,956,948, 921,959,970,980,997,964,969,938, 930,961,960,966,966,958,949,959},
    {-55,-36,-19,-12,-30,-12,-26,-45, -28, -9,  6, 11, 12,  6, -3,-19, -21, -1, 13, 19, 21, 18,  9,-10, -19, -3, 23, 22, 25, 25, 11,-12, -10, 24, 26, 25, 24, 35, 28,  1,  10, 18, 24, 13, 18, 46, 45, 11, -12, 19, 16, 16, 15, 40, 25, 12, -74,-34,-18,-20,-13, 17,  5,-19}
};

// Mobility
static const int mob_center[7] = { 0, 0, 4, 6, 6, 13, 0 };
static const int mob_step_mg[7] = { 0, 0, 3, 4, 3, 2, 0 };
static const int mob_step_eg[7] = { 0, 0, 3, 4, 4, 2, 0 };

// Bishop pair
static const int bishop_pair_mg = 31;
static const int bishop_pair_eg = 30;

// King pawn shield
static const int shield_val[8] = { 0, 12, 4, -2, -2, 0, 0, -12 };
static const int shield_open_penalty = -18;

// Rook on open/semi-open files
static const int rook_open_file_mg = 20;
static const int rook_open_file_eg = 20;
static const int rook_semi_open_file_mg = 10;
static const int rook_semi_open_file_eg = 10;

// Pawn structure
static const int doubled_pawn_mg = -20;
static const int doubled_pawn_eg = -20;
static const int isolated_pawn_mg = -10;
static const int isolated_pawn_eg = -10;

// Passed pawns (indexed by own_rank 0..7)
static const int pp_mg[8] = { 0,  5, 10, 20, 35,  55,  80, 0 };
static const int pp_eg[8] = { 0, 20, 30, 55, 80, 115, 170, 0 };

// Blocked passed pawns get half the bonus -- linearized as separate params
// initialized to half of pp values
static const int pp_blocked_mg[8] = { 0,  2,  5, 10, 17,  27,  40, 0 };
static const int pp_blocked_eg[8] = { 0, 10, 15, 27, 40,  57,  85, 0 };

// Passed pawn king distance factor (EG only)
static const int pp_king_distance_eg = 4;

// --- Position representation for the tuner ---

struct Position {
    int piece[64];   // piece type (EMPTY/PAWN..KING) at each square
    int color[64];   // WHITE/BLACK (-1 for empty)
    bool white_to_move;
};

static void set_fen(Position& pos, const string& fen) {
    for (int i = 0; i < 64; i++) { pos.piece[i] = EMPTY; pos.color[i] = -1; }

    stringstream ss{fen};
    string word;
    ss >> word;

    int sq = 56; // start at a8 = rank 7
    for (const auto c : word) {
        if (c >= '1' && c <= '8') {
            sq += c - '0';
        } else if (c == '/') {
            sq -= 16;
        } else {
            int side = (c >= 'a' && c <= 'z') ? BLACK : WHITE;
            char lower = (c >= 'A' && c <= 'Z') ? c + 32 : c;
            int pt = (lower == 'p') ? PAWN
                   : (lower == 'n') ? KNIGHT
                   : (lower == 'b') ? BISHOP
                   : (lower == 'r') ? ROOK
                   : (lower == 'q') ? QUEEN
                   : KING;
            pos.piece[sq] = pt;
            pos.color[sq] = side;
            sq++;
        }
    }

    ss >> word;
    pos.white_to_move = (word == "w");
}

static Position get_position_from_board(const chess::Board& board) {
    Position pos;
    for (int i = 0; i < 64; i++) { pos.piece[i] = EMPTY; pos.color[i] = -1; }

    for (int sq = 0; sq < 64; sq++) {
        auto piece = board.at(chess::Square(sq));
        if (piece == chess::Piece::NONE) continue;

        int pt;
        auto ptype = piece.type();
        if (ptype == chess::PieceType::PAWN) pt = PAWN;
        else if (ptype == chess::PieceType::KNIGHT) pt = KNIGHT;
        else if (ptype == chess::PieceType::BISHOP) pt = BISHOP;
        else if (ptype == chess::PieceType::ROOK) pt = ROOK;
        else if (ptype == chess::PieceType::QUEEN) pt = QUEEN;
        else pt = KING;

        int side = (piece.color() == chess::Color::WHITE) ? WHITE : BLACK;
        pos.piece[sq] = pt;
        pos.color[sq] = side;
    }

    pos.white_to_move = (board.sideToMove() == chess::Color::WHITE);
    return pos;
}

// --- Mobility calculation using bitboards ---

using u64 = uint64_t;

static u64 knight_attacks(int sq) {
    const u64 bb = 1ULL << sq;
    return (((bb << 15) | (bb >> 17)) & 0x7F7F7F7F7F7F7F7FULL) |
           (((bb << 17) | (bb >> 15)) & 0xFEFEFEFEFEFEFEFEULL) |
           (((bb << 10) | (bb >> 6)) & 0xFCFCFCFCFCFCFCFCULL) |
           (((bb << 6) | (bb >> 10)) & 0x3F3F3F3F3F3F3F3FULL);
}

template <typename F>
static u64 ray(int sq, u64 blockers, F f) {
    u64 mask = f(1ULL << sq);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    mask |= f(mask & ~blockers);
    return mask;
}

static u64 north(u64 bb) { return bb << 8; }
static u64 south(u64 bb) { return bb >> 8; }
static u64 east(u64 bb)  { return (bb << 1) & ~0x0101010101010101ULL; }
static u64 west(u64 bb)  { return (bb >> 1) & ~0x8080808080808080ULL; }
static u64 nw(u64 bb)    { return north(west(bb)); }
static u64 ne(u64 bb)    { return north(east(bb)); }
static u64 sw(u64 bb)    { return south(west(bb)); }
static u64 se(u64 bb)    { return south(east(bb)); }

static u64 bishop_attacks(int sq, u64 blockers) {
    return ray(sq, blockers, nw) | ray(sq, blockers, ne) | ray(sq, blockers, sw) | ray(sq, blockers, se);
}

static u64 rook_attacks(int sq, u64 blockers) {
    return ray(sq, blockers, north) | ray(sq, blockers, east) | ray(sq, blockers, south) | ray(sq, blockers, west);
}

static int chebyshev_distance(int sq1, int sq2) {
    int f1 = sq1 & 7, r1 = sq1 >> 3;
    int f2 = sq2 & 7, r2 = sq2 >> 3;
    return max(abs(f1 - f2), abs(r1 - r2));
}

// --- Trace struct ---

struct Trace {
    int score;
    tune_t endgame_scale;

    // PST: [piece-1][sq][color]
    int pst[6][64][2]{};

    // Mobility: [piece_type - KNIGHT][color]  (Knight=0, Bishop=1, Rook=2, Queen=3)
    int mobility[4][2]{};

    // Bishop pair
    int bishop_pair[2]{};

    // King shield: [rank_index][color]
    int king_shield[8][2]{};
    int king_shield_open[2]{};

    // Rook files
    int rook_open_file[2]{};
    int rook_semi_open_file[2]{};

    // Pawn structure
    int doubled_pawn[2]{};
    int isolated_pawn[2]{};

    // Passed pawns: [rank][color]
    int passed_pawn[8][2]{};
    int passed_pawn_blocked[8][2]{};
    int passed_pawn_king_dist[2]{};
};

#define TraceIncr(param) trace.param[color]++
#define TraceAdd(param, cnt) trace.param[color] += (cnt)

static Trace eval(const Position& pos) {
    Trace trace{};
    int mg[2] = {}, eg[2] = {};
    int phase = 0;
    int lowest_pawn_rank[2][8];
    int bishop_count[2] = {};
    int king_sq[2] = {};

    for (int i = 0; i < 8; i++) lowest_pawn_rank[WHITE][i] = lowest_pawn_rank[BLACK][i] = 7;

    // Build occupancy bitboard and find kings
    u64 all_pieces = 0;
    u64 own_pieces[2] = {};
    for (int sq = 0; sq < 64; sq++) {
        if (pos.piece[sq] != EMPTY) {
            all_pieces |= 1ULL << sq;
            own_pieces[pos.color[sq]] |= 1ULL << sq;
            if (pos.piece[sq] == KING) king_sq[pos.color[sq]] = sq;
            if (pos.piece[sq] == BISHOP) bishop_count[pos.color[sq]]++;
        }
    }

    // Track pawns and rooks for second pass
    int pr_list[32], pr_index = 0;

    // First pass: material+PST, phase, mobility, record pawns/rooks
    for (int sq = 0; sq < 64; sq++) {
        int pt = pos.piece[sq];
        if (pt == EMPTY) continue;
        int color = pos.color[sq];
        int rank = sq >> 3;
        int f = sq & 7;

        if (pt == PAWN) {
            int own_rank = (color == WHITE) ? rank : (7 - rank);
            if (own_rank < lowest_pawn_rank[color][f])
                lowest_pawn_rank[color][f] = own_rank;
            pr_list[pr_index++] = sq;
        } else if (pt == ROOK) {
            pr_list[pr_index++] = sq;
        }

        // PST index: White uses rank*8+file directly, Black mirrors vertically
        int idx = (color == WHITE) ? rank * 8 + f : (7 - rank) * 8 + f;

        // Material + PST traced as single coefficient
        trace.pst[pt - 1][idx][color]++;

        phase += phase_inc[pt];

        // Mobility for Knight..Queen
        if (pt >= KNIGHT && pt <= QUEEN) {
            // Count pseudo-legal moves (including captures on any square, excluding own pieces)
            u64 moves = 0;
            if (pt == KNIGHT) moves = knight_attacks(sq);
            else if (pt == BISHOP) moves = bishop_attacks(sq, all_pieces);
            else if (pt == ROOK) moves = rook_attacks(sq, all_pieces);
            else if (pt == QUEEN) moves = (bishop_attacks(sq, all_pieces) | rook_attacks(sq, all_pieces));

            // Exclude own pieces from mobility count (matching chal: empty squares + opponent squares)
            int mob = popcount(moves & ~own_pieces[color]) - mob_center[pt];
            trace.mobility[pt - KNIGHT][color] += mob;
        }
    }

    // Bishop pair
    for (int c = 0; c < 2; c++) {
        if (bishop_count[c] >= 2) {
            trace.bishop_pair[c]++;
        }
    }

    // King pawn shield (MG only in original, but we trace it as a tunable pair)
    for (int color = 0; color < 2; color++) {
        int ksq = king_sq[color];
        int kf = ksq & 7;
        if (kf <= 2 || kf >= 5) {
            for (int ft = kf - 1; ft <= kf + 1; ft++) {
                if (ft >= 0 && ft <= 7) {
                    int pawn_rank = lowest_pawn_rank[color][ft];
                    trace.king_shield[pawn_rank][color]++;

                    // Open file adjacent to king penalty
                    if (pawn_rank == 7 && lowest_pawn_rank[color ^ 1][ft] == 7) {
                        trace.king_shield_open[color]++;
                    }
                }
            }
        }
    }

    // Second pass: rook files, pawn structure
    for (int i = 0; i < pr_index; i++) {
        int sq = pr_list[i];
        int pt = pos.piece[sq];
        int color = pos.color[sq];
        int f = sq & 7;

        if (pt == ROOK) {
            if (lowest_pawn_rank[color][f] == 7) {
                if (lowest_pawn_rank[color ^ 1][f] == 7) {
                    trace.rook_open_file[color]++;
                } else {
                    trace.rook_semi_open_file[color]++;
                }
            }
            continue;
        }

        // PAWN
        int rank = sq >> 3;
        int own_rank = (color == WHITE) ? rank : (7 - rank);
        int enemy = color ^ 1;

        // Doubled
        if (own_rank != lowest_pawn_rank[color][f]) {
            trace.doubled_pawn[color]++;
        }

        // Passed and isolated detection
        int passed = 1, isolated = 1;
        for (int df = -1; df <= 1; df++) {
            int ef = f + df;
            if (ef < 0 || ef > 7) continue;

            if (lowest_pawn_rank[enemy][ef] != 7) {
                int enemy_front_rank = 7 - lowest_pawn_rank[enemy][ef];
                if (enemy_front_rank >= own_rank) passed = 0;
            }
            if (df != 0 && lowest_pawn_rank[color][ef] != 7) isolated = 0;
        }

        if (isolated) {
            trace.isolated_pawn[color]++;
        }

        if (!passed) continue;

        // Passed pawn: check if blocked
        int front_sq = sq + (color == WHITE ? 8 : -8);
        bool blocked = (front_sq >= 0 && front_sq < 64 && pos.piece[front_sq] != EMPTY && pos.color[front_sq] == enemy);

        if (blocked) {
            trace.passed_pawn_blocked[own_rank][color]++;
        } else {
            trace.passed_pawn[own_rank][color]++;
        }

        // King distance factor (EG only) -- applies to all passed pawns
        int king_dist_diff = chebyshev_distance(sq, king_sq[enemy]) - chebyshev_distance(sq, king_sq[color]);
        trace.passed_pawn_king_dist[color] += king_dist_diff;
    }

    // Compute actual score for the trace
    // MG
    for (int c = 0; c < 2; c++) {
        for (int p = 0; p < 6; p++)
            for (int sq = 0; sq < 64; sq++)
                mg[c] += mg_pst[p][sq] * trace.pst[p][sq][c];

        mg[c] += mob_step_mg[KNIGHT] * trace.mobility[0][c];
        mg[c] += mob_step_mg[BISHOP] * trace.mobility[1][c];
        mg[c] += mob_step_mg[ROOK]   * trace.mobility[2][c];
        mg[c] += mob_step_mg[QUEEN]  * trace.mobility[3][c];

        mg[c] += bishop_pair_mg * trace.bishop_pair[c];

        for (int r = 0; r < 8; r++)
            mg[c] += shield_val[r] * trace.king_shield[r][c];
        mg[c] += shield_open_penalty * trace.king_shield_open[c];

        mg[c] += rook_open_file_mg * trace.rook_open_file[c];
        mg[c] += rook_semi_open_file_mg * trace.rook_semi_open_file[c];

        mg[c] += doubled_pawn_mg * trace.doubled_pawn[c];
        mg[c] += isolated_pawn_mg * trace.isolated_pawn[c];

        for (int r = 0; r < 8; r++) {
            mg[c] += pp_mg[r] * trace.passed_pawn[r][c];
            mg[c] += pp_blocked_mg[r] * trace.passed_pawn_blocked[r][c];
        }
        // king distance is EG only, no MG contribution
    }

    // EG
    for (int c = 0; c < 2; c++) {
        for (int p = 0; p < 6; p++)
            for (int sq = 0; sq < 64; sq++)
                eg[c] += eg_pst[p][sq] * trace.pst[p][sq][c];

        eg[c] += mob_step_eg[KNIGHT] * trace.mobility[0][c];
        eg[c] += mob_step_eg[BISHOP] * trace.mobility[1][c];
        eg[c] += mob_step_eg[ROOK]   * trace.mobility[2][c];
        eg[c] += mob_step_eg[QUEEN]  * trace.mobility[3][c];

        eg[c] += bishop_pair_eg * trace.bishop_pair[c];

        // Shield is MG only in original, but we allow EG tuning too (initialized 0)
        // (no EG shield contribution in initial values)

        eg[c] += rook_open_file_eg * trace.rook_open_file[c];
        eg[c] += rook_semi_open_file_eg * trace.rook_semi_open_file[c];

        eg[c] += doubled_pawn_eg * trace.doubled_pawn[c];
        eg[c] += isolated_pawn_eg * trace.isolated_pawn[c];

        for (int r = 0; r < 8; r++) {
            eg[c] += pp_eg[r] * trace.passed_pawn[r][c];
            eg[c] += pp_blocked_eg[r] * trace.passed_pawn_blocked[r][c];
        }
        eg[c] += pp_king_distance_eg * trace.passed_pawn_king_dist[c];
    }

    // Taper -- always from white's perspective to match tuner's linear_eval
    int p = phase > 24 ? 24 : phase;
    int mg_score = mg[WHITE] - mg[BLACK];
    int eg_score = eg[WHITE] - eg[BLACK];
    trace.score = (mg_score * p + eg_score * (24 - p)) / 24;
    trace.endgame_scale = 1;

    return trace;
}

// --- Coefficient extraction ---

static coefficients_t get_coefficients(const Trace& trace) {
    coefficients_t coefficients;

    // PST: 6 pieces * 64 squares
    for (int p = 0; p < 6; p++)
        for (int sq = 0; sq < 64; sq++)
            get_coefficient_single(coefficients, trace.pst[p][sq]);

    // Mobility: 4 piece types
    get_coefficient_array(coefficients, trace.mobility, 4);

    // Bishop pair
    get_coefficient_single(coefficients, trace.bishop_pair);

    // King shield: 8 rank values + open penalty
    get_coefficient_array(coefficients, trace.king_shield, 8);
    get_coefficient_single(coefficients, trace.king_shield_open);

    // Rook files
    get_coefficient_single(coefficients, trace.rook_open_file);
    get_coefficient_single(coefficients, trace.rook_semi_open_file);

    // Pawn structure
    get_coefficient_single(coefficients, trace.doubled_pawn);
    get_coefficient_single(coefficients, trace.isolated_pawn);

    // Passed pawns
    get_coefficient_array(coefficients, trace.passed_pawn, 8);
    get_coefficient_array(coefficients, trace.passed_pawn_blocked, 8);
    get_coefficient_single(coefficients, trace.passed_pawn_king_dist);

    return coefficients;
}

// --- Initial parameters ---

parameters_t ChalEval::get_initial_parameters() {
    parameters_t parameters;

    // PST: 6 pieces * 64 squares, packed as S(mg, eg)
    for (int p = 0; p < 6; p++)
        for (int sq = 0; sq < 64; sq++) {
            const pair_t pair = { static_cast<tune_t>(mg_pst[p][sq]), static_cast<tune_t>(eg_pst[p][sq]) };
            parameters.push_back(pair);
        }

    // Mobility: Knight, Bishop, Rook, Queen
    for (int i = KNIGHT; i <= QUEEN; i++) {
        const pair_t pair = { static_cast<tune_t>(mob_step_mg[i]), static_cast<tune_t>(mob_step_eg[i]) };
        parameters.push_back(pair);
    }

    // Bishop pair
    {
        const pair_t pair = { static_cast<tune_t>(bishop_pair_mg), static_cast<tune_t>(bishop_pair_eg) };
        parameters.push_back(pair);
    }

    // King shield: 8 values (MG only initially, EG = 0)
    for (int r = 0; r < 8; r++) {
        const pair_t pair = { static_cast<tune_t>(shield_val[r]), 0.0 };
        parameters.push_back(pair);
    }

    // Shield open penalty
    {
        const pair_t pair = { static_cast<tune_t>(shield_open_penalty), 0.0 };
        parameters.push_back(pair);
    }

    // Rook open file
    {
        const pair_t pair = { static_cast<tune_t>(rook_open_file_mg), static_cast<tune_t>(rook_open_file_eg) };
        parameters.push_back(pair);
    }

    // Rook semi-open file
    {
        const pair_t pair = { static_cast<tune_t>(rook_semi_open_file_mg), static_cast<tune_t>(rook_semi_open_file_eg) };
        parameters.push_back(pair);
    }

    // Doubled pawn
    {
        const pair_t pair = { static_cast<tune_t>(doubled_pawn_mg), static_cast<tune_t>(doubled_pawn_eg) };
        parameters.push_back(pair);
    }

    // Isolated pawn
    {
        const pair_t pair = { static_cast<tune_t>(isolated_pawn_mg), static_cast<tune_t>(isolated_pawn_eg) };
        parameters.push_back(pair);
    }

    // Passed pawns (unblocked)
    for (int r = 0; r < 8; r++) {
        const pair_t pair = { static_cast<tune_t>(pp_mg[r]), static_cast<tune_t>(pp_eg[r]) };
        parameters.push_back(pair);
    }

    // Passed pawns (blocked)
    for (int r = 0; r < 8; r++) {
        const pair_t pair = { static_cast<tune_t>(pp_blocked_mg[r]), static_cast<tune_t>(pp_blocked_eg[r]) };
        parameters.push_back(pair);
    }

    // Passed pawn king distance (EG only)
    {
        const pair_t pair = { 0.0, static_cast<tune_t>(pp_king_distance_eg) };
        parameters.push_back(pair);
    }

    return parameters;
}

// --- Eval result from FEN ---

EvalResult ChalEval::get_fen_eval_result(const string& fen) {
    Position position;
    set_fen(position, fen);
    const auto trace = eval(position);
    EvalResult result;
    result.coefficients = get_coefficients(trace);
    result.score = trace.score;
    result.endgame_scale = trace.endgame_scale;
    return result;
}

EvalResult ChalEval::get_external_eval_result(const chess::Board& board) {
    auto position = get_position_from_board(board);
    const auto trace = eval(position);
    EvalResult result;
    result.coefficients = get_coefficients(trace);
    result.score = trace.score;
    result.endgame_scale = trace.endgame_scale;
    return result;
}

// --- Print parameters in chal.c format ---
// Output is designed to be copy-pasted directly into chal.c

static int32_t round_value(tune_t value) {
    return static_cast<int32_t>(round(value));
}

static int rv_mg(const pair_t& p) { return round_value(p[static_cast<int>(PhaseStages::Midgame)]); }
static int rv_eg(const pair_t& p) { return round_value(p[static_cast<int>(PhaseStages::Endgame)]); }

void ChalEval::print_parameters(const parameters_t& parameters) {
    int index = 0;
    stringstream ss;

    // mg_pst[6][64] -- format matches chal.c line 943
    ss << "static int mg_pst[6][64] = {";
    for (int p = 0; p < 6; p++) {
        ss << " {";
        for (int sq = 0; sq < 64; sq++) {
            int v = rv_mg(parameters[index + p * 64 + sq]);
            if (sq > 0) {
                ss << ",";
                if (sq % 8 == 0) ss << " ";
            }
            ss << v;
        }
        ss << "}";
        if (p < 5) ss << ",";
    }
    ss << " };\n";

    // eg_pst[6][64] -- format matches chal.c line 944
    ss << "static int eg_pst[6][64] = {";
    for (int p = 0; p < 6; p++) {
        ss << " {";
        for (int sq = 0; sq < 64; sq++) {
            int v = rv_eg(parameters[index + p * 64 + sq]);
            if (sq > 0) {
                ss << ",";
                if (sq % 8 == 0) ss << " ";
            }
            ss << v;
        }
        ss << "}";
        if (p < 5) ss << ",";
    }
    ss << " };\n";
    index += 6 * 64; // 384

    // mob_step_mg[7] and mob_step_eg[7] -- format matches chal.c lines 965-966
    ss << "static const int mob_step_mg[7] = { 0, 0, "
       << rv_mg(parameters[index]) << ", "
       << rv_mg(parameters[index + 1]) << ", "
       << rv_mg(parameters[index + 2]) << ", "
       << rv_mg(parameters[index + 3]) << ", 0 };\n";
    ss << "static const int mob_step_eg[7] = { 0, 0, "
       << rv_eg(parameters[index]) << ", "
       << rv_eg(parameters[index + 1]) << ", "
       << rv_eg(parameters[index + 2]) << ", "
       << rv_eg(parameters[index + 3]) << ", 0 };\n";
    index += 4; // 388

    // Bishop pair -- inline in code as: add_score(mg, eg, c, MG, EG);
    // chal.c line 1034
    ss << "        if (count[c][BISHOP] >= 2) add_score(mg, eg, c, "
       << rv_mg(parameters[index]) << ", " << rv_eg(parameters[index]) << ");\n";
    index++; // 389

    // shield_val[8] -- chal.c line 1040
    ss << "    static const int shield_val[8] = {";
    for (int r = 0; r < 8; r++) {
        if (r > 0) ss << ",";
        ss << " " << rv_mg(parameters[index + r]);
    }
    ss << " };\n";
    index += 8; // 397

    // shield open penalty -- chal.c line 1047: shield -= N * (...)
    ss << "                if (lowest_pawn_rank[color][ft] == 7) shield -= "
       << -rv_mg(parameters[index])
       << " * (lowest_pawn_rank[color ^ 1][ft] == 7);\n";
    index++; // 398

    // Rook open/semi-open -- chal.c line 1065
    int rook_open_mg = rv_mg(parameters[index]);
    int rook_open_eg = rv_eg(parameters[index]);
    index++; // 399
    int rook_semi_mg = rv_mg(parameters[index]);
    int rook_semi_eg = rv_eg(parameters[index]);
    index++; // 400
    ss << "            if (lowest_pawn_rank[color][f] == 7) {\n";
    ss << "                if (lowest_pawn_rank[color ^ 1][f] == 7)\n";
    ss << "                    add_score(mg, eg, color, " << rook_open_mg << ", " << rook_open_eg << ");\n";
    ss << "                else\n";
    ss << "                    add_score(mg, eg, color, " << rook_semi_mg << ", " << rook_semi_eg << ");\n";
    ss << "            }\n";

    // Doubled pawn -- chal.c line 1078
    ss << "            add_score(mg, eg, color, " << rv_mg(parameters[index]) << ", " << rv_eg(parameters[index]) << "); /* doubled */\n";
    index++; // 401

    // Isolated pawn -- chal.c line 1095
    ss << "        if (isolated) add_score(mg, eg, color, " << rv_mg(parameters[index]) << ", " << rv_eg(parameters[index]) << ");\n";
    index++; // 402

    // Passed pawns (unblocked) -- chal.c lines 1056-1057
    ss << "    static const int pp_mg[8] = {";
    for (int r = 0; r < 8; r++) {
        if (r > 0) ss << ",";
        ss << " " << rv_mg(parameters[index + r]);
    }
    ss << " };\n";
    ss << "    static const int pp_eg[8] = {";
    for (int r = 0; r < 8; r++) {
        if (r > 0) ss << ",";
        ss << " " << rv_eg(parameters[index + r]);
    }
    ss << " };\n";
    index += 8; // 410

    // Passed pawns (blocked) -- new arrays for chal.c
    ss << "    static const int pp_blocked_mg[8] = {";
    for (int r = 0; r < 8; r++) {
        if (r > 0) ss << ",";
        ss << " " << rv_mg(parameters[index + r]);
    }
    ss << " };\n";
    ss << "    static const int pp_blocked_eg[8] = {";
    for (int r = 0; r < 8; r++) {
        if (r > 0) ss << ",";
        ss << " " << rv_eg(parameters[index + r]);
    }
    ss << " };\n";
    index += 8; // 418

    // Passed pawn king distance factor (EG only) -- chal.c line 1102
    ss << "        bonus_eg += " << rv_eg(parameters[index])
       << " * (distance(sq, king_sq(enemy)) - distance(sq, king_sq(color)));\n";
    index++; // 419

    cout << ss.str() << "\n";
}
