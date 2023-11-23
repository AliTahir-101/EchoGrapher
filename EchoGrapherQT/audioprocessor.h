#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <fftw3.h>
#include <portaudio.h>

#include <mutex>
#include <queue>
#include <vector>
#include <atomic>
#include <QMutex>
#include <QQueue>
#include <fstream>
#include <QObject>
#include <QThread>
#include <QVector>
#include <QWaitCondition>
#include <condition_variable>

struct WAVHeader
{
    char chunkID[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunkSize; // Size of the entire file in bytes minus 8 bytes
    char format[4] = {'W', 'A', 'V', 'E'};
    char subchunk1ID[4] = {'f', 'm', 't', ' '};
    uint32_t subchunk1Size = 16; // PCM header size
    uint16_t audioFormat = 3;    // PCM = 1 | 3 - IEEE float
    uint16_t numChannels;        // Mono = 1
    uint32_t sampleRate;
    uint32_t byteRate;           // sampleRate * numChannels * bitsPerSample/8
    uint16_t blockAlign;         // numChannels * bitsPerSample/8
    uint16_t bitsPerSample = 32; // 8 bits = 8, 16 bits = 16, etc.
    char subchunk2ID[4] = {'d', 'a', 't', 'a'};
    uint32_t subchunk2Size; // numSamples * numChannels * bitsPerSample/8
};

class AudioProcessor : public QObject
{
    Q_OBJECT
    friend class TestAudioProcessor;

public:
    int numMelFilters = 25;
    int windowSize = 512;      // window size
    float windowOverlap = 0.5; // 50% overlap

    explicit AudioProcessor(QObject *parent = nullptr);
    ~AudioProcessor();

    QString setOutputPath(const QString &path); // Method to set the output path
    void startProcessing();
    void stopProcessing();

signals:
    void newLogMelSpectrogram(const QVector<float> &spectrum);
    void errorOccurred(const QString &errorMessage); // Signal to report errors

private:
    PaStream *paStream;                // Initialize to nullptr
    std::atomic<bool> stopFlag{false}; // Ensure it is initialized
    QThread *audioInputThread;         // Separate thread for audio input
    QThread *audioProcessingThread;    // Separate thread for audio processing

    QMutex dataMutex;                 // Mutex for synchronizing access to the dataQueue
    QWaitCondition dataCondition;     // Condition variable for data availability
    QQueue<QVector<float>> dataQueue; // Queue to hold chunks of audio data

    QString outputPath; // Member variable to hold the output path
    QMutex pathMutex;   // Mutex to protect access to outputPath

    void audioInputThreadFunction();
    void audioProcessingThreadFunction(uint32_t sampleRate);

    // Helper functions
    QVector<QVector<float>> CreateMelFilterbank(int numFilters, int fftSize, int sampleRate);
    QVector<float> ConvertToMelSpectrum(fftwf_complex *fftData, int dataSize, int sampleRate);
    float FrequencyToMel(float frequency);
    float MelToFrequency(float mel);
    void WriteWAVHeader(std::ofstream &file, WAVHeader header);
};

#endif // AUDIOPROCESSOR_H
