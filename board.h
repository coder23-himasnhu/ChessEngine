#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <utility>
#include <unordered_map>
#include <string>

enum Piece{
    EMPTY,
    BQ, BR, BP, BN, BK, BB,
    WQ, WR, WP, WN, WK, WB
};

// Simple move record used for make/unmake
struct Move {
    int fromR, fromC;
    int toR, toC;
    Piece moved;      // piece that moved
    Piece captured;   // captured piece (if any)

    // Snapshot of castling rights so unmake can restore them
    bool prevWhiteKingMoved;
    bool prevBlackKingMoved;
    bool prevWhiteLeftRookMoved;
    bool prevWhiteRightRookMoved;
    bool prevBlackLeftRookMoved;
    bool prevBlackRightRookMoved;

    // Promotion support
    bool wasPromotion = false;
    Piece promotedTo = EMPTY;

    bool wasEnPassant;
    std::pair<int,int> prevEnPassantTarget;

    int prevHalfMoveClock;
};

class board
{
public:
    std::vector<std::vector<Piece>> CurrentState;
    board();
    void reset_board();
    void update_board();

    // pseudo-legal generator
    std::vector<std::pair<int,int>> getLegalMoves(int row, int col);


    // returns fully legal destinations for a piece (uses getAllLegalMoves)
    std::vector<std::pair<int,int>> getFullyLegalDestinations(int fromR, int fromC, bool white);

    // make / unmake a move (simple; used for testing move legality)
    // NOTE: added `promotion` parameter (EMPTY unless a promotion is requested)
    Move makeMove(int fromR, int fromC, int toR, int toC, Piece promotion = EMPTY);
    void unmakeMove(const Move &m);

    // king-in-check test
    bool isKingInCheck(bool white);

    // fully legal moves (returns Move records)
    std::vector<Move> getAllLegalMoves(bool white);
    std::vector<Move> getAllPseudoLegalMoves(bool white);

    // checkmate / stalemate
    bool isCheckmate(bool white);
    bool isStalemate(bool white);

    std::pair<int,int> enPassantTarget;
    int halfMoveClock = 0;

    std::unordered_map<std::string, int> positionCount;
    std::string getPositionKey(bool whiteToMove);


private:
    bool isInsideBoard(int r, int c);
    bool isWhitePiece(Piece p);
    bool isBlackPiece(Piece p);



    // Castling rights stored here
    bool whiteKingMoved;
    bool blackKingMoved;
    bool whiteLeftRookMoved;   // rook at a1 (white)
    bool whiteRightRookMoved;  // rook at h1 (white)
    bool blackLeftRookMoved;   // rook at a8 (black)
    bool blackRightRookMoved;  // rook at h8 (black)
};

#endif // BOARD_H
