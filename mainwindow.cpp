#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QDir>
#include <QIcon>
#include <QDebug>
#include <QMessageBox>
#include <QShortcut>
#include <QPainter>
#include <QPen>
#include <QTimer>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    // --- Main horizontal layout ---
    QHBoxLayout *mainLayout = new QHBoxLayout(central);

    // ================================================================
    // LEFT PANEL
    // ================================================================
    QVBoxLayout *leftPanel = new QVBoxLayout();

    // Give the left panel inner padding so children are not flush to the edge
    leftPanel->setContentsMargins(8, 8, 8, 8);
    leftPanel->setSpacing(6);

    QWidget *leftWidget = new QWidget();
    leftWidget->setLayout(leftPanel);
    leftWidget->setFixedWidth(150);
    mainLayout->addWidget(leftWidget);

    // Undo/Redo buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *undoBtn = new QPushButton("Undo", this);
    QPushButton *redoBtn = new QPushButton("Redo", this);
    undoBtn->setFixedSize(50, 30);
    redoBtn->setFixedSize(50, 30);
    undoBtn->setStyleSheet("font-weight: bold; font-size: 14px;");
    redoBtn->setStyleSheet("font-weight: bold; font-size: 14px;");

    buttonLayout->addWidget(undoBtn);
    buttonLayout->addWidget(redoBtn);

    // Put the buttons at the top-left of the left panel (keep them left aligned)
    leftPanel->addLayout(buttonLayout);

    // Move history label (left aligned inside left panel)
    QLabel *historyLabel = new QLabel("Move History", this);
    historyLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    historyLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    leftPanel->addWidget(historyLabel);

    // Move history list - give it a clear frame and a fixed width that fits inside left panel margins
    moveHistoryList = new QListWidget(this);
    moveHistoryList->setFixedWidth(134); // 120 - left/right margins (8+8) = 104
    moveHistoryList->setStyleSheet(
        "font-size: 13px; padding-left: 6px; padding-top: 4px; padding-bottom: 4px;"
        );
    moveHistoryList->setFrameShape(QFrame::StyledPanel);
    moveHistoryList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    // Center the list horizontally inside the left panel but leave it touching top after label
    QHBoxLayout *historyCenter = new QHBoxLayout();
    historyCenter->setContentsMargins(0, 0, 0, 0);
    historyCenter->addStretch();
    historyCenter->addWidget(moveHistoryList);
    historyCenter->addStretch();
    leftPanel->addLayout(historyCenter);

    leftPanel->addStretch();

    connect(undoBtn, &QPushButton::clicked, this, &MainWindow::undoMove);
    connect(redoBtn, &QPushButton::clicked, this, &MainWindow::redoMove);

    // ================================================================
    // CENTER PANEL (turn label + board with rank/file labels)
    // ================================================================
    QVBoxLayout *centerPanel = new QVBoxLayout();
    mainLayout->addLayout(centerPanel);

    // Turn label
    turnLabel = new QLabel("White's Turn", this);
    turnLabel->setAlignment(Qt::AlignCenter);
    turnLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    centerPanel->addWidget(turnLabel, 0, Qt::AlignHCenter);

    // Outer wrapper for board + labels
    QWidget *boardWithLabels = new QWidget(this);
    QGridLayout *outerGrid = new QGridLayout(boardWithLabels);
    outerGrid->setSpacing(0);
    outerGrid->setContentsMargins(0, 0, 0, 0);

    // --- Rank labels (1–8) on left ---
    for (int r = 0; r < 8; r++) {
        QLabel *rank = new QLabel(QString::number(8 - r));
        rank->setAlignment(Qt::AlignCenter);
        rank->setStyleSheet("font-size: 14px; font-weight: bold;");
        rank->setFixedSize(20, 80);
        outerGrid->addWidget(rank, r, 0);
    }

    // --- File labels (A–H) on bottom ---
    for (int c = 0; c < 8; c++) {
        QLabel *file = new QLabel(QString(QChar('A' + c)));
        file->setAlignment(Qt::AlignCenter);
        file->setStyleSheet("font-size: 14px; font-weight: bold;");
        file->setFixedSize(80, 20);
        outerGrid->addWidget(file, 8, c + 1);
    }

    // --- Actual chessboard ---
    QWidget *boardWidget = new QWidget(this);
    boardWidget->setFixedSize(640, 640);

    QGridLayout *grid = new QGridLayout(boardWidget);
    grid->setSpacing(0);
    grid->setContentsMargins(0, 0, 0, 0);

    const int SIZE = 8;
    for (int row = 0; row < SIZE; ++row) {
        for (int col = 0; col < SIZE; ++col) {

            QPushButton *tile = new QPushButton(this);
            QString color = ((row + col) % 2 == 0) ? "#EEEED2" : "#769656";
            tile->setFixedSize(80, 80);

            // FULL square chess-piece behavior (chess.com-like)
            tile->setIconSize(QSize(80, 80));
            tile->setStyleSheet(
                "background-color:" + color +
                "; border: none; padding: 0px; margin: 0px;"
                );
            grid->addWidget(tile, row, col);
            boardButtons[row][col] = tile;

            connect(tile, &QPushButton::clicked, this, [=]() {
                selectedRow = row;
                selectedCol = col;
                handleTileClick();
            });
        }
    }

    // Add board inside outer grid (row 0–7, col 1–8)
    outerGrid->addWidget(boardWidget, 0, 1, 8, 8);

    centerPanel->addWidget(boardWithLabels, 0, Qt::AlignHCenter);
    centerPanel->addStretch();

    // ================================================================
    // RIGHT PANEL
    // ================================================================
    QVBoxLayout *rightPanel = new QVBoxLayout();
    QWidget *rightWidget = new QWidget();
    rightWidget->setLayout(rightPanel);
    rightWidget->setFixedWidth(120);
    mainLayout->addWidget(rightWidget);

    // ================================================================
    // Init game
    // ================================================================
    gameBoard.reset_board();
    updateBoardUI();

    // Keyboard shortcuts
    QShortcut *undoShortcut = new QShortcut(QKeySequence("Ctrl+Z"), this);
    connect(undoShortcut, &QShortcut::activated, this, &MainWindow::undoMove);

    QShortcut *redoShortcut = new QShortcut(QKeySequence("Ctrl+Y"), this);
    connect(redoShortcut, &QShortcut::activated, this, &MainWindow::redoMove);

    engineWatcher = new QFutureWatcher<Move>(this);
    connect(engineWatcher, &QFutureWatcher<Move>::finished, this, &MainWindow::onEngineMoveReady);
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateBoardUI()
{
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Piece piece = gameBoard.CurrentState[row][col];
            QString iconPath;

            switch (piece) {
            case WP: iconPath = ":/images/images/white_pawn.png"; break;
            case WR: iconPath = ":/images/images/white_rook.png"; break;
            case WN: iconPath = ":/images/images/white_knight.png"; break;
            case WB: iconPath = ":/images/images/white_bishop.png"; break;
            case WQ: iconPath = ":/images/images/white_queen.png"; break;
            case WK: iconPath = ":/images/images/white_king.png"; break;

            case BP: iconPath = ":/images/images/black_pawn.png"; break;
            case BR: iconPath = ":/images/images/black_rook.png"; break;
            case BN: iconPath = ":/images/images/black_knight.png"; break;
            case BB: iconPath = ":/images/images/black_bishop.png"; break;
            case BQ: iconPath = ":/images/images/black_queen.png"; break;
            case BK: iconPath = ":/images/images/black_king.png"; break;

            default: iconPath = ""; break;
            }

            if (!iconPath.isEmpty()) {
                QIcon icon(iconPath);
                boardButtons[row][col]->setIcon(icon);
                boardButtons[row][col]->setIconSize(boardButtons[row][col]->size());
            } else {
                boardButtons[row][col]->setIcon(QIcon());
            }
        }
    }
}

void MainWindow::handleTileClick()
{
    // Prevent user interaction while engine is thinking
    if (engineThinking) return;

    static int fromRow = -1, fromCol = -1;

    if (!pieceSelected) {
        Piece piece = gameBoard.CurrentState[selectedRow][selectedCol];
        if (piece == EMPTY) return;

        bool isWhitePiece = (piece >= WQ && piece <= WB);

        if ((isWhiteTurn && !isWhitePiece) || (!isWhiteTurn && isWhitePiece))
            return;

        pieceSelected = true;
        fromRow = selectedRow;
        fromCol = selectedCol;

        boardButtons[fromRow][fromCol]->setStyleSheet("background-color: yellow; border: none;");

        auto moves = gameBoard.getFullyLegalDestinations(fromRow, fromCol, isWhiteTurn);
        highlightMoves(moves);


        return;
    }

    if (pieceSelected) {
        bool valid = false;
        clearHighlights();
        auto legalMoves = gameBoard.getFullyLegalDestinations(fromRow, fromCol, isWhiteTurn);
        for (auto [r, c] : legalMoves) {
            if (r == selectedRow && c == selectedCol) {
                valid = true;
                break;
            }
        }

        resetColors();

        if (valid) {
            // Check for pawn promotion possibility
            Piece movingPiece = gameBoard.CurrentState[fromRow][fromCol];
            Piece promotionChoice = EMPTY;
            bool isPromotionMove = false;

            if (movingPiece == WP && selectedRow == 0) isPromotionMove = true;
            if (movingPiece == BP && selectedRow == 7) isPromotionMove = true;

            if (isPromotionMove) {
                promotionChoice = showPromotionDialog(movingPiece == WP);
            }

            // Call makeMove with promotionChoice (EMPTY if not a promotion)
            Move mv = gameBoard.makeMove(fromRow, fromCol, selectedRow, selectedCol, promotionChoice);
            undoStack.push(mv);
            while (!redoStack.empty()) redoStack.pop(); // clear redo on new move

            QString notation = notationFromMove(mv);

            std::string posKey = gameBoard.getPositionKey(isWhiteTurn);
            gameBoard.positionCount[posKey]++;

            if (gameBoard.positionCount[posKey] >= 3) {
                turnLabel->setText("Draw by Threefold Repetition!");
                updateBoardUI();
                return;
            }

            isWhiteTurn = !isWhiteTurn;

            // If opponent is in check or mate, append suffix
            if (gameBoard.isCheckmate(isWhiteTurn)) notation += "#";
            else if (gameBoard.isKingInCheck(isWhiteTurn)) notation += "+";

            // Add to history
            bool wasWhiteMove = (mv.moved == WP || (mv.moved >= WQ && mv.moved <= WB)); // true if moved piece was white
            addMoveToHistory(notation, wasWhiteMove);

            bool inCheck = gameBoard.isKingInCheck(isWhiteTurn);
            auto nextMoves = gameBoard.getAllLegalMoves(isWhiteTurn);

            if (gameBoard.halfMoveClock >= 100) {
                turnLabel->setText("Draw by Fifty-Move Rule!");
                updateBoardUI();
                return;
            }


            if (nextMoves.empty()) {
                if (inCheck)
                    turnLabel->setText(isWhiteTurn ? "Checkmate! Black Wins!" : "Checkmate! White Wins!");
                else
                    turnLabel->setText("Draw by Stalemate!");
            } else {
                QString turnText = isWhiteTurn ? "White's Turn" : "Black's Turn";
                if (inCheck) turnText += " (in Check!)";
                turnLabel->setText(turnText);
            }

            updateBoardUI();
            if (inCheck) highlightKingInCheck(isWhiteTurn);

            // Only start engine if the side to move has legal moves (i.e., game not over)
            if (!isWhiteTurn) {
                auto nextMovesAfterHuman = gameBoard.getAllLegalMoves(isWhiteTurn);
                if (!nextMovesAfterHuman.empty()) {
                    // Start engine asynchronously (use a copy of the board for thread-safety)
                    if (!engineWatcher->isRunning()) {
                        engineThinking = true;
                        // optional: update UI to indicate thinking (e.g., disable board or change label)
                        turnLabel->setText("Engine thinking...");

                        board boardCopy = gameBoard; // copy to run in background
                        bool colorToMove = isWhiteTurn;
                        int depth = engineDepth;

                        QFuture<Move> future = QtConcurrent::run([this, boardCopy, colorToMove, depth]() mutable {
                            // compute best move on copy -> returns Move relative to the same coordinates
                            return chessEngine.findBestMove(boardCopy, colorToMove, depth);
                        });
                        engineWatcher->setFuture(future);
                    }
                }
            }

        }

        pieceSelected = false;
        fromRow = fromCol = -1;
    }
}

void MainWindow::resetColors()
{
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            QString color = ((r + c) % 2 == 0) ? "#EEEED2" : "#769656";
            boardButtons[r][c]->setStyleSheet("background-color:" + color + "; border: none;");
        }
    }
}

void MainWindow::highlightKingInCheck(bool whiteTurn)
{
    Piece king = whiteTurn ? WK : BK;

    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            if (gameBoard.CurrentState[r][c] == king)
                boardButtons[r][c]->setStyleSheet("background-color: red; border: none;");
}

// show a simple modal dialog to pick promotion piece; returns the Piece enum value chosen.
// whiteSide == true -> return WQ/WR/WB/WN; else return BQ/BR/BB/BN
Piece MainWindow::showPromotionDialog(bool whiteSide)
{
    QMessageBox msg(this);
    msg.setWindowTitle("Pawn Promotion");
    msg.setText("Promote pawn to:");
    QPushButton *qBtn = msg.addButton("Queen", QMessageBox::AcceptRole);
    QPushButton *rBtn = msg.addButton("Rook", QMessageBox::AcceptRole);
    QPushButton *bBtn = msg.addButton("Bishop", QMessageBox::AcceptRole);
    QPushButton *nBtn = msg.addButton("Knight", QMessageBox::AcceptRole);

    msg.exec();

    if (msg.clickedButton() == qBtn) {
        return whiteSide ? WQ : BQ;
    } else if (msg.clickedButton() == rBtn) {
        return whiteSide ? WR : BR;
    } else if (msg.clickedButton() == bBtn) {
        return whiteSide ? WB : BB;
    } else {
        return whiteSide ? WN : BN;
    }
}

void MainWindow::undoMove()
{
    if (undoStack.empty()) return;

    Move mv = undoStack.top();
    undoStack.pop();

    gameBoard.unmakeMove(mv);
    redoStack.push(mv);

    // Update history based on which color moved
    bool whiteMoved = (mv.moved == WP || (mv.moved >= WQ && mv.moved <= WB));
    if (whiteMoved) {
        // remove last item (white move)
        if (moveHistoryList && moveHistoryList->count() > 0) {
            delete moveHistoryList->takeItem(moveHistoryList->count() - 1);
            fullMoveNumber = std::max(0, fullMoveNumber - 1);
        }
    } else {
        // black move: remove the appended black move text from last item
        if (moveHistoryList && moveHistoryList->count() > 0) {
            QListWidgetItem *it = moveHistoryList->item(moveHistoryList->count() - 1);
            QString text = it->text();
            // remove everything after the 4-space separator "    "
            int idx = text.indexOf("    ");
            if (idx != -1) {
                text = text.left(idx);
                it->setText(text);
            } else {
                // fallback: remove whole line
                delete moveHistoryList->takeItem(moveHistoryList->count() - 1);
                fullMoveNumber = std::max(0, fullMoveNumber - 1);
            }
        }
    }

    isWhiteTurn = !isWhiteTurn; // revert turn (keep as you had it)

    updateBoardUI();
    resetColors();

    // Update turn label
    QString turnText = isWhiteTurn ? "White's Turn" : "Black's Turn";
    bool inCheck = gameBoard.isKingInCheck(isWhiteTurn);
    if (inCheck) turnText += " (in Check!)";
    turnLabel->setText(turnText);

    if (inCheck) highlightKingInCheck(isWhiteTurn);
}

void MainWindow::redoMove()
{
    if (redoStack.empty()) return;

    Move mv = redoStack.top();
    redoStack.pop();

    // Re-apply the same move (preserve promotion)
    gameBoard.makeMove(mv.fromR, mv.fromC, mv.toR, mv.toC, mv.wasPromotion ? mv.promotedTo : EMPTY);
    undoStack.push(mv);

    // Build notation and append to history like a new move
    QString notation = notationFromMove(mv);

    // toggle turn as a normal move does
    isWhiteTurn = !isWhiteTurn;

    if (gameBoard.isCheckmate(isWhiteTurn)) notation += "#";
    else if (gameBoard.isKingInCheck(isWhiteTurn)) notation += "+";

    bool wasWhiteMove = (mv.moved == WP || (mv.moved >= WQ && mv.moved <= WB));
    addMoveToHistory(notation, wasWhiteMove);

    updateBoardUI();
    resetColors();

    QString turnText = isWhiteTurn ? "White's Turn" : "Black's Turn";
    bool inCheck = gameBoard.isKingInCheck(isWhiteTurn);
    if (inCheck) turnText += " (in Check!)";
    turnLabel->setText(turnText);

    if (inCheck) highlightKingInCheck(isWhiteTurn);
}


// Convert Move -> simple algebraic-like notation (not full PGN, but good for history)
// Uses Move's from/to, moved, captured, wasPromotion & promotedTo.
// Returns e.g. "e4", "exd8=Q", "Nf3", "O-O", "O-O-O"
QString MainWindow::notationFromMove(const Move &mv) {
    // Cast values to local names
    int fr = mv.fromR, fc = mv.fromC, tr = mv.toR, tc = mv.toC;
    Piece moved = mv.moved;
    Piece captured = mv.captured;

    auto file = [](int c){ return QChar('a' + c); };
    auto rank = [](int r){ return QChar('8' - r); };

    // Castling detection: king moved two squares horizontally
    if ((moved == WK || moved == BK) && fr == tr && qAbs(tc - fc) == 2) {
        if (tc > fc) return QString("O-O"); else return QString("O-O-O");
    }

    QString s;

    // Piece letter (uppercase single letter), pawns omit
    if (!(moved == WP || moved == BP)) {
        // map piece -> letter
        switch (moved) {
        case WN: case BN: s += 'N'; break;
        case WB: case BB: s += 'B'; break;
        case WR: case BR: s += 'R'; break;
        case WQ: case BQ: s += 'Q'; break;
        case WK: case BK: s += 'K'; break;
        default: break;
        }
    }

    // Capture
    if (captured != EMPTY) {
        // For pawn captures, include origin file (exd5)
        if (moved == WP || moved == BP) {
            s += file(fc);
        }
        s += 'x';
    }

    // Destination square
    s += file(tc);
    s += rank(tr);

    // Promotion
    if (mv.wasPromotion) {
        s += '=';
        switch (mv.promotedTo) {
        case WQ: case BQ: s += 'Q'; break;
        case WR: case BR: s += 'R'; break;
        case WB: case BB: s += 'B'; break;
        case WN: case BN: s += 'N'; break;
        default: break;
        }
    }

    // After move is applied externally, caller will decide check/mate suffix.
    return s;
}

// Add a move to the history list (handles numbering: white starts new line, black appends)
void MainWindow::addMoveToHistory(const QString &notation, bool wasWhiteMove) {
    if (!moveHistoryList) return;

    if (wasWhiteMove) {
        // white move -> increment full-move number, add new item "N. <move>"
        fullMoveNumber++;
        QString line = QString::number(fullMoveNumber) + ". " + notation;
        moveHistoryList->addItem(line);
    } else {
        // black move -> append to last item with some spacing
        int last = moveHistoryList->count() - 1;
        if (last < 0) {
            // defensive: if no white move present, create black-only entry (rare)
            moveHistoryList->addItem(QString("... ") + notation);
        } else {
            QListWidgetItem *it = moveHistoryList->item(last);
            QString text = it->text();
            text += "    " + notation; // 4 spaces between white and black move
            it->setText(text);
        }
    }

    moveHistoryList->scrollToBottom();
}

void MainWindow::highlightMoves(const std::vector<std::pair<int,int>>& moves) {
    clearHighlights();

    for (auto &mv : moves) {
        int r = mv.first;
        int c = mv.second;

        QWidget *overlay = new QWidget(boardButtons[r][c]);
        overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
        overlay->setGeometry(0, 0, 80, 80);

        bool isCapture = (gameBoard.CurrentState[r][c] != EMPTY);

        if (!isCapture) {

            QWidget *dot = new QWidget(overlay);
            dot->setAttribute(Qt::WA_TransparentForMouseEvents);

            dot->setFixedSize(22, 22);                 // perfect dot size
            dot->move((80 - 22) / 2, (80 - 22) / 2);   // perfectly centered

            dot->setStyleSheet(
                "background-color: rgba(0,0,0,50);"     // translucent black
                "border-radius: 11px;"                  // circle
                "border: none;"
                );

            dot->show();
        }
        else {
            // ---- Capture Move: Even translucent ring ----
            overlay->setStyleSheet("background-color: transparent;");
            overlay->setFixedSize(80, 80);
            overlay->move(0, 0);

            // Custom paint event for clean ring
            overlay->installEventFilter(this);
        }

        overlay->show();
        highlightOverlays.push_back(overlay);
    }
}


void MainWindow::clearHighlights() {
    for (auto *w : highlightOverlays) {
        w->deleteLater();
    }
    highlightOverlays.clear();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::Paint) {
        QWidget *w = qobject_cast<QWidget*>(obj);
        if (!w) return false;

        QPainter p(w);
        p.setRenderHint(QPainter::Antialiasing, true);

        int outerRadius = 70;
        int innerRadius = 58;

        QPoint center(w->width() / 2, w->height() / 2);

        // Outer circle (translucent)
        QPen pen(QColor(0, 0, 0, 50));
        pen.setWidth(6);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        // Draw smooth ring
        p.drawEllipse(center, outerRadius/2, outerRadius/2);
    }
    return false;
}



void MainWindow::onEngineMoveReady()
{
    // Engine finished computing
    Move best = engineWatcher->result();

    // Basic validation: ensure returned move is within board (0..7)
    if (best.fromR < 0 || best.fromR > 7 || best.toR < 0 || best.toR > 7) {
        engineThinking = false;
        // restore UI
        QString turnText = isWhiteTurn ? "White's Turn" : "Black's Turn";
        turnLabel->setText(turnText);
        return;
    }

    // Record position key for repetition (use current side to move before engine move)
    std::string posKey = gameBoard.getPositionKey(isWhiteTurn);
    gameBoard.positionCount[posKey]++;

    // Apply engine move to real board (preserve promotion if present in best)
    Move mv = gameBoard.makeMove(best.fromR, best.fromC, best.toR, best.toC,
                                 best.wasPromotion ? best.promotedTo : EMPTY);

    undoStack.push(mv);
    while (!redoStack.empty()) redoStack.pop();

    // Build notation for engine move (mirror of human path)
    QString notation = notationFromMove(mv);

    // Toggle side (engine just moved)
    isWhiteTurn = !isWhiteTurn;

    // Append '+' / '#' if needed for opponent
    if (gameBoard.isCheckmate(isWhiteTurn)) notation += "#";
    else if (gameBoard.isKingInCheck(isWhiteTurn)) notation += "+";

    // Add to history as Black's move (wasWhiteMove = false)
    addMoveToHistory(notation, false);

    // update UI: half-move / 3fold / 50-move checks
    if (gameBoard.positionCount[posKey] >= 3) {
        turnLabel->setText("Draw by Threefold Repetition!");
        updateBoardUI();
        engineThinking = false;
        return;
    }

    if (gameBoard.halfMoveClock >= 100) {
        turnLabel->setText("Draw by Fifty-Move Rule!");
        updateBoardUI();
        engineThinking = false;
        return;
    }

    // Check if game ended after engine move
    bool inCheck = gameBoard.isKingInCheck(isWhiteTurn);
    auto nextMoves = gameBoard.getAllLegalMoves(isWhiteTurn);

    if (nextMoves.empty()) {
        if (inCheck)
            turnLabel->setText(isWhiteTurn ? "Checkmate! Black Wins!" : "Checkmate! White Wins!");
        else
            turnLabel->setText("Draw by Stalemate!");
    } else {
        QString turnText = isWhiteTurn ? "White's Turn" : "Black's Turn";
        if (inCheck) turnText += " (in Check!)";
        turnLabel->setText(turnText);
    }

    updateBoardUI();
    resetColors();
    if (inCheck) highlightKingInCheck(isWhiteTurn);

    engineThinking = false;
}

