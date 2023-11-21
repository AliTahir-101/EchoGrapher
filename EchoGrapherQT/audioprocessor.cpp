#include "audioprocessor.h"
#include <portaudio.h>
#include <QFile>
#include <QDataStream>
#include <iostream>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>

using namespace std;

AudioProcessor::AudioProcessor(QObject *parent)
    : QObject(parent),
      audioInputThread(nullptr),
      audioProcessingThread(nullptr),
      stopFlag(false),
      paStream(nullptr) {}

AudioProcessor::~AudioProcessor() {}

void AudioProcessor::stopProcessing()
{

    cout << "STop it ......." << endl;
    stopFlag.store(true);

    if (audioInputThread)
    {
        audioInputThread->quit(); // Request the thread to stop
        audioInputThread->wait(); // Wait for the thread to finish
        delete audioInputThread;  // Clean up the thread
        audioInputThread = nullptr;
    }

    if (audioProcessingThread)
    {
        audioProcessingThread->quit(); // Request the thread to stop
        audioProcessingThread->wait(); // Wait for the thread to finish
        delete audioProcessingThread;  // Clean up the thread
        audioProcessingThread = nullptr;
    }
}

void AudioProcessor::startProcessing()
{

    cout << "Start it ..." << endl;
    stopFlag.store(false);

    // Start the audio input thread
    audioInputThread = QThread::create([this]
                                       { this->audioInputThreadFunction(); });
    connect(audioInputThread, &QThread::finished, audioInputThread, &QObject::deleteLater);
    audioInputThread->start();

    uint32_t actualSampleRate = 44100;
    audioProcessingThread = QThread::create([this, actualSampleRate]
                                            { this->audioProcessingThreadFunction(actualSampleRate); });
    connect(audioProcessingThread, &QThread::finished, audioProcessingThread, &QObject::deleteLater);
    audioProcessingThread->start();
}

void AudioProcessor::audioInputThreadFunction()
{
    PaError err = paNoError;
    PaStreamParameters inputParameters;
    const PaDeviceInfo *deviceInfo;
    inputParameters.device = Pa_GetDefaultInputDevice();

    deviceInfo = Pa_GetDeviceInfo(inputParameters.device);

    // Check and set the sample rate based on device capability
    uint32_t desiredSampleRate = 44100;
    uint32_t actualSampleRate = min(deviceInfo->defaultSampleRate, static_cast<double>(desiredSampleRate));

    inputParameters.channelCount = 1;         // Mono input
    inputParameters.sampleFormat = paFloat32; // 32-bit floating point input
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(
        &paStream,
        &inputParameters,
        nullptr,          // No output parameters for recording only
        actualSampleRate, // Sample rate
        windowSize,              // Frames per buffer
        paClipOff,        // We won't output out-of-range samples so don't bother clipping them
        nullptr,          // No callback, use blocking API
        nullptr);         // No data for the callback since we're not using one
    if (err != paNoError)
    {
        emit errorOccurred(QString("PortAudio error: open stream: %1").arg(Pa_GetErrorText(err)));
        return; // Stop the function if stream opening fails
    }

    // Constants for Hanning window and frame processing
//    const int windowSize = 512 by default;
    QVector<float> window(windowSize);

    // Hanning window
    for (int i = 0; i < windowSize; ++i)
    {
        window[i] = 0.5 * (1 - cos(2 * M_PI * i / (windowSize - 1)));
    }

    // Prepare the WAVHeader for 32-bit float format
    WAVHeader header;
    header.numChannels = inputParameters.channelCount;
    header.sampleRate = actualSampleRate;
    header.bitsPerSample = 32; // For 32-bit float data
    header.audioFormat = 3;    // IEEE float
    header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8;
    header.blockAlign = header.numChannels * header.bitsPerSample / 8;

    // Initialize file for writing
    QString localOutputPath;
    {
        QMutexLocker locker(&pathMutex); // Lock the mutex before reading the path
        // Get the current date and time
        QString dateTimeStr = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString filename = "/output_" + dateTimeStr + ".wav";
        localOutputPath = outputPath + filename;
        locker.unlock(); // Unlock the mutex
    }

    QFile file(localOutputPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        emit errorOccurred("Error: Could not open file for writing.");
        return;
    }

    // Placeholder for header, we'll write the actual header later
    file.write(reinterpret_cast<const char *>(&header), sizeof(WAVHeader));

    Pa_StartStream(paStream);
    QVector<float> audioChunk(windowSize); // Temporary buffer to hold the audio chunk
    while (!stopFlag.load())
    {
        err = Pa_ReadStream(paStream, audioChunk.data(), windowSize);
        if (err)
        {
            emit errorOccurred(QString("PortAudio error: read stream: %1").arg(Pa_GetErrorText(err)));
            break;
        }
        else
        {
            // Write float data directly to file
            file.write(reinterpret_cast<const char *>(audioChunk.constData()), audioChunk.size() * sizeof(float));
            // Lock mutex before accessing shared data
            QMutexLocker locker(&dataMutex); // Lock the mutex
            dataQueue.enqueue(audioChunk);   // Push the data into the queue
            locker.unlock();                 // Unlock the mutex
            dataCondition.wakeOne();         // Wake one waiting thread
        }
        audioChunk.fill(0); // Reset all values to 0 without changing the size
    }
    Pa_CloseStream(paStream);

    // Finalize WAV header and file
    uint32_t fileDataSize = static_cast<uint32_t>(file.size() - sizeof(WAVHeader));
    header.subchunk2Size = fileDataSize;
    header.chunkSize = 36 + header.subchunk2Size;

    // Go back and update the header with the correct sizes
    file.seek(0);
    file.write(reinterpret_cast<const char *>(&header), sizeof(WAVHeader));
    file.close();
}

QString AudioProcessor::setOutputPath(const QString &path)
{
    QMutexLocker locker(&pathMutex);
    if (path.isEmpty())
    {
        // Retrieve the user's home directory based on the operating system
        QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

// For Windows, specifically want the C drive, i beleive can do something like this:
#ifdef Q_OS_WIN
        homePath = "C:/";
#endif

        // Append the specific folder structure you want to use
        QString defaultPath = QDir::cleanPath(homePath + "/EchoGrapher/output");

        // Ensure the directory exists or create it
        QDir dir;
        if (!dir.exists(defaultPath))
        {
            dir.mkpath(defaultPath);
        }

        outputPath = defaultPath; // Set the default file name
    }
    else
    {
        outputPath = path;
    }
    locker.unlock();
    return outputPath;
}

void AudioProcessor::audioProcessingThreadFunction(uint32_t sampleRate)
{

    // Variables for FFTW
    fftwf_complex *in, *out;
    fftwf_plan plan_forward;
    QVector<float> window(windowSize);

    // Initialize Hanning window
    for (int i = 0; i < windowSize; ++i)
    {
        window[i] = 0.5 * (1 - cos(2 * M_PI * i / (windowSize - 1)));
    }

    in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * windowSize);
    out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * windowSize);
    if (!in || !out)
    {
        emit errorOccurred("Failed to allocate memory for FFTW.");
        fftwf_free(in);
        fftwf_free(out);
        fftwf_cleanup();
        return;
    }

    plan_forward = fftwf_plan_dft_1d(windowSize, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    if (!plan_forward)
    {
        emit errorOccurred(tr("Error: FFTW plan creation failed."));
        fftwf_free(in);
        fftwf_free(out);
        fftwf_cleanup();
        return;
    }

    // Initialize these as member variables or parameters

    int overlapSamples = static_cast<int>(windowSize * windowOverlap);
    int hopSize = windowSize - overlapSamples;

    QVector<float> audioBuffer; // This buffer will hold a large enough sample of audio to apply the window and overlap
    while (!stopFlag.load())
    { // Use load() to read the atomic variable

        QMutexLocker locker(&dataMutex); // Lock the mutex
        while (dataQueue.empty() && !stopFlag.load())
        {
            dataCondition.wait(&dataMutex); // Wait for the condition
        }
        if (stopFlag.load())
        {
            break;
        }
        audioBuffer.append(dataQueue.dequeue()); // Dequeue the chunk and append to buffer
        locker.unlock();                  // Unlock the mutex

        while (audioBuffer.size() >= windowSize)  {
            if (stopFlag.load()) {break;}
            // Now process the audioBuffer from index 0 to windowSize
            for (int i = 0; ((i < windowSize) && !stopFlag.load()); ++i) {
                in[i][0] = audioBuffer[i] * window[i]; // Apply window function
                in[i][1] = 0.0;
            }

            fftwf_execute(plan_forward);

            // Convert the FFT data to the Mel spectrum
            QVector<float> melSpectrum = ConvertToMelSpectrum(out, windowSize, sampleRate);
            emit newLogMelSpectrogram(melSpectrum);
            // Remove the processed frame considering the overlap
            audioBuffer.remove(0, hopSize);
        }
    }

    // Clean up FFTW resources
    fftwf_destroy_plan(plan_forward);
    fftwf_free(in);
    fftwf_free(out);
    fftwf_cleanup();
}

QVector<float> AudioProcessor::ConvertToMelSpectrum(fftwf_complex *fftData, int dataSize, int sampleRate)
{
    // DataSize is even and greater than 0 to avoid division by zero
    if (dataSize <= 0 || dataSize % 2 != 0)
    {
        emit errorOccurred("Invalid dataSize provided to ConvertToMelSpectrum.");
        return {};
    }

    // Compute the Mel filterbank
    QVector<QVector<float>> melFilterbank = CreateMelFilterbank(numMelFilters, dataSize, sampleRate);

    // Initialize the Mel spectrum vector
    QVector<float> melSpectrum(numMelFilters, 0.0f);

    // Convert each FFT bin to power
    QVector<float> powerSpectrum(dataSize / 2);
    for (int i = 0; i < dataSize / 2; ++i)
    {
        powerSpectrum[i] = fftData[i][0] * fftData[i][0] + fftData[i][1] * fftData[i][1];
    }

    // Apply the filterbank to the power spectrum
    for (int filterNum = 0; filterNum < numMelFilters; ++filterNum)
    {
        float melEnergy = 0.0f;
        for (int bin = 0; bin < dataSize / 2; ++bin)
        {
            melEnergy += powerSpectrum[bin] * melFilterbank[filterNum][bin];
        }
        melSpectrum[filterNum] = melEnergy; // Adding a small value to prevent log(0) can be done if log is taken later
    }

    return melSpectrum;
}

QVector<QVector<float>> AudioProcessor::CreateMelFilterbank(int numFilters, int fftSize, int sampleRate)
{
    QVector<QVector<float>> filterbank;
    filterbank.reserve(numFilters);

    // Compute the Mel frequency limits
    const float lowerMelFreq = FrequencyToMel(0);
    const float upperMelFreq = FrequencyToMel(sampleRate / 2);
    const float melStep = (upperMelFreq - lowerMelFreq) / (numFilters + 1);

    QVector<float> melPoints;
    melPoints.reserve(numFilters + 2);

    QVector<float> binFrequencies;
    binFrequencies.reserve(numFilters + 2);

    QVector<int> bins;
    bins.reserve(numFilters + 2);

    for (int i = 0; i <= numFilters + 1; ++i)
    {
        float melPoint = lowerMelFreq + i * melStep;
        melPoints.push_back(melPoint);
        binFrequencies.push_back(MelToFrequency(melPoint));
        bins.push_back(static_cast<int>(floor((fftSize + 1) * binFrequencies.last() / sampleRate)));
    }

    for (int i = 1; i <= numFilters; ++i)
    {
        QVector<float> filter(fftSize / 2, 0.0f);
        int startBin = bins[i - 1];
        int centerBin = bins[i];
        int endBin = bins[i + 1];

        for (int j = startBin; j < centerBin; ++j)
        {
            filter[j] = (j - startBin) / static_cast<float>(centerBin - startBin);
        }
        for (int j = centerBin; j < endBin; ++j)
        {
            filter[j] = (1.0f - (j - centerBin) / static_cast<float>(endBin - centerBin));
        }

        filterbank.push_back(std::move(filter)); // Use move semantics to avoid copying
    }

    return filterbank;
}

// Helper function to convert Mel to frequency
float AudioProcessor::MelToFrequency(float mel)
{
    return 700.0f * (pow(10.0f, mel / 2595.0f) - 1.0f);
}

// Helper function to convert frequency to Mel scale
float AudioProcessor::FrequencyToMel(float frequency)
{
    return 2595.0f * log10(1.0f + frequency / 700.0f);
}
