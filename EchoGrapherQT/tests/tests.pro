TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += ../ # Include the path to the main application headers

# Add the sources and headers from the test directory
SOURCES += testmainwindow.cpp \
           ../mainwindow.cpp
HEADERS += testmainwindow.h \
           ../mainwindow.h


# Link to the Qt modules
QT += testlib widgets