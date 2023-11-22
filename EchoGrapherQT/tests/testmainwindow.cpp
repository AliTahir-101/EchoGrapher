#include "testmainwindow.h"
#include "qgraphicsview.h"
#include "qpushbutton.h"
#include <QSignalSpy>
#include <QTest>

void TestMainWindow::initTestCase() {
    // Perform global setup for all tests here, if necessary
}

void TestMainWindow::cleanupTestCase() {
    // Perform global cleanup for all tests here, if necessary
}

void TestMainWindow::testStartStopProcessing() {
    MainWindow mainWindow;
    mainWindow.show(); // Necessary to render the UI for interaction

    // Assuming MainWindow emits signals when processing starts and stops
    QSignalSpy startSpy(&mainWindow, &MainWindow::startProcessing);
    QSignalSpy stopSpy(&mainWindow, &MainWindow::stopProcessing);

    QTest::mouseClick(mainWindow.findChild<QPushButton*>("startButton"), Qt::LeftButton);
    QCOMPARE(startSpy.count(), 1); // Verify that processing has started

    QTest::mouseClick(mainWindow.findChild<QPushButton*>("stopButton"), Qt::LeftButton);
    QCOMPARE(stopSpy.count(), 1); // Verify that processing has stopped
}

void TestMainWindow::testSpectrogramUpdates() {
    // This test case will require more information about how spectrogram updates are handled
    // and how to simulate newLogMelSpectrogram signal emissions
}

void TestMainWindow::testSliderAdjustments() {
    MainWindow mainWindow;
    mainWindow.show(); // Necessary to render the UI for interaction

    // Assuming MainWindow emits signals when sliders change
    QSignalSpy windowSizeSpy(&mainWindow, &MainWindow::on_windowSizeslider_valueChanged);
    QSignalSpy melBandSpy(&mainWindow, &MainWindow::on_melBandSlider_valueChanged);
    QSignalSpy overlapSpy(&mainWindow, &MainWindow::on_overlapSlider_valueChanged);

    // Simulate slider value changes and verify the changes through signals or getters
}

void TestMainWindow::testZoomFunctions() {
    MainWindow mainWindow;
    mainWindow.show(); // Necessary to render the QGraphicsView

    // Assuming MainWindow has public slots for zooming
    QGraphicsView* view = mainWindow.findChild<QGraphicsView*>("graphicsView");

    qreal initialScaleFactor = view->transform().m11();
    mainWindow.on_zoomInButton_clicked();
    qreal zoomInScaleFactor = view->transform().m11();
    QVERIFY(zoomInScaleFactor > initialScaleFactor);

    mainWindow.on_zoomOutButton_clicked();
    qreal zoomOutScaleFactor = view->transform().m11();
    QVERIFY(zoomOutScaleFactor < zoomInScaleFactor);

    mainWindow.on_resetZoomButton_clicked();
    qreal resetScaleFactor = view->transform().m11();
    QCOMPARE(resetScaleFactor, 1.0);
}
QTEST_MAIN(TestMainWindow)
