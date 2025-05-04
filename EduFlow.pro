QT       += core gui
QT       += sql
QT       += network
QT       += charts
QT       += widgets
QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG   += c++17

SOURCES += \
    connection.cpp \
    equipements.cpp \
    main.cpp \
    mainwindow.cpp \
    statistics_window.cpp

HEADERS += \
    connection.h \
    equipements.h \
    mainwindow.h \
    statistics_window.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res/res.qrc
