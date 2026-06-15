#include "GameSession.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QDebug>

GameSession::GameSession(const QString &gameId, QTcpSocket *player1, QTcpSocket *player2, QObject *parent)
    : QObject(parent), m_gameId(gameId), m_player1(player1), m_player2(player2), m_isGameOver(false)
{
    m_currentTurnPlayer = m_player1;
    initializeRealFleets();

    auto getFleetJson = [](const std::vector<Ship> &fleet) {
        QJsonArray arr;
        for (const auto &ship : fleet) {
            for (const auto &coord : ship.coordinates) {
                QJsonObject c;
                c["x"] = coord.first;
                c["y"] = coord.second;
                arr.append(c);
            }
        }
        return arr;
    };

    // Notify both players with their random ship positions
    QJsonObject notify1;
    notify1["action"] = "game_started";
    notify1["game_id"] = m_gameId;
    notify1["role"] = "player1";
    notify1["ships"] = getFleetJson(m_player1Fleet);
    sendJsonToPlayer(m_player1, notify1);

    QJsonObject notify2;
    notify2["action"] = "game_started";
    notify2["game_id"] = m_gameId;
    notify2["role"] = "player2";
    notify2["ships"] = getFleetJson(m_player2Fleet);
    sendJsonToPlayer(m_player2, notify2);

    qDebug() << "GameSession initialized with dynamic random fleets for ID:" << m_gameId;
}

void GameSession::initializeRealFleets() {
    m_board1.assign(10, std::vector<int>(10, 0));
    m_board2.assign(10, std::vector<int>(10, 0));

    std::vector<std::pair<QString, int>> shipTypes = {
        {"Aircraft Carrier", 5},
        {"Battleship",       4},
        {"Cruiser",          3},
        {"Submarine",        3},
        {"Destroyer",        2}
    };

    // random placement with collision checks
    auto placeRandomly = [&](std::vector<std::vector<int>> &board) {
        std::vector<Ship> fleet;
        for (const auto &type : shipTypes) {
            Ship ship;
            ship.name = type.first;
            ship.size = type.second;
            ship.health = type.second;

            bool placed = false;
            while (!placed) {
                bool horizontal = QRandomGenerator::global()->bounded(2) == 0;
                int x = QRandomGenerator::global()->bounded(10);
                int y = QRandomGenerator::global()->bounded(10);

                // Boundary verification
                if (horizontal && (y + ship.size > 10)) continue;
                if (!horizontal && (x + ship.size > 10)) continue;

                // Collision verification
                bool collision = false;
                for (int i = 0; i < ship.size; ++i) {
                    int nx = horizontal ? x : x + i;
                    int ny = horizontal ? y + i : y;
                    if (board[nx][ny] != 0) {
                        collision = true;
                        break;
                    }
                }

                // Apply placement if safe
                if (!collision) {
                    for (int i = 0; i < ship.size; ++i) {
                        int nx = horizontal ? x : x + i;
                        int ny = horizontal ? y + i : y;
                        board[nx][ny] = 1; // Mark grid occupied
                        ship.coordinates.push_back({nx, ny});
                    }
                    fleet.push_back(ship);
                    placed = true;
                }
            }
        }
        return fleet;
    };

    m_player1Fleet = placeRandomly(m_board1);
    m_player2Fleet = placeRandomly(m_board2);
}

void GameSession::handlePlayerAction(QTcpSocket *sender, const QJsonObject &json) {
    if (m_isGameOver) return;
    QString action = json["action"].toString();

    if (action == "shoot") {
        processShot(sender, json["x"].toInt(), json["y"].toInt());
    }
    // Catch forfeit
    else if (action == "forfeit") {
        qDebug() << "Player forfeited in game:" << m_gameId;
        // awards the other player post forfeit
        handlePlayerDisconnect(sender);
    }
}

void GameSession::processShot(QTcpSocket *shooter, int x, int y) {
    if (shooter != m_currentTurnPlayer) return;

    std::vector<std::vector<int>> &targetBoard = (shooter == m_player1) ? m_board2 : m_board1;
    std::vector<Ship> &targetFleet = (shooter == m_player1) ? m_player2Fleet : m_player1Fleet;
    QTcpSocket *enemy = (shooter == m_player1) ? m_player2 : m_player1;

    // Prevent double shooting at the same tile
    if (targetBoard[x][y] == 2 || targetBoard[x][y] == 3) {
        QJsonObject errorMsg;
        errorMsg["action"] = "error";
        errorMsg["game_id"] = m_gameId;
        errorMsg["message"] = "You have already shot at these coordinates!";
        sendJsonToPlayer(shooter, errorMsg);
        return;
    }

    bool isHit = false;
    bool isSunk = false;
    Ship sunkShip;

    if (targetBoard[x][y] == 1) {
        targetBoard[x][y] = 3; // Hit
        isHit = true;

        for (auto &ship : targetFleet) {
            bool shipHitInThisLoop = false;
            for (const auto &coord : ship.coordinates) {
                if (coord.first == x && coord.second == y) {
                    ship.health--;
                    if (ship.health == 0) {
                        isSunk = true;
                        sunkShip = ship;
                    }
                    shipHitInThisLoop = true;
                    break;
                }
            }
            if (shipHitInThisLoop) break; // Break when matching ship is found
        }
    } else if (targetBoard[x][y] == 0) {
        targetBoard[x][y] = 2; // Miss
    }

    QJsonArray sunkCellsJson;
    if (isSunk) {
        for (const auto &coord : sunkShip.coordinates) {
            QJsonObject c;
            c["x"] = coord.first;
            c["y"] = coord.second;
            sunkCellsJson.append(c);
        }
    }

    auto createMsg = [&](bool isMyShot, bool yourTurn) {
        QJsonObject obj;
        obj["action"] = "shot_result";
        obj["game_id"] = m_gameId;
        obj["x"] = x;
        obj["y"] = y;
        obj["hit"] = isHit;
        obj["sunk"] = isSunk;
        if (isSunk) {
            obj["sunk_ship_name"] = sunkShip.name;
            obj["sunk_cells"] = sunkCellsJson;
        }
        obj["is_my_shot"] = isMyShot;
        obj["your_turn"] = yourTurn;
        return obj;
    };

    if (!isHit) { m_currentTurnPlayer = enemy; }

    sendJsonToPlayer(shooter, createMsg(true, m_currentTurnPlayer == shooter));
    sendJsonToPlayer(enemy, createMsg(false, m_currentTurnPlayer == enemy));

    if (isSunk) {
        checkVictory(shooter, targetFleet);
    }
}

void GameSession::checkVictory(QTcpSocket *shooter, const std::vector<Ship> &targetFleet) {
    bool allSunk = true;
    for (const auto &ship : targetFleet) {
        if (ship.health > 0) { allSunk = false; break; }
    }

    if (allSunk) {
        endGame(shooter, "all_ships_sunk");
    }
}

void GameSession::handlePlayerDisconnect(QTcpSocket *disconnectedPlayer) {
    if (m_isGameOver) return;
    QTcpSocket *winner = (disconnectedPlayer == m_player1) ? m_player2 : m_player1;
    endGame(winner, "opponent_disconnected");
}

void GameSession::endGame(QTcpSocket *winner, const QString &reason) {
    m_isGameOver = true;
    QJsonObject msg;
    msg["action"] = "game_over";
    msg["game_id"] = m_gameId;
    msg["reason"] = reason;

    msg["winner"] = true;
    sendJsonToPlayer(winner, msg);

    msg["winner"] = false;
    QTcpSocket *loser = (winner == m_player1) ? m_player2 : m_player1;
    sendJsonToPlayer(loser, msg);
}

void GameSession::sendJsonToPlayer(QTcpSocket *player, const QJsonObject &json) {
    QJsonDocument doc(json);
    player->write(doc.toJson(QJsonDocument::Compact) + "\n");
}
