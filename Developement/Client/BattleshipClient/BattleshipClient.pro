QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    gametab.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    gametab.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    BattleshipClient_en_150.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
