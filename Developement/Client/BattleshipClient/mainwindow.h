#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTabWidget>
#include <QPushButton>
#include <QMap>
#include "GameTab.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onJoinButtonClicked();

    //Routes local grid click signals from a specific child tab up to the server socket
    void onTabShotFired(const QString &gameId, int x, int y);

    //Catches manual tab closures to evaluate and issue server-side forfeits
    void onTabCloseRequested(int index);

private:
    void sendJsonToServer(const QJsonObject &json);
    void handleJsonMessage(const QJsonObject &json);

    QTcpSocket *m_socket;
    QTabWidget *m_tabWidget;
    QPushButton *m_joinButton;

    //Maps asynchronous server session UUIDs directly to dynamic visual tab widgets
    QMap<QString, GameTab*> m_gameTabs;
};

#endif
