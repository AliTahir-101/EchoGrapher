#ifndef TESTAUDIOPROCESSOR_H
#define TESTAUDIOPROCESSOR_H

#include <QtTest>
#include "../audioprocessor.h"

class TestAudioProcessor : public QObject
{
    Q_OBJECT

public:
    TestAudioProcessor();
    ~TestAudioProcessor();

private:
    AudioProcessor *processor;

private slots:
    void testAudioInputThreadFunction();
    void testSetOutputPath();
    void testAudioProcessingThreadFunction();
    void testCreateMelFilterbank();
    void testMelFrequencyConversion();
    void testConvertToMelSpectrum();
    void testStartProcessing();
    void testStopProcessing();
    void testFrequencyToMel();
};

#endif // TESTAUDIOPROCESSOR_H
