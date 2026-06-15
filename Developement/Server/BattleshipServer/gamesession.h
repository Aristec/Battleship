#ifndef GAMESESSION_H
#define GAMESESSION_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>

class QTcpSocket;

struct Ship {
    QString name;
    int size;
    int health;
    std::vector<std::pair<int, int>> coordinates;
};

class GameSession : public QObject {
    Q_OBJECT
public:
    GameSession(const QString &gameId, QTcpSocket *player1, QTcpSocket *player2, QObject *parent = nullptr);

    QString gameId() const { return m_gameId; }
    void handlePlayerAction(QTcpSocket *sender, const QJsonObject &json);
    void handlePlayerDisconnect(QTcpSocket *disconnectedPlayer);

private:
    void processShot(QTcpSocket *shooter, int x, int y);
    void sendJsonToPlayer(QTcpSocket *player, const QJsonObject &json);
    void initializeRealFleets();
    void checkVictory(QTcpSocket *shooter, const std::vector<Ship> &targetFleet);
    void endGame(QTcpSocket *winner, const QString &reason);

    QString m_gameId;
    QTcpSocket *m_player1;
    QTcpSocket *m_player2;
    QTcpSocket *m_currentTurnPlayer;
    bool m_isGameOver;

    // 10x10 boards
    // 0 = Empty, 2 = Miss, 3 = Hit, 1 = hidden
    std::vector<std::vector<int>> m_board1;
    std::vector<std::vector<int>> m_board2;

    // Fleet tracking
    std::vector<Ship> m_player1Fleet;
    std::vector<Ship> m_player2Fleet;
};

#endif
