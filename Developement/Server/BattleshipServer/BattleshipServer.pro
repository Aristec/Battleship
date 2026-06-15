QT += core network
QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

SOURCES += \
        consolereader.cpp \
        gamesession.cpp \
        main.cpp \
        networkserver.cpp

TRANSLATIONS += \
    BattleshipServer_en_150.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    consolereader.h \
    gamesession.h \
    networkserver.h
