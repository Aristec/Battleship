#ifndef CONSOLEREADER_H
#define CONSOLEREADER_H

#include <QThread>

class ConsoleReader : public QThread {
    Q_OBJECT
signals:
    void inputReceived(); // Emitted whenever user presses Enter

protected:
    void run() override; // The background loop
};

#endif
