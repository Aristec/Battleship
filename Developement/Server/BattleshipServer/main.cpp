#include <QCoreApplication>
#include "NetworkServer.h"
#include <thread>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    NetworkServer server;
    server.startServer();

    qDebug() << "--------------------------------------------------";
    qDebug() << "SERVER IS RUNNING. Press [ENTER] at any time to print live stats!";
    qDebug() << "--------------------------------------------------";

    std::thread inputThread([&server]() {
            std::string input;

            while (std::getline(std::cin, input)) {
                QMetaObject::invokeMethod(&server, &NetworkServer::printStatus, Qt::QueuedConnection);
            }
            qDebug() << "Standard input stream closed or non-interactive. Enter-to-log disabled.";
        });
        inputThread.detach();

    return a.exec();
}
