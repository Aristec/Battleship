#ifndef NETWORKSERVER_H
#define NETWORKSERVER_H

#include <QTcpServer>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include "GameSession.h"

class QTcpSocket;

class NetworkServer : public QTcpServer {
    Q_OBJECT
public:
    NetworkServer(QObject *parent = nullptr);
    void startServer();
    void printStatus() const;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();
    void onClientDisconnected();

private:
    void handleJsonMessage(QTcpSocket *client, const QJsonObject &json);
    void sendJsonToClient(QTcpSocket *client, const QJsonObject &json);
    void startNewGame(QTcpSocket *p1, QTcpSocket *p2);

    QList<QTcpSocket*> m_clients;
    QList<QTcpSocket*> m_matchmakingQueue;
    QMap<QString, GameSession*> m_activeGames;    // game_id -> GameSession mapping
};

#endif
