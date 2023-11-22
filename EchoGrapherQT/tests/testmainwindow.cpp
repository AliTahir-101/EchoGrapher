#include "testmainwindow.h"
#include "qgraphicsview.h"
#include "qpushbutton.h"
#include <QSignalSpy>
#include <QLinearGradient>
#include <QGraphicsRectItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QSlider>
#include <QTimer>
#include <QLabel>
#include <QTest>
#include <iostream>




TestMainWindow::TestMainWindow(QObject *parent)
    : QObject(parent) {
    // Constructor implementation, if needed
}

void TestMainWindow::initTestCase() {
    // Perform global setup for all tests here, if necessary
}

void TestMainWindow::cleanupTestCase() {
    // Perform global cleanup for all tests here, if necessary
}

void TestMainWindow::testStartStopProcessing() {
    MainWindow mainWindow;
    mainWindow.show(); // Necessary to render the UI for interaction

    // Spy on the new signals that are emitted when processing starts and stops
    QSignalSpy startSpy(&mainWindow, &MainWindow::processingStarted);
    QSignalSpy stopSpy(&mainWindow, &MainWindow::processingStopped);

    QTest::mouseClick(mainWindow.findChild<QPushButton*>("startButton"), Qt::LeftButton);
    QApplication::processEvents(); // Process events to ensure signals are dispatched
    QCOMPARE(startSpy.count(), 1); // Verify that processing has started

    QTest::mouseClick(mainWindow.findChild<QPushButton*>("stopButton"), Qt::LeftButton);
    QApplication::processEvents(); // Process events to ensure signals are dispatched
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

void TestMainWindow::testWindowSizeSlider() {
    MainWindow mainWindow;
    mainWindow.show();
    QApplication::processEvents(); // Ensure the UI updates are processed

    // Find the slider and label
    QSlider* windowSizeSlider = mainWindow.findChild<QSlider*>("windowSizeslider");
    QLabel* windowSizeLabel = mainWindow.findChild<QLabel*>("windowSlabel");

    QVERIFY(windowSizeSlider); // Ensure the slider is found
    QVERIFY(windowSizeLabel); // Ensure the label is found

    // Verify the slider's initial properties
    QCOMPARE(windowSizeSlider->value(), 512);
    QCOMPARE(windowSizeSlider->minimum(), 460);
    QCOMPARE(windowSizeSlider->maximum(), 1024);
    QCOMPARE(windowSizeSlider->singleStep(), 1);
    QCOMPARE(windowSizeSlider->pageStep(), 1);
    QCOMPARE(windowSizeSlider->orientation(), Qt::Vertical);

    // Set the slider to a new test value within the valid range
    const int testValue = 600; // Choose a value within the slider's range
    windowSizeSlider->setValue(testValue);
    QApplication::processEvents(); // Ensure the slider movement is processed

    // Check the value on the audioProcessor
    QCOMPARE(mainWindow.audioProcessor->windowSize, testValue);

    // Check the label text
    QCOMPARE(windowSizeLabel->text(), QString("Window Size: %1").arg(testValue));
}

void TestMainWindow::testOverlapSlider() {
    // TODO: implement test
}


void TestMainWindow::testMelBandSlider() {
    MainWindow mainWindow;
    mainWindow.show();
    QApplication::processEvents(); // Ensure the UI updates are processed

    // Find the slider and label
    QSlider* melBandSlider = mainWindow.findChild<QSlider*>("melBandSlider");
    QLabel* melBandFLabel = mainWindow.findChild<QLabel*>("melBandFLabel");

    QVERIFY(melBandSlider); // Ensure the slider is found
    QVERIFY(melBandFLabel); // Ensure the label is found

    // Verify the slider's initial properties
    QCOMPARE(melBandSlider->value(), 25);
    QCOMPARE(melBandSlider->minimum(), 20);
    QCOMPARE(melBandSlider->maximum(), 40);
    QCOMPARE(melBandSlider->singleStep(), 1);
    QCOMPARE(melBandSlider->pageStep(), 1);
    QCOMPARE(melBandSlider->orientation(), Qt::Vertical);

    // Set the slider to a new test value within the valid range
    const int testValue = 30; // Choose a value within the slider's range
    melBandSlider->setValue(testValue);
    QApplication::processEvents(); // Ensure the slider movement is processed

    // Check the value on the audioProcessor
    QCOMPARE(mainWindow.audioProcessor->numMelFilters, testValue);

    // Check the label text
    QCOMPARE(melBandFLabel->text(), QString("Mel Bands: %1").arg(testValue));
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
