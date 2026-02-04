#ifndef ENGINE_H
#define ENGINE_H

#include "board.h"
#include <vector>

class Engine {
public:
    Move findBestMove(board &b, bool whiteToMove, int depth);

private:
    int evaluate(board &b);
    int negamax(board &b, int depth, int alpha, int beta, bool whiteToMove);

    int pieceValue(Piece p);
    int pstValue(Piece p, int r, int c);
};

#endif
