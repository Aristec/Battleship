#ifndef GAMETAB_H
#define GAMETAB_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QJsonArray>

class GameTab : public QWidget {
    Q_OBJECT
public:
    GameTab(const QString &gameId, const QString &role, QWidget *parent = nullptr);
    QString gameId() const { return m_gameId; }

    // sunkCells contains all coordinates of the ship only when sunk = true
    void handleShotResult(int x, int y, bool hit, bool sunk, const QJsonArray &sunkCells, bool isMyShot, bool yourTurn);
    void handleGameOver(bool winner, const QString &reason);
    void colorFriendlyShip(int x, int y, const QColor &color);
    bool isGameOver() const { return m_isGameOver; }

signals:
    // Decoupling: Emitted to MainWindow to handle network serialization and sending
    void shotFired(const QString &gameId, int x, int y);

private slots:
    void onEnemyCellClicked(int row, int column);

private:
    void setupBoard(QTableWidget *board);

    QString m_gameId;
    QString m_role;
    bool m_isMyTurn;

    // Checked during tab closure to differentiate between a forfeit and a normal exit
    bool m_isGameOver;

    QTableWidget *m_myBoard;
    QTableWidget *m_enemyBoard;
    QLabel *m_statusLabel;
};

#endif
