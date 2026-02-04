#include "engine.h"
#include <algorithm>
#include <limits>

static const int INF = 1000000000;

// ----------------------------------------------
// MATERIAL VALUES
// ----------------------------------------------
int Engine::pieceValue(Piece p)
{
    switch (p) {
    case WP: return 100;
    case WN: return 300;
    case WB: return 320;
    case WR: return 500;
    case WQ: return 900;
    case WK: return 0;

    case BP: return -100;
    case BN: return -300;
    case BB: return -320;
    case BR: return -500;
    case BQ: return -900;
    case BK: return 0;

    default: return 0;
    }
}

// ----------------------------------------------
// Piece-Square Tables (SIMPLE VERSION)
// ----------------------------------------------
static const int pawnPST[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {5,10,10,-20,-20,10,10, 5},
    {5,-5,-10, 0, 0,-10,-5, 5},
    {0, 0, 0,20,20, 0, 0, 0},
    {5, 5,10,25,25,10, 5, 5},
    {10,10,20,30,30,20,10,10},
    {50,50,50,50,50,50,50,50},
    {0, 0, 0, 0, 0, 0, 0, 0}
};

static const int knightPST[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20, 0, 0, 0, 0,-20,-40},
    {-30, 0,10,15,15,10, 0,-30},
    {-30, 5,15,20,20,15, 5,-30},
    {-30, 0,15,20,20,15, 0,-30},
    {-30, 5,10,15,15,10, 5,-30},
    {-40,-20, 0, 5, 5, 0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

static const int bishopPST[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10, 5, 0, 0, 0, 0, 5,-10},
    {-10,10,10,10,10,10,10,-10},
    {-10, 0,10,10,10,10, 0,-10},
    {-10, 5, 5,10,10, 5, 5,-10},
    {-10, 0, 5,10,10, 5, 0,-10},
    {-10, 0, 0, 0, 0, 0, 0,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};

// (Rook, Queen, King PST skipped for simplicity; 1500 Elo doesn't need them)

int Engine::pstValue(Piece p, int r, int c)
{
    if (p == WP) return  pawnPST[r][c];
    if (p == BP) return -pawnPST[7-r][c];

    if (p == WN) return  knightPST[r][c];
    if (p == BN) return -knightPST[7-r][c];

    if (p == WB) return  bishopPST[r][c];
    if (p == BB) return -bishopPST[7-r][c];

    return 0;
}

// ----------------------------------------------
// EVALUATION = material + PST + mobility
// ----------------------------------------------
int Engine::evaluate(board &b)
{
    int score = 0;

    // --- Material + PST ---
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = b.CurrentState[r][c];
            if (p == EMPTY) continue;

            score += pieceValue(p);
            score += pstValue(p, r, c);
        }
    }

    // --- Mobility bonus (simple) ---
    auto whiteMoves = b.getAllLegalMoves(true);
    auto blackMoves = b.getAllLegalMoves(false);

    score += (whiteMoves.size() * 2);
    score -= (blackMoves.size() * 2);

    return score;
}

// ----------------------------------------------
// NEGAMAX + ALPHA-BETA
// ----------------------------------------------
int Engine::negamax(board &b, int depth, int alpha, int beta, bool whiteToMove)
{
    if (depth == 0)
        return evaluate(b);

    auto moves = b.getAllLegalMoves(whiteToMove);

    if (moves.empty()) {
        // Checkmate or stalemate
        if (b.isKingInCheck(whiteToMove))
            return -INF + 1; // losing position
        else
            return 0; // stalemate
    }

    int best = -INF;

    for (auto &mv : moves) {
        Move m = b.makeMove(mv.fromR, mv.fromC, mv.toR, mv.toC, mv.wasPromotion ? mv.promotedTo : EMPTY);

        int score = -negamax(b, depth - 1, -beta, -alpha, !whiteToMove);

        b.unmakeMove(m);

        best = std::max(best, score);
        alpha = std::max(alpha, score);

        if (alpha >= beta) break; // prune
    }

    return best;
}

// ----------------------------------------------
// BEST MOVE SELECTION
// ----------------------------------------------
Move Engine::findBestMove(board &b, bool whiteToMove, int depth)
{
    auto moves = b.getAllLegalMoves(whiteToMove);

    Move bestMove;
    int bestScore = -INF;

    for (auto &mv : moves) {
        Move m = b.makeMove(mv.fromR, mv.fromC, mv.toR, mv.toC, mv.wasPromotion ? mv.promotedTo : EMPTY);

        int score = -negamax(b, depth - 1, -INF, INF, !whiteToMove);

        b.unmakeMove(m);

        if (score > bestScore) {
            bestScore = score;
            bestMove = mv;
        }
    }



    return bestMove;
}
