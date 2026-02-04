#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <stack>
#include <QListWidget>
#include <utility>
#include "board.h"
#include "engine.h"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void highlightMoves(const std::vector<std::pair<int,int>>& moves);
    std::vector<QWidget*> highlightOverlays;
    void clearHighlights();
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;


private slots:
    void handleTileClick();  // when a tile is clicked
    void undoMove();
    void redoMove();
    void onEngineMoveReady();

private:
    QPushButton* boardButtons[8][8];  // 2D grid of buttons
    QLabel *turnLabel = NULL;
    board gameBoard;                  // Your board object

    std::stack<Move> undoStack;
    std::stack<Move> redoStack;

    bool pieceSelected = false; // Is a piece currently selected?
    int selectedRow = -1;
    int selectedCol = -1;
    bool isWhiteTurn = true; // true = White's turn, false = Black's

    void updateBoardUI();             // Sync board state → UI
    void highlightKingInCheck(bool isWhiteTurn);
    void resetColors();

    void addMoveToHistory(const QString &notation, bool wasWhiteMove);
    QString notationFromMove(const Move &mv);

    // show promotion dialog and return chosen Piece (WQ/WR/WB/WN or BQ/BR/BB/BN)
    Piece showPromotionDialog(bool whiteSide);

    QListWidget *moveHistoryList = nullptr;
    int fullMoveNumber = 0;   // 1-based full-move number


    Engine chessEngine;
    QFutureWatcher<Move> *engineWatcher = nullptr; // watcher for async engine run
    int engineDepth = 5;                      // engine search depth (tuneable)
    bool engineThinking = false;              // true while engine is thinking

    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
