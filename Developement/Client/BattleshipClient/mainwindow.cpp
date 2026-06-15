#include "MainWindow.h"
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QJsonArray>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    m_joinButton = new QPushButton("Find new match", this);
    m_joinButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #E67E22;"
        "   color: white;"
        "   font-size: 15px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 5px;"
        "   padding: 12px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #D35400;" // Darker orange on hover
        "}"
    );
    layout->addWidget(m_joinButton);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    layout->addWidget(m_tabWidget);

    setCentralWidget(centralWidget);
    resize(850, 520);

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &MainWindow::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(m_joinButton, &QPushButton::clicked, this, &MainWindow::onJoinButtonClicked);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

    m_socket->connectToHost("127.0.0.1", 12345);
}

MainWindow::~MainWindow() {}

void MainWindow::onConnected() { qDebug() << "Connected to server."; }
void MainWindow::onDisconnected() { qDebug() << "Disconnected."; }

void MainWindow::onJoinButtonClicked() {
    QJsonObject msg;
    msg["action"] = "join_matchmaking";
    sendJsonToServer(msg);
}

void MainWindow::onTabShotFired(const QString &gameId, int x, int y) {
    QJsonObject msg;
    msg["action"] = "shoot";
    msg["game_id"] = gameId;
    msg["x"] = x;
    msg["y"] = y;
    sendJsonToServer(msg);
}

void MainWindow::onReadyRead() {
    while (m_socket->canReadLine()) {
        QByteArray rawData = m_socket->readLine().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(rawData);
        if (!doc.isNull() && doc.isObject()) {
            handleJsonMessage(doc.object());
        }
    }
}

void MainWindow::handleJsonMessage(const QJsonObject &json) {
    QString action = json["action"].toString();
    QString gameId = json["game_id"].toString();

    if (action == "game_started") {
            QString role = json["role"].toString();
            QJsonArray initialShips = json["ships"].toArray();

            GameTab *newTab = new GameTab(gameId, role, this);

            // Save the index where the tab was inserted
            int newTabIndex = m_tabWidget->addTab(newTab, QString("Battle: %1").arg(gameId.left(4)));
            m_gameTabs.insert(gameId, newTab);

            // gets the new active game tab
            m_tabWidget->setCurrentIndex(newTabIndex);

            QColor friendlyShipColor("#2ECC71");
            for (int i = 0; i < initialShips.size(); ++i) {
                QJsonObject coord = initialShips[i].toObject();
                newTab->colorFriendlyShip(coord["x"].toInt(), coord["y"].toInt(), friendlyShipColor);
            }

            connect(newTab, &GameTab::shotFired, this, &MainWindow::onTabShotFired);
        }
    else if (action == "shot_result") {
        if (m_gameTabs.contains(gameId)) {
            m_gameTabs[gameId]->handleShotResult(
                json["x"].toInt(),
                json["y"].toInt(),
                json["hit"].toBool(),
                json["sunk"].toBool(),
                json["sunk_cells"].toArray(),
                json["is_my_shot"].toBool(),
                json["your_turn"].toBool()
            );
        }
    }
    else if (action == "game_over") {
        if (m_gameTabs.contains(gameId)) {
            m_gameTabs[gameId]->handleGameOver(
                json["winner"].toBool(),
                json["reason"].toString()
            );
        }
    }
}

void MainWindow::sendJsonToServer(const QJsonObject &json) {
    QJsonDocument doc(json);
    m_socket->write(doc.toJson(QJsonDocument::Compact) + "\n");
}

void MainWindow::onTabCloseRequested(int index) {
    // Get the widget associated with the clicked tab
    GameTab *tab = qobject_cast<GameTab*>(m_tabWidget->widget(index));
    if (tab) {
        QString gameId = tab->gameId();

        //If game is still active, forfeit game
        if (!tab->isGameOver()) {
            qDebug() << "Active tab closed! Sending forfeit for game:" << gameId;
            QJsonObject forfeitMsg;
            forfeitMsg["action"] = "forfeit";
            forfeitMsg["game_id"] = gameId;
            sendJsonToServer(forfeitMsg);
        }

        // Clean up from memory and UI
        m_gameTabs.remove(gameId);
        m_tabWidget->removeTab(index);
        tab->deleteLater();
    }
}
