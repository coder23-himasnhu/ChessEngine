#include "board.h"
#include <algorithm>

board::board():CurrentState(8, std::vector<Piece>(8, EMPTY)) {
    // initialize castling rights
    whiteKingMoved = false;
    blackKingMoved = false;
    whiteLeftRookMoved = false;
    whiteRightRookMoved = false;
    blackLeftRookMoved = false;
    blackRightRookMoved = false;

    // en-passant target inactive
    enPassantTarget = {-1, -1};

    reset_board();
}

void board::reset_board() {
    // First, fill everything with EMPTY
    for (auto &row : CurrentState)
        std::fill(row.begin(), row.end(), EMPTY);

    // --- Black pieces ---
    CurrentState[0] = { BR, BN, BB, BQ, BK, BB, BN, BR };  // Back rank
    CurrentState[1] = { BP, BP, BP, BP, BP, BP, BP, BP };  // Pawns

    // --- Empty rows ---
    for (int i = 2; i <= 5; ++i)
        std::fill(CurrentState[i].begin(), CurrentState[i].end(), EMPTY);

    // --- White pieces ---
    CurrentState[6] = { WP, WP, WP, WP, WP, WP, WP, WP };  // Pawns
    CurrentState[7] = { WR, WN, WB, WQ, WK, WB, WN, WR };  // Back rank

    // Reset castling rights (fresh game)
    whiteKingMoved = false;
    blackKingMoved = false;
    whiteLeftRookMoved = false;
    whiteRightRookMoved = false;
    blackLeftRookMoved = false;
    blackRightRookMoved = false;

    // Reset en-passant target
    enPassantTarget = {-1, -1};
}

void board::update_board(){
    // currently unused — keep for future use
}

bool board::isInsideBoard(int r, int c) {
    return r >= 0 && r < 8 && c >= 0 && c < 8;
}

bool board::isWhitePiece(Piece p) {
    return p == WP || p == WR || p == WN || p == WB || p == WQ || p == WK;
}

bool board::isBlackPiece(Piece p) {
    return p == BP || p == BR || p == BN || p == BB || p == BQ || p == BK;
}

std::vector<std::pair<int, int>> board::getLegalMoves(int row, int col) {
    // PSEUDO-LEGAL moves only. Do NOT call isKingInCheck() here.
    std::vector<std::pair<int, int>> moves;
    if (!isInsideBoard(row, col)) return moves;

    Piece piece = CurrentState[row][col];
    if (piece == EMPTY) return moves;

    // Helper lambda to add capture or empty square if valid
    auto tryAdd = [&](int r, int c, bool allowCapture, bool allowEmpty){
        if (!isInsideBoard(r,c)) return;
        Piece targ = CurrentState[r][c];
        if (targ == EMPTY && allowEmpty) moves.emplace_back(r,c);
        else if (targ != EMPTY && allowCapture) {
            // only capture opposite-color pieces
            if (isWhitePiece(piece) && isBlackPiece(targ)) moves.emplace_back(r,c);
            if (isBlackPiece(piece) && isWhitePiece(targ)) moves.emplace_back(r,c);
        }
    };

    // Pawn moves
    if (piece == WP || piece == BP) {
        bool isWhite = isWhitePiece(piece);
        int dir = isWhite ? -1 : 1;
        int startRow = isWhite ? 6 : 1;
        int oneR = row + dir;

        // Forward one
        if (isInsideBoard(oneR, col) && CurrentState[oneR][col] == EMPTY)
            moves.emplace_back(oneR, col);

        // Forward two from starting rank
        int twoR = row + 2*dir;
        if (row == startRow && isInsideBoard(oneR, col) && CurrentState[oneR][col] == EMPTY
            && isInsideBoard(twoR, col) && CurrentState[twoR][col] == EMPTY)
            moves.emplace_back(twoR, col);

        // Captures (only existing pieces here - en-passant handled separately in getAllLegalMoves)
        for (int dc : {-1, 1}) {
            int cr = row + dir, cc = col + dc;
            if (!isInsideBoard(cr,cc)) continue;
            Piece targ = CurrentState[cr][cc];
            if (targ != EMPTY) {
                if (isWhite && isBlackPiece(targ)) moves.emplace_back(cr,cc);
                if (!isWhite && isWhitePiece(targ)) moves.emplace_back(cr,cc);
            }
        }

        // Note: en-passant and promotion handled later in getAllLegalMoves
        return moves;
    }

    // Knight moves
    if (piece == WN || piece == BN) {
        const int kd[8][2] = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};
        for (auto &d : kd) {
            int nr = row + d[0], nc = col + d[1];
            if (!isInsideBoard(nr,nc)) continue;
            Piece targ = CurrentState[nr][nc];
            if (targ == EMPTY) moves.emplace_back(nr,nc);
            else {
                if (isWhitePiece(piece) && isBlackPiece(targ)) moves.emplace_back(nr,nc);
                if (isBlackPiece(piece) && isWhitePiece(targ)) moves.emplace_back(nr,nc);
            }
        }
        return moves;
    }

    // Sliding pieces: rook, bishop, queen
    auto slide = [&](const std::vector<std::pair<int,int>>& dirs) {
        for (auto d : dirs) {
            int dr = d.first, dc = d.second;
            int nr = row + dr, nc = col + dc;
            while (isInsideBoard(nr,nc)) {
                Piece targ = CurrentState[nr][nc];
                if (targ == EMPTY) {
                    moves.emplace_back(nr,nc);
                } else {
                    // can capture opponent
                    if ( (isWhitePiece(piece) && isBlackPiece(targ)) ||
                        (isBlackPiece(piece) && isWhitePiece(targ)) ) {
                        moves.emplace_back(nr,nc);
                    }
                    break; // blocked beyond capture or own piece
                }
                nr += dr; nc += dc;
            }
        }
    };

    if (piece == WR || piece == BR) {
        std::vector<std::pair<int,int>> rookDirs = {{1,0},{-1,0},{0,1},{0,-1}};
        slide(rookDirs);
        return moves;
    }

    if (piece == WB || piece == BB) {
        std::vector<std::pair<int,int>> bishopDirs = {{1,1},{1,-1},{-1,1},{-1,-1}};
        slide(bishopDirs);
        return moves;
    }

    if (piece == WQ || piece == BQ) {
        std::vector<std::pair<int,int>> queenDirs = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
        slide(queenDirs);
        return moves;
    }

    if (piece == WK || piece == BK) {
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int nr = row + dr, nc = col + dc;
                if (!isInsideBoard(nr,nc)) continue;
                Piece targ = CurrentState[nr][nc];
                if (targ == EMPTY) moves.emplace_back(nr,nc);
                else {
                    if (isWhitePiece(piece) && isBlackPiece(targ)) moves.emplace_back(nr,nc);
                    if (isBlackPiece(piece) && isWhitePiece(targ)) moves.emplace_back(nr,nc);
                }
            }
        }

        // CASTLING IS DELIBERATELY NOT HANDLED HERE (keep pseudo-legal)
        return moves;
    }

    return moves;
}

// --- makeMove / unmakeMove --------------------------------------------------
// makeMove records previous castling-rights snapshot in the Move record
Move board::makeMove(int fromR, int fromC, int toR, int toC, Piece promotion) {
    Move mv;
    mv.fromR = fromR; mv.fromC = fromC;
    mv.toR = toR; mv.toC = toC;
    mv.moved = CurrentState[fromR][fromC];
    mv.captured = CurrentState[toR][toC];

    // ---- UPDATE 50-MOVE RULE CLOCK ----
    mv.prevHalfMoveClock = halfMoveClock;




    // Save previous castling rights snapshot
    mv.prevWhiteKingMoved = whiteKingMoved;
    mv.prevBlackKingMoved = blackKingMoved;
    mv.prevWhiteLeftRookMoved = whiteLeftRookMoved;
    mv.prevWhiteRightRookMoved = whiteRightRookMoved;
    mv.prevBlackLeftRookMoved = blackLeftRookMoved;
    mv.prevBlackRightRookMoved = blackRightRookMoved;

    // Save previous en-passant target
    mv.prevEnPassantTarget = enPassantTarget;

    // clear promotion/en-passant flags by default
    mv.wasPromotion = false;
    mv.promotedTo = EMPTY;
    mv.wasEnPassant = false;

    Piece p = mv.moved;

    if (p == WP || p == BP || mv.captured != EMPTY)
        halfMoveClock = 0;   // pawn move or capture resets count
    else
        halfMoveClock += 1;  // quiet move

    // ---- EN PASSANT CAPTURE DETECTION ----
    // If a pawn moves diagonally into an empty square which equals enPassantTarget, then it's an en-passant capture.
    if ((p == WP || p == BP) && abs(fromC - toC) == 1 && fromR != toR && CurrentState[toR][toC] == EMPTY) {
        if (enPassantTarget.first == toR && enPassantTarget.second == toC) {
            mv.wasEnPassant = true;
            // Captured pawn sits behind the target square
            int capturedPawnRow = (p == WP) ? toR + 1 : toR - 1;
            mv.captured = CurrentState[capturedPawnRow][toC];
            // Remove the captured pawn from its square
            CurrentState[capturedPawnRow][toC] = EMPTY;
        }
    }

    // ---- APPLY MOVE ----
    CurrentState[toR][toC] = p;
    CurrentState[fromR][fromC] = EMPTY;

    // ---- CASTLING DETECTION & rook movement ----
    // If a king moved two squares horizontally, treat it as castling and move the rook
    if ((p == WK || p == BK) && fromR == toR && abs(toC - fromC) == 2) {
        int row = fromR;
        if (toC == fromC + 2) {
            // king-side: rook moves from h-file to f-file (7 -> 5)
            CurrentState[row][5] = CurrentState[row][7];
            CurrentState[row][7] = EMPTY;
        } else if (toC == fromC - 2) {
            // queen-side: rook moves from a-file to d-file (0 -> 3)
            CurrentState[row][3] = CurrentState[row][0];
            CurrentState[row][0] = EMPTY;
        }
    }

    // ---- PAWN PROMOTION DETECTION ----
    // If a pawn reached the last rank, apply promotion piece (promotion parameter wins, otherwise default to queen)
    if (p == WP && toR == 0) {
        mv.wasPromotion = true;
        if (promotion != EMPTY)
            mv.promotedTo = promotion;
        else
            mv.promotedTo = WQ; // default queen
        CurrentState[toR][toC] = mv.promotedTo;
    } else if (p == BP && toR == 7) {
        mv.wasPromotion = true;
        if (promotion != EMPTY)
            mv.promotedTo = promotion;
        else
            mv.promotedTo = BQ;
        CurrentState[toR][toC] = mv.promotedTo;
    }

    // ---- UPDATE en-passant TARGET ----
    // Reset by default; set if this move was a pawn double-step.
    enPassantTarget = {-1, -1};
    if (p == WP && fromR == 6 && toR == 4) {
        // white moved 6 -> 4, en-passant target is square passed over (5, fromC)
        enPassantTarget = {5, fromC};
    } else if (p == BP && fromR == 1 && toR == 3) {
        // black moved 1 -> 3, en-passant target is square passed over (2, fromC)
        enPassantTarget = {2, fromC};
    }

    // ---- UPDATE CASTLING RIGHTS (permanent change) ----
    // Note: we saved previous flags in mv; unmakeMove will restore them.
    if (p == WK) whiteKingMoved = true;
    if (p == BK) blackKingMoved = true;

    if (p == WR) {
        if (fromR == 7 && fromC == 0) whiteLeftRookMoved = true;
        if (fromR == 7 && fromC == 7) whiteRightRookMoved = true;
    }
    if (p == BR) {
        if (fromR == 0 && fromC == 0) blackLeftRookMoved = true;
        if (fromR == 0 && fromC == 7) blackRightRookMoved = true;
    }

    return mv;
}

void board::unmakeMove(const Move &m) {
    // Restore previous castling-rights snapshot first (so we can undo rook correctly)
    whiteKingMoved = m.prevWhiteKingMoved;
    blackKingMoved = m.prevBlackKingMoved;
    whiteLeftRookMoved = m.prevWhiteLeftRookMoved;
    whiteRightRookMoved = m.prevWhiteRightRookMoved;
    blackLeftRookMoved = m.prevBlackLeftRookMoved;
    blackRightRookMoved = m.prevBlackRightRookMoved;

    halfMoveClock = m.prevHalfMoveClock;

    // Restore previous en-passant target
    enPassantTarget = m.prevEnPassantTarget;

    // Detect if this was a castling move by seeing if moved piece is king and horizontal distance == 2
    bool wasCastle = (m.moved == WK || m.moved == BK) && (m.fromR == m.toR) && (abs(m.toC - m.fromC) == 2);

    // If it was a castling, restore rook original square first
    if (wasCastle) {
        int row = m.fromR;
        if (m.toC == m.fromC + 2) {
            // king-side: rook was moved from 7 -> 5; put it back
            CurrentState[row][7] = CurrentState[row][5];
            CurrentState[row][5] = EMPTY;
        } else if (m.toC == m.fromC - 2) {
            // queen-side: rook moved from 0 -> 3; put it back
            CurrentState[row][0] = CurrentState[row][3];
            CurrentState[row][3] = EMPTY;
        }
    }

    // If this was a promotion, the destination square currently holds the promoted piece.
    // Restore original pawn on from-square and captured piece on to-square.
    if (m.wasPromotion) {
        CurrentState[m.fromR][m.fromC] = m.moved;
        CurrentState[m.toR][m.toC] = m.captured;
        return;
    }

    // If this was an en-passant capture, we removed the captured pawn from its square at makeMove.
    // Restore that captured pawn and restore moved piece and clear destination.
    if (m.wasEnPassant) {
        int capturedPawnRow = (m.moved == WP) ? m.toR + 1 : m.toR - 1;
        CurrentState[capturedPawnRow][m.toC] = m.captured; // restore the captured pawn
        CurrentState[m.fromR][m.fromC] = m.moved;
        CurrentState[m.toR][m.toC] = EMPTY;
        return;
    }

    // Normal undo
    CurrentState[m.fromR][m.fromC] = m.moved;
    CurrentState[m.toR][m.toC] = m.captured;
}


// --- isKingInCheck ---------------------------------------------------------
// Returns true if the king of the given color is under attack.
// Uses opponent pseudo-legal moves (getLegalMoves). getLegalMoves deliberately
// does not attempt to call isKingInCheck or process castling, so there's no recursion here.
bool board::isKingInCheck(bool white) {
    // find king position
    int kr = -1, kc = -1;
    Piece kingPiece = white ? WK : BK;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (CurrentState[r][c] == kingPiece) {
                kr = r; kc = c;
                break;
            }
        }
        if (kr != -1) break;
    }

    if (kr == -1) {
        // No king found (should not happen in normal play). Treat as not in check.
        return false;
    }

    // For every opponent piece, generate its pseudo-legal moves and see if any attack the king square.
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = CurrentState[r][c];
            if (p == EMPTY) continue;

            // skip same-color pieces
            if (white && isWhitePiece(p)) continue;
            if (!white && isBlackPiece(p)) continue;

            // get pseudo-legal moves for this opponent piece
            std::vector<std::pair<int,int>> targets = getLegalMoves(r, c);

            for (auto &t : targets) {
                if (t.first == kr && t.second == kc) {
                    // opponent can attack the king square
                    return true;
                }
            }
        }
    }

    return false;
}

// --- getAllLegalMoves ------------------------------------------------------
// Generate all pseudo-legal moves for side `white`, filter out those that leave own king in check.
// Also handles castling generation and checks its legality (by simulating passing squares).
std::vector<Move> board::getAllLegalMoves(bool white) {
    std::vector<Move> legalMoves;

    // iterate all squares; if piece is of the requested color, get pseudo moves
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = CurrentState[r][c];
            if (p == EMPTY) continue;
            if (white && !isWhitePiece(p)) continue;
            if (!white && !isBlackPiece(p)) continue;

            auto targets = getLegalMoves(r, c); // pseudo-legal destinations (does not include en-passant)
            for (auto &t : targets) {
                int tr = t.first, tc = t.second;

                // If this is a pawn move that reaches promotion rank, expand into 4 promotion choices.
                if (p == WP && tr == 0) {
                    Piece promos[4] = { WQ, WR, WB, WN };
                    for (Piece promo : promos) {
                        Move m = makeMove(r, c, tr, tc, promo);
                        bool kingInCheck = isKingInCheck(white);
                        unmakeMove(m);
                        if (!kingInCheck) legalMoves.push_back(m);
                    }
                } else if (p == BP && tr == 7) {
                    Piece promos[4] = { BQ, BR, BB, BN };
                    for (Piece promo : promos) {
                        Move m = makeMove(r, c, tr, tc, promo);
                        bool kingInCheck = isKingInCheck(white);
                        unmakeMove(m);
                        if (!kingInCheck) legalMoves.push_back(m);
                    }
                } else {
                    // Normal move
                    Move m = makeMove(r, c, tr, tc);
                    bool kingInCheck = isKingInCheck(white);
                    unmakeMove(m);
                    if (!kingInCheck) {
                        legalMoves.push_back(m);
                    }
                }
            }

            // --- EN PASSANT generation (special-case) ---
            // If there is an en-passant target square, and this piece is a pawn that can capture it,
            // include that capture as a legal candidate (it will be validated by make/unmake check).
            if (p == WP || p == BP) {
                if (enPassantTarget.first != -1) {
                    int tr = enPassantTarget.first;
                    int tc = enPassantTarget.second;
                    // pawn capture direction
                    int dir = isWhitePiece(p) ? -1 : 1;
                    // en-passant capture: pawn must be on same rank as target +/- 1 depending
                    // For white pawn capturing a black double-push, the white pawn sits on row 4 and captures to row 5 (target).
                    // We check that the pawn can move diagonally into enPassantTarget from current square.
                    if ( (tr == r + (isWhitePiece(p) ? -1 : 1)) && (abs(tc - c) == 1) ) {
                        // simulate the en-passant move
                        Move m = makeMove(r, c, tr, tc);
                        bool kingInCheck = isKingInCheck(white);
                        unmakeMove(m);
                        if (!kingInCheck) {
                            legalMoves.push_back(m);
                        }
                    }
                }
            }
        }
    }

    // -------------------------
    // Now generate castling moves (if applicable). We perform these separately
    // so we can carefully check "not in check", "squares passed not attacked", no pieces between, and that king/rook haven't moved.
    // -------------------------
    // White side castling
    if (white) {
        // King must be on initial square
        if (!whiteKingMoved) {
            // kingside: king on 7,4 and rook on 7,7 and rook not moved
            if (!whiteRightRookMoved && CurrentState[7][4] == WK && CurrentState[7][7] == WR) {
                if (CurrentState[7][5] == EMPTY && CurrentState[7][6] == EMPTY) {
                    // king must not be in check now
                    if (!isKingInCheck(true)) {
                        // square king passes: f1 (7,5)
                        Move m1 = makeMove(7,4,7,5);
                        bool sq1ok = !isKingInCheck(true);
                        unmakeMove(m1);

                        // square king lands: g1 (7,6)
                        Move m2 = makeMove(7,4,7,6);
                        bool sq2ok = !isKingInCheck(true);
                        unmakeMove(m2);

                        if (sq1ok && sq2ok) {
                            Move m_castle = makeMove(7,4,7,6); // will move rook too inside makeMove
                            unmakeMove(m_castle); // restore; we only want the Move record
                            legalMoves.push_back(m_castle);
                        }
                    }
                }
            }

            // queenside: king on 7,4 and rook on 7,0 and rook not moved
            if (!whiteLeftRookMoved && CurrentState[7][4] == WK && CurrentState[7][0] == WR) {
                if (CurrentState[7][1] == EMPTY && CurrentState[7][2] == EMPTY && CurrentState[7][3] == EMPTY) {
                    if (!isKingInCheck(true)) {
                        Move m1 = makeMove(7,4,7,3);
                        bool sq1ok = !isKingInCheck(true);
                        unmakeMove(m1);

                        Move m2 = makeMove(7,4,7,2);
                        bool sq2ok = !isKingInCheck(true);
                        unmakeMove(m2);

                        if (sq1ok && sq2ok) {
                            Move m_castle = makeMove(7,4,7,2);
                            unmakeMove(m_castle);
                            legalMoves.push_back(m_castle);
                        }
                    }
                }
            }
        }
    } else {
        // Black side castling
        if (!blackKingMoved) {
            if (!blackRightRookMoved && CurrentState[0][4] == BK && CurrentState[0][7] == BR) {
                if (CurrentState[0][5] == EMPTY && CurrentState[0][6] == EMPTY) {
                    if (!isKingInCheck(false)) {
                        Move m1 = makeMove(0,4,0,5);
                        bool sq1ok = !isKingInCheck(false);
                        unmakeMove(m1);

                        Move m2 = makeMove(0,4,0,6);
                        bool sq2ok = !isKingInCheck(false);
                        unmakeMove(m2);

                        if (sq1ok && sq2ok) {
                            Move m_castle = makeMove(0,4,0,6);
                            unmakeMove(m_castle);
                            legalMoves.push_back(m_castle);
                        }
                    }
                }
            }

            if (!blackLeftRookMoved && CurrentState[0][4] == BK && CurrentState[0][0] == BR) {
                if (CurrentState[0][1] == EMPTY && CurrentState[0][2] == EMPTY && CurrentState[0][3] == EMPTY) {
                    if (!isKingInCheck(false)) {
                        Move m1 = makeMove(0,4,0,3);
                        bool sq1ok = !isKingInCheck(false);
                        unmakeMove(m1);

                        Move m2 = makeMove(0,4,0,2);
                        bool sq2ok = !isKingInCheck(false);
                        unmakeMove(m2);

                        if (sq1ok && sq2ok) {
                            Move m_castle = makeMove(0,4,0,2);
                            unmakeMove(m_castle);
                            legalMoves.push_back(m_castle);
                        }
                    }
                }
            }
        }
    }

    return legalMoves;
}

// -----------------------------
// PSEUDO-LEGAL MOVES (fast)
// -----------------------------
// Creates Move records with .fromR/.fromC/.toR/.toC set and .promotedTo set for promotions.
// Does NOT call makeMove/unmakeMove and does NOT test king safety. Used by engine search.
std::vector<Move> board::getAllPseudoLegalMoves(bool white) {
    std::vector<Move> moves;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = CurrentState[r][c];
            if (p == EMPTY) continue;
            if (white && !isWhitePiece(p)) continue;
            if (!white && !isBlackPiece(p)) continue;

            auto targets = getLegalMoves(r, c); // pseudo-legal destinations (fast)
            for (auto &t : targets) {
                int tr = t.first, tc = t.second;

                // Pawn promotions: expand into 4 promotion choices
                if ((p == WP && tr == 0) || (p == BP && tr == 7)) {
                    if (p == WP) {
                        Piece promos[4] = { WQ, WR, WB, WN };
                        for (Piece promo : promos) {
                            Move m;
                            m.fromR = r; m.fromC = c;
                            m.toR = tr; m.toC = tc;
                            m.promotedTo = promo;
                            // moved/captured left to makeMove (not required here)
                            moves.push_back(m);
                        }
                    } else {
                        Piece promos[4] = { BQ, BR, BB, BN };
                        for (Piece promo : promos) {
                            Move m;
                            m.fromR = r; m.fromC = c;
                            m.toR = tr; m.toC = tc;
                            m.promotedTo = promo;
                            moves.push_back(m);
                        }
                    }
                } else {
                    Move m;
                    m.fromR = r; m.fromC = c;
                    m.toR = tr; m.toC = tc;
                    m.promotedTo = EMPTY;
                    moves.push_back(m);
                }
            }

            // En-passant pseudo move: if enPassantTarget exists and pawn can capture it,
            // include that capture as a candidate. This mimics your getAllLegalMoves logic.
            if ((p == WP || p == BP) && enPassantTarget.first != -1) {
                int er = enPassantTarget.first;
                int ec = enPassantTarget.second;
                int dir = isWhitePiece(p) ? -1 : 1;
                if ( (er == r + (isWhitePiece(p) ? -1 : 1)) && (abs(ec - c) == 1) ) {
                    Move m;
                    m.fromR = r; m.fromC = c;
                    m.toR = er; m.toC = ec;
                    m.promotedTo = EMPTY;
                    moves.push_back(m);
                }
            }
        }
    }

    // Castling as pseudo-legal: generate castling king moves if squares empty and rook present
    // (We do NOT check "squares attacked" here — that's checked later after makeMove)
    // White castling
    if (white) {
        if (!whiteKingMoved) {
            // kingside
            if (!whiteRightRookMoved && CurrentState[7][4] == WK && CurrentState[7][7] == WR) {
                if (CurrentState[7][5] == EMPTY && CurrentState[7][6] == EMPTY) {
                    Move m; m.fromR = 7; m.fromC = 4; m.toR = 7; m.toC = 6; m.promotedTo = EMPTY;
                    moves.push_back(m);
                }
            }
            // queenside
            if (!whiteLeftRookMoved && CurrentState[7][4] == WK && CurrentState[7][0] == WR) {
                if (CurrentState[7][1] == EMPTY && CurrentState[7][2] == EMPTY && CurrentState[7][3] == EMPTY) {
                    Move m; m.fromR = 7; m.fromC = 4; m.toR = 7; m.toC = 2; m.promotedTo = EMPTY;
                    moves.push_back(m);
                }
            }
        }
    } else {
        // Black castling
        if (!blackKingMoved) {
            if (!blackRightRookMoved && CurrentState[0][4] == BK && CurrentState[0][7] == BR) {
                if (CurrentState[0][5] == EMPTY && CurrentState[0][6] == EMPTY) {
                    Move m; m.fromR = 0; m.fromC = 4; m.toR = 0; m.toC = 6; m.promotedTo = EMPTY;
                    moves.push_back(m);
                }
            }
            if (!blackLeftRookMoved && CurrentState[0][4] == BK && CurrentState[0][0] == BR) {
                if (CurrentState[0][1] == EMPTY && CurrentState[0][2] == EMPTY && CurrentState[0][3] == EMPTY) {
                    Move m; m.fromR = 0; m.fromC = 4; m.toR = 0; m.toC = 2; m.promotedTo = EMPTY;
                    moves.push_back(m);
                }
            }
        }
    }

    return moves;
}


// --- checkmate / stalemate helpers ----------------------------------------
bool board::isCheckmate(bool white) {
    if (!isKingInCheck(white)) return false;
    auto moves = getAllLegalMoves(white);
    return moves.empty();
}

bool board::isStalemate(bool white) {
    if (isKingInCheck(white)) return false;
    auto moves = getAllLegalMoves(white);
    return moves.empty();
}

std::vector<std::pair<int,int>> board::getFullyLegalDestinations(int fromR, int fromC, bool white) {
    std::vector<std::pair<int,int>> result;
    auto moves = getAllLegalMoves(white);
    for (const auto &m : moves)
        if (m.fromR == fromR && m.fromC == fromC)
            result.push_back({m.toR, m.toC});
    return result;
}

std::string board::getPositionKey(bool whiteToMove)
{
    std::string key;

    // Board pieces
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            key += std::to_string(CurrentState[r][c]);
            key += ',';
        }
    }

    // Turn
    key += whiteToMove ? 'w' : 'b';

    // Castling rights
    key += whiteKingMoved ? '0' : '1';
    key += whiteLeftRookMoved ? '0' : '1';
    key += whiteRightRookMoved ? '0' : '1';
    key += blackKingMoved ? '0' : '1';
    key += blackLeftRookMoved ? '0' : '1';
    key += blackRightRookMoved ? '0' : '1';

    // En-passant square
    key += std::to_string(enPassantTarget.first) + ",";
    key += std::to_string(enPassantTarget.second);

    return key;
}

