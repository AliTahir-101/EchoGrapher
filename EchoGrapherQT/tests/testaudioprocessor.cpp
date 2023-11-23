#include "testaudioprocessor.h"
#include <cstdio>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

TestAudioProcessor::TestAudioProcessor()
{
    // Initialize the AudioProcessor object
    processor = new AudioProcessor(this); // Passing 'this' to set the test case as the parent
    // Flush stderr to ensure all pending error messages are written
    fflush(stderr);

// Save the original stderr file descriptor
#if defined(_WIN32)
    int old_stderr = _dup(_fileno(stderr));
    freopen("nul", "w", stderr);
#else
    int old_stderr = dup(fileno(stderr));
    freopen("/dev/null", "w", stderr);
#endif

    // Initialize PortAudio (or other operations that generate unwanted messages)
    Pa_Initialize(); // Calling Library initialization function

    // Flush stderr again to ensure any error messages are written to nul or /dev/null
    fflush(stderr);

// Restore the original stderr file descriptor
#if defined(_WIN32)
    _dup2(old_stderr, _fileno(stderr));
    _close(old_stderr);
#else
    dup2(old_stderr, fileno(stderr));
    ::close(old_stderr);
#endif
}

TestAudioProcessor::~TestAudioProcessor()
{
    // Cleanup
    Pa_Terminate();
}

void TestAudioProcessor::testAudioInputThreadFunction() {}

void TestAudioProcessor::testAudioProcessingThreadFunction()
{
    // Test the audio processing function
}

void TestAudioProcessor::testSetOutputPath()
{
    processor = new AudioProcessor(this);
    QString path = "some/path";
    QString result = processor->setOutputPath(path);
    QCOMPARE(result, path);

    // Test the default path logic.
    QString defaultResult = processor->setOutputPath("");
    QVERIFY(!defaultResult.isEmpty());
    delete processor;
};

void TestAudioProcessor::testConvertToMelSpectrum()
{
    processor = new AudioProcessor(this);
    // Prepare sample FFT data
    fftwf_complex sampleFFTData[10]; // Just an example size
    // Fill sampleFFTData with test values...

    // Call the method
    QVector<float> melSpectrum = processor->ConvertToMelSpectrum(sampleFFTData, 10, 44100);
    delete processor;
}

void TestAudioProcessor::testCreateMelFilterbank()
{
    processor = new AudioProcessor(this);
    // Call the method
    QVector<QVector<float>> filterbank = processor->CreateMelFilterbank(10, 512, 44100);

    // Verify the filterbank
    // Check that the filterbank size is correct
    QCOMPARE(filterbank.size(), 10);

    // Check that the filters have the correct size and values
    for (const QVector<float> &filter : filterbank)
    {
        QCOMPARE(filter.size(), 512 / 2);
    }
    delete processor;
}

void TestAudioProcessor::testMelFrequencyConversion()
{
    processor = new AudioProcessor(this);
    // Test frequency to Mel conversion
    float frequency = 440.0f; // A4 note
    float mel = processor->FrequencyToMel(frequency);

    // Test Mel to frequency conversion
    float inverseFrequency = processor->MelToFrequency(mel);
    // Check that the inverse conversion returns to the original frequency
    QCOMPARE(inverseFrequency, frequency);
    delete processor;
}

void TestAudioProcessor::testStartProcessing()
{
    processor = new AudioProcessor(); // Instantiate the AudioProcessor
    processor->startProcessing();     // Call startProcessing

    QVERIFY(!processor->stopFlag.load());
    QVERIFY(processor->audioInputThread->isRunning());
    QVERIFY(processor->audioProcessingThread->isRunning());

    processor->stopProcessing();
    delete processor;
}

void TestAudioProcessor::testStopProcessing()
{
    processor = new AudioProcessor(); // Instantiate the AudioProcessor
    processor->startProcessing();     // Call startProcessing

    QVERIFY(!processor->stopFlag.load());
    QVERIFY(processor->audioInputThread->isRunning());
    QVERIFY(processor->audioProcessingThread->isRunning());

    processor->stopProcessing();

    QVERIFY(processor->stopFlag.load());

    delete processor; // Clean up after the test
}

void TestAudioProcessor::testFrequencyToMel()
{
    processor = new AudioProcessor(this);
    // Test with a known frequency to Mel conversion
    float frequency = 440.0f;   // Frequency of the A4 note
    float expectedMel = 548.7f; // Mel value for 440 Hz

    float actualMel = processor->FrequencyToMel(frequency);
    float tolerance = 1.0f; // Define a reasonable tolerance

    QVERIFY(qAbs(actualMel - expectedMel) <= tolerance); // Verify that the actual value is within the tolerance range

    // Test edge cases like zero frequency
    float zeroFrequency = 0.0f;
    float expectedMelForZero = 0.0f; // The Mel value for 0 Hz should be 0
    float actualMelForZero = processor->FrequencyToMel(zeroFrequency);
    QCOMPARE(actualMelForZero, expectedMelForZero);
    delete processor; // Clean up after the test
}