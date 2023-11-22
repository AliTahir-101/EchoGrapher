#ifndef TESTMAINWINDOW_H
#define TESTMAINWINDOW_H

#include <QtTest>
#include "../mainwindow.h"

class TestMainWindow : public QObject
{
    Q_OBJECT
public:
    explicit TestMainWindow(QObject *parent = nullptr);

private slots:
    void testStartStopProcessing(); // Tests the start and stop functionality
    void testSpectrogramUpdates();  // Tests the spectrogram update mechanism
    void testZoomFunctions();       // Tests the zoom in, zoom out, and reset zoom functionality
    void testWindowSizeSlider();
    void testMelBandSlider();
    void testOverlapSlider();
    void testSelectOutputPath();
};

#endif // TESTMAINWINDOW_H
