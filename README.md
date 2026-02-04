♟️ Qt Chess Game with AI Engine

A fully featured desktop chess application built using C++ and Qt, featuring a custom-built chess engine, complete rule enforcement, and an interactive graphical user interface. The application allows a human player to compete against an AI opponent with real-time move highlighting, undo/redo support, and full game-state validation.

🚀 Features
🎮 Gameplay

Human vs AI chess gameplay

Turn-based move handling with visual feedback

Real-time check, checkmate, stalemate, and draw detection

Supports pawn promotion, castling, and en passant

🧠 Chess Engine

Custom chess engine implemented from scratch

AI opponent powered by Negamax search with alpha–beta pruning

Depth-limited search for efficient move evaluation

Reversible makeMove / unmakeMove system for fast engine analysis

Threefold repetition and fifty-move rule detection

🖥️ Graphical User Interface (Qt)

Interactive 8×8 chessboard with rank/file labels

Click-based piece movement

Legal move highlighting:

Dots for normal moves

Rings for capture moves

Highlighted king when in check

Algebraic-style move history panel

Undo / Redo buttons with keyboard shortcuts (Ctrl+Z, Ctrl+Y)

Asynchronous AI computation using QtConcurrent to keep UI responsive
