#include "ConsoleReader.h"
#include <iostream>
#include <string>

void ConsoleReader::run() {
    std::string line;
    while (true) {
        //block the background thread, but keeps the main network thread alive
        if (std::getline(std::cin, line)) {
            emit inputReceived();
        }
    }
}
