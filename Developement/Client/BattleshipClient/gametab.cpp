#include "GameTab.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QJsonObject>
#include <QJsonArray>

GameTab::GameTab(const QString &gameId, const QString &role, QWidget *parent)
    : QWidget(parent), m_gameId(gameId), m_role(role)
{
    m_isMyTurn = (m_role == "player1");
    m_isGameOver = false;
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_statusLabel = new QLabel(m_isMyTurn ? "YOUR TURN: Take a shot!" : "ENEMY TURN: Waiting...", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "   color: #2C3E50;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   background-color: #ECF0F1;"
        "   padding: 8px;"
        "   border-radius: 4px;"
        "}"
    );
    mainLayout->addWidget(m_statusLabel);

    QHBoxLayout *boardsLayout = new QHBoxLayout();
    boardsLayout->setAlignment(Qt::AlignCenter);

    // My Board setup
    QVBoxLayout *myLayout = new QVBoxLayout();
    myLayout->setAlignment(Qt::AlignCenter);
    QLabel *myLabel = new QLabel("My Fleet (Defense Zone)", this);
    myLabel->setStyleSheet("font-weight: bold; color: #27AE60;");
    myLayout->addWidget(myLabel, 0, Qt::AlignCenter);

    m_myBoard = new QTableWidget(10, 10, this);
    setupBoard(m_myBoard);
    myLayout->addWidget(m_myBoard);

    // Enemy Board setup
    QVBoxLayout *enemyLayout = new QVBoxLayout();
    enemyLayout->setAlignment(Qt::AlignCenter);
    QLabel *enemyLabel = new QLabel("Enemy Radar (Attack Zone)", this);
    enemyLabel->setStyleSheet("font-weight: bold; color: #C0392B;");
    enemyLayout->addWidget(enemyLabel, 0, Qt::AlignCenter);

    m_enemyBoard = new QTableWidget(10, 10, this);
    setupBoard(m_enemyBoard);
    enemyLayout->addWidget(m_enemyBoard);

    boardsLayout->addLayout(myLayout);
    boardsLayout->addLayout(enemyLayout);
    mainLayout->addLayout(boardsLayout);

    connect(m_enemyBoard, &QTableWidget::cellClicked, this, &GameTab::onEnemyCellClicked);
}

void GameTab::setupBoard(QTableWidget *board) {
    board->setColumnCount(10);
    board->setRowCount(10);
    board->verticalHeader()->setFixedWidth(45);
    board->horizontalHeader()->setFixedHeight(30);
    board->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    board->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    board->horizontalHeader()->setDefaultSectionSize(36);
    board->verticalHeader()->setDefaultSectionSize(36);
    board->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    board->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);

    // Explicitly calculate sizes to prevent OS-specific padding issues (e.g. Windows 11).
    // length() aggregates total cell sizes dynamically before the widget is fully rendered.
    int totalWidth = board->horizontalHeader()->length() + 45 + (board->frameWidth() * 2);
    int totalHeight = board->verticalHeader()->length() + 30 + (board->frameWidth() * 2);

    board->setFixedSize(totalWidth, totalHeight);
    board->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    board->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    board->setEditTriggers(QAbstractItemView::NoEditTriggers);
    board->setSelectionMode(QAbstractItemView::NoSelection);

    QStringList columnHeaders;
    for (char c = 'A'; c <= 'J'; ++c) columnHeaders << QString(c);
    board->setHorizontalHeaderLabels(columnHeaders);

    QStringList rowHeaders;
    for (int i = 1; i <= 10; ++i) rowHeaders << QString::number(i);
    board->setVerticalHeaderLabels(rowHeaders);

    board->setStyleSheet(
        "QTableWidget {"
        "   gridline-color: #BDC3C7;"
        "   border: 2px solid #34495E;"
        "}"
        "QHeaderView::section {"
        "   background-color: #34495E;"
        "   color: white;"
        "   font-weight: bold;"
        "   border: 1px solid #2C3E50;"
        "}"
    );

    QColor oceanBlue("#2980B9");
    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < 10; ++c) {
            board->setItem(r, c, new QTableWidgetItem());
            board->item(r, c)->setBackground(oceanBlue);
        }
    }
}

void GameTab::colorFriendlyShip(int x, int y, const QColor &color) {
    if (m_myBoard->item(x, y)) {
        m_myBoard->item(x, y)->setBackground(color);
    }
}

void GameTab::onEnemyCellClicked(int row, int column) {
    if (!m_isMyTurn) return; // Prevent out-of-turn shooting attempts at the UI level
    emit shotFired(m_gameId, row, column);
}

void GameTab::handleShotResult(int x, int y, bool hit, bool sunk, const QJsonArray &sunkCells, bool isMyShot, bool yourTurn) {
    m_isMyTurn = yourTurn;

    if (m_isMyTurn) {
        m_statusLabel->setText("YOUR TURN: Take a shot!");
        m_statusLabel->setStyleSheet("color: white; font-weight: bold; background-color: #27AE60; padding: 8px; border-radius: 4px;");
    } else {
        m_statusLabel->setText("ENEMY TURN: Waiting for opponent...");
        m_statusLabel->setStyleSheet("color: white; font-weight: bold; background-color: #7F8C8D; padding: 8px; border-radius: 4px;");
    }

    QTableWidget *targetBoard = isMyShot ? m_enemyBoard : m_myBoard;

    if (targetBoard->item(x, y)) {
        if (hit) {
            targetBoard->item(x, y)->setBackground(QColor("#E74C3C"));
        } else {
            targetBoard->item(x, y)->setBackground(QColor("#ECF0F1"));
        }
    }

    if (sunk) {
        QColor sunkColor("#2C3E50");
        // Re-render all coordinates of the destroyed ship into a single visual block
        for (int i = 0; i < sunkCells.size(); ++i) {
            QJsonObject coord = sunkCells[i].toObject();
            int sx = coord["x"].toInt();
            int sy = coord["y"].toInt();
            if (targetBoard->item(sx, sy)) {
                targetBoard->item(sx, sy)->setBackground(sunkColor);
                targetBoard->item(sx, sy)->setText("X");
                targetBoard->item(sx, sy)->setTextAlignment(Qt::AlignCenter);
                targetBoard->item(sx, sy)->setForeground(Qt::white); // Modern non-deprecated replacement for setTextColor
            }
        }
        m_statusLabel->setText(QString("SHIP SUNK! %1 went down!").arg(isMyShot ? "An enemy ship" : "Your ship"));
    }
}

void GameTab::handleGameOver(bool winner, const QString &reason) {
    m_isMyTurn = false;
    m_enemyBoard->setEnabled(false);
    m_isGameOver = true;

    if (winner) {
        m_statusLabel->setText(QString("VICTORY! Winner by: %1").arg(reason == "all_ships_sunk" ? "Fleet Destroyed!" : "Opponent Left!"));
        m_statusLabel->setStyleSheet("color: white; font-weight: bold; background-color: #D4AF37; border: 2px solid #AA7C11; padding: 10px; border-radius: 4px;");
    } else {
        m_statusLabel->setText("DEFEAT! Your ships were upgraded to submarines.");
        m_statusLabel->setStyleSheet("color: white; font-weight: bold; background-color: #7B241C; padding: 10px; border-radius: 4px;");
    }
}
