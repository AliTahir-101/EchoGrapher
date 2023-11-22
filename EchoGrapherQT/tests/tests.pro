TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD/../.. # This goes up to the EchoGrapherQT directory
INCLUDEPATH += $$PWD/../../build-EchoGrapherQT-Desktop_Qt_6_6_0_GCC_64bit-Debug # This is the build directory

# Add the sources and headers from the test directory
SOURCES += testmainwindow.cpp \
           ../mainwindow.cpp \
           ../audioprocessor.cpp

HEADERS += testmainwindow.h \
           ../mainwindow.h \
           ../audioprocessor.h

# Link to the Qt modules and any additional libraries
QT += testlib widgets
LIBS += -lportaudio
LIBS += -lfftw3f
