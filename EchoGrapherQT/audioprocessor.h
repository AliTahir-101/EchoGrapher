#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <QObject>
#include <QThread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <fftw3.h>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>
#include <QVector>
#include <fstream>
#include <atomic>
#include <portaudio.h>

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

public:
    explicit AudioProcessor(QObject *parent = nullptr);
    ~AudioProcessor();
    QString setOutputPath(const QString &path); // Method to set the output path
    void startProcessing();
    void stopProcessing();

signals:
    void newLogMelSpectrogram(const QVector<float> &spectrum);
    void errorOccurred(const QString &errorMessage); // Signal to report errors

private:
    void audioInputThreadFunction();
    void audioProcessingThreadFunction(uint32_t sampleRate);

    // Function to stop and clean up audio input
    void finalizeAudioInput();
    PaDeviceIndex selectAppropriateDevice();

    QThread *audioInputThread;      // Separate thread for audio input
    QThread *audioProcessingThread; // Separate thread for audio processing
                                    // std::atomic<bool> stopFlag;  // Use std::atomic for thread-safe read/write

    QMutex dataMutex;                 // Mutex for synchronizing access to the dataQueue
    QWaitCondition dataCondition;     // Condition variable for data availability
    QQueue<QVector<float>> dataQueue; // Queue to hold chunks of audio data

    std::atomic<bool> stopFlag{false}; // Ensure it is initialized
    PaStream *paStream;                // Initialize to nullptr

    QString outputPath; // Member variable to hold the output path
    QMutex pathMutex;   // Mutex to protect access to outputPath
    // PaStream* paStream;
    //    QMutex dataMutex;
    //    std::condition_variable dataCondition; // Replaced QWaitCondition with std::condition_variable
    //    std::queue<QVector<float>> dataQueue; // Assuming a std::queue instead of QQueue for C++ standard library use

    // Helper functions
    float FrequencyToMel(float frequency);
    float MelToFrequency(float mel);
    QVector<QVector<float>> CreateMelFilterbank(int numFilters, int fftSize, int sampleRate);
    QVector<float> ConvertToMelSpectrum(fftwf_complex *fftData, int dataSize, int sampleRate);

    void WriteWAVHeader(std::ofstream &file, WAVHeader header);

    // PortAudio variables

    // ... Additional private members and helper functions ...
};

#endif // AUDIOPROCESSOR_H
