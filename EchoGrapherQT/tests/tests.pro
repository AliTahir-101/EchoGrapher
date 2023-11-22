TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += ../ # Include the path to the main application headers

# Add the sources and headers from the test directory
SOURCES += testmainwindow.cpp
HEADERS += testmainwindow.h

# Link to the QtTest module
QT += testlib
