#include <QTest>
#include "testaudioprocessor.h"
#include "testmainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    int status = 0;

    TestAudioProcessor testAudioProcessor;
    status |= QTest::qExec(&testAudioProcessor, argc, argv);

    TestMainWindow testMainWindow;
    status |= QTest::qExec(&testMainWindow, argc, argv);

    return status;
}
