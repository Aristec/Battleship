#include "NetworkServer.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QDebug>

NetworkServer::NetworkServer(QObject *parent) : QTcpServer(parent) {}

void NetworkServer::startServer() {
    if (this->listen(QHostAddress::Any, 12345)) {
        qDebug() << "Server started on port 12345";
    } else {
        qDebug() << "Server failed to start:" << this->errorString();
    }
}

void NetworkServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket *clientSocket = new QTcpSocket(this);
    clientSocket->setSocketDescriptor(socketDescriptor);

    connect(clientSocket, &QTcpSocket::readyRead, this, &NetworkServer::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &NetworkServer::onClientDisconnected);

    m_clients.append(clientSocket);
    qDebug() << "Client connected. Total clients:" << m_clients.size();
}

void NetworkServer::onReadyRead() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    while (clientSocket->canReadLine()) {
        QByteArray rawData = clientSocket->readLine().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(rawData);

        if (!doc.isNull() && doc.isObject()) {
            handleJsonMessage(clientSocket, doc.object());
        }
    }
}

void NetworkServer::handleJsonMessage(QTcpSocket *client, const QJsonObject &json) {
    QString action = json["action"].toString();
    QString gameId = json["game_id"].toString();

    // Route message directly to game session if it belongs to an ongoing game
    if (!gameId.isEmpty() && m_activeGames.contains(gameId)) {
        m_activeGames[gameId]->handlePlayerAction(client, json);
        return;
    }

    // Handle global server actions
    if (action == "join_matchmaking") {
        if (!m_matchmakingQueue.contains(client)) {
            m_matchmakingQueue.append(client);
            qDebug() << "Player added to matchmaking queue. Queue size:" << m_matchmakingQueue.size();

            QJsonObject response;
            response["action"] = "status_update";
            response["message"] = "Searching for an opponent...";
            sendJsonToClient(client, response);
        }

        //If 2 players are waiting, start a game
        if (m_matchmakingQueue.size() >= 2) {
            QTcpSocket *p1 = m_matchmakingQueue.takeFirst();
            QTcpSocket *p2 = m_matchmakingQueue.takeFirst();
            startNewGame(p1, p2);
        }
    }
}

void NetworkServer::startNewGame(QTcpSocket *p1, QTcpSocket *p2) {
    QString gameId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    GameSession *session = new GameSession(gameId, p1, p2, this);
    m_activeGames.insert(gameId, session);

    qDebug() << "Successfully matched 2 players into game:" << gameId;
}

void NetworkServer::sendJsonToClient(QTcpSocket *client, const QJsonObject &json) {
    QJsonDocument doc(json);
    client->write(doc.toJson(QJsonDocument::Compact) + "\n");
}

void NetworkServer::onClientDisconnected() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    m_clients.removeAll(clientSocket);
    m_matchmakingQueue.removeAll(clientSocket);

    // Look for active games containing this socket and trigger disconnect logic
    QList<QString> gamesToRemove;
    for (auto it = m_activeGames.begin(); it != m_activeGames.end(); ++it) {
        GameSession *session = it.value();
        //Trigger the cleanup inside the session
        session->handlePlayerDisconnect(clientSocket);
        gamesToRemove.append(it.key());
    }

    for (const QString &id : gamesToRemove) {
        m_activeGames.remove(id);
    }

    clientSocket->deleteLater();
    qDebug() << "Client disconnected cleanly. Remaining active games:" << m_activeGames.size();
}

void NetworkServer::printStatus() const {
    qDebug() << "\n=========================================";
    qDebug() << "           SERVER STATUS REPORT          ";
    qDebug() << "=========================================";
    qDebug() << "Connected Clients:" << m_clients.size();
    qDebug() << "Players in Queue: " << m_matchmakingQueue.size();
    qDebug() << "Active Game Rooms:" << m_activeGames.size();

    if (!m_activeGames.isEmpty()) {
        qDebug() << "-----------------------------------------";
        qDebug() << "Active Game IDs:";
        for (auto it = m_activeGames.begin(); it != m_activeGames.end(); ++it) {
            qDebug() << " -> ID:" << it.key();
        }
    }
    qDebug() << "=========================================\n";
}
