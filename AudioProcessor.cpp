#include <iostream>
#include <portaudio.h> // PortAudio is flexible and can be used for both simple and complex audio tasks, including system audio recording.
#include <fstream>
#include <vector>
#include <algorithm>
#include <fftw3.h>
#include <cmath>
#include <cstring>

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

using namespace std;

// Global variables for thread communication
mutex mtx;
condition_variable cv;
queue<vector<float>> dataQueue;          // Queue to hold chunks of audio data
queue<vector<float>> processedDataQueue; // Queue to hold chunks of processed audio data
bool finishedRecording = false;          // Flag to signal the recording is finished

// Helper function to convert frequency to Mel scale
float FrequencyToMel(float frequency)
{
    return 2595.0f * log10(1.0f + frequency / 700.0f);
}

// Helper function to convert Mel to frequency
float MelToFrequency(float mel)
{
    return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
}

// Function to create a Mel filterbank
vector<vector<float>> CreateMelFilterbank(int numFilters, int fftSize, int sampleRate)
{
    vector<vector<float>> filterbank(numFilters, vector<float>(fftSize / 2, 0.0f));

    // Compute the Mel frequency limits
    float lowerMelFreq = FrequencyToMel(0);
    float upperMelFreq = FrequencyToMel(sampleRate / 2);

    // Compute the Mel frequencies for each filter
    vector<float> melPoints(numFilters + 2); // +2 for the start and end points
    for (int i = 0; i < melPoints.size(); ++i)
    {
        melPoints[i] = lowerMelFreq + i * (upperMelFreq - lowerMelFreq) / (numFilters + 1);
    }

    // Convert Mel points back to frequencies
    vector<float> binFrequencies(melPoints.size());
    for (int i = 0; i < melPoints.size(); ++i)
    {
        binFrequencies[i] = MelToFrequency(melPoints[i]);
    }

    // Determine the FFT bin corresponding to each frequency
    vector<int> bins(melPoints.size());
    for (int i = 0; i < melPoints.size(); ++i)
    {
        bins[i] = std::floor((fftSize + 1) * binFrequencies[i] / sampleRate);
    }

    // Create the triangular filters
    for (int i = 1; i < melPoints.size() - 1; ++i)
    {
        int startBin = bins[i - 1];
        int centerBin = bins[i];
        int endBin = bins[i + 1];

        for (int j = startBin; j < centerBin; ++j)
        {
            filterbank[i - 1][j] = (j - startBin) / (float)(centerBin - startBin);
        }
        for (int j = centerBin; j < endBin; ++j)
        {
            filterbank[i - 1][j] = (1 - (j - centerBin) / (float)(endBin - centerBin));
        }
    }

    return filterbank;
}

// Function to convert FFT data to Mel Spectrum
vector<float> ConvertToMelSpectrum(fftwf_complex *fftData, int dataSize, int sampleRate)
{
    // Number of Mel filters
    const int numMelFilters = 26; // You can choose this number based on your needs
    // Compute the Mel filterbank
    auto melFilterbank = CreateMelFilterbank(numMelFilters, dataSize / 2, sampleRate);

    // Initialize the Mel spectrum vector
    vector<float> melSpectrum(numMelFilters, 0.0f);

    // Convert each FFT bin to power
    vector<float> powerSpectrum(dataSize / 2);
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
        melSpectrum[filterNum] = melEnergy;
    }

    return melSpectrum;
}

void ProcessAudioThread(uint32_t actualSampleRate)
{
    // Variables for FFTW
    const int windowSize = 256;
    fftwf_complex *in, *out;
    fftwf_plan plan_forward, plan_backward;
    const int hopSize = windowSize / 2;
    vector<float> window(windowSize);

    // Initialize Hanning window
    for (int i = 0; i < windowSize; ++i)
    {
        window[i] = 0.5 * (1 - cos(2 * M_PI * i / (windowSize - 1)));
    }

    in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * windowSize);
    if (!in)
    {
        cerr << "Error: FFTW allocation failed for input array." << endl;
    }
    out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * windowSize);

    plan_forward = fftwf_plan_dft_1d(windowSize, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    if (!plan_forward)
    {
        cerr << "Error: FFTW plan creation failed." << endl;
    }
    plan_backward = fftwf_plan_dft_1d(windowSize, out, in, FFTW_BACKWARD, FFTW_ESTIMATE);

    vector<float> audioChunk;
    while (true)
    {
        // Lock and wait for new data
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, []
                { return !dataQueue.empty() || finishedRecording; });

        if (finishedRecording && dataQueue.empty())
        {
            break; // No more data to process and recording is finished
        }

        // Retrieve the next chunk of audio data
        audioChunk = move(dataQueue.front());
        dataQueue.pop();
        lock.unlock(); // Unlock as soon as possible

        cout << "Audio chunk size: " << audioChunk.size() << ", window size: " << window.size() << endl;
        if (audioChunk.size() < windowSize)
        {
            cerr << "Error: audioChunk size is less than windowSize." << endl;
            continue; // Skip this iteration or handle error appropriately
        }

        // FFT processing here
        // Compute the FFT on the chunk
        for (int i = 0; i < windowSize; ++i)
        {
            in[i][0] = audioChunk[i] * window[i]; // Apply window function
            in[i][1] = 0.0;
        }

        fftwf_execute(plan_forward);

        // Convert to power spectrum and apply mel scaling
        vector<float> melSpectrum = ConvertToMelSpectrum(out, windowSize, actualSampleRate);

        // Take the log of the mel spectrum
        for (auto &value : melSpectrum)
        {
            value = log(value + 1e-6); // Avoid log(0) by adding a small constant
        }

        // Print the log mel spectrogram data for this chunk
        for (const auto &value : melSpectrum)
        {
            cout << value << " ";
        }
        cout << endl;

        // After processing, push the data to the processedDataQueue
        unique_lock<mutex> lockProcessed(mtx);
        processedDataQueue.push(audioChunk); // assuming 'audioChunk' now contains processed data
        lockProcessed.unlock();              // Unlock immediately after the operation
        cv.notify_one();                     // Signal that processed data is available
    }

    // Clean up FFTW resources
    fftwf_destroy_plan(plan_forward);
    fftwf_destroy_plan(plan_backward);
    fftwf_free(in);
    fftwf_free(out);
    fftwf_cleanup();
}

struct WAVHeader
{
    char chunkID[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunkSize; // Size of the entire file in bytes minus 8 bytes
    char format[4] = {'W', 'A', 'V', 'E'};
    char subchunk1ID[4] = {'f', 'm', 't', ' '};
    uint32_t subchunk1Size = 16; // PCM header size
    uint16_t audioFormat = 1;    // PCM = 1
    uint16_t numChannels;        // Mono = 1
    uint32_t sampleRate;
    uint32_t byteRate;           // sampleRate * numChannels * bitsPerSample/8
    uint16_t blockAlign;         // numChannels * bitsPerSample/8
    uint16_t bitsPerSample = 16; // 8 bits = 8, 16 bits = 16, etc.
    char subchunk2ID[4] = {'d', 'a', 't', 'a'};
    uint32_t subchunk2Size; // numSamples * numChannels * bitsPerSample/8
};

void WriteWAVHeader(ofstream &file, WAVHeader header)
{
    file.write(reinterpret_cast<const char *>(&header), sizeof(header));
}

int main()
{
    PaError err = Pa_Initialize(); // Calling Library initialization function
    if (err != paNoError)
    {
        cerr << "PortAudio error: " << Pa_GetErrorText(err) << endl;
        return 1;
    }
    else
    {
        cout << "PortAudio Api Initialized Successfully!" << endl;

        PaStream *stream;
        PaStreamParameters inputParameters;
        const PaDeviceInfo *deviceInfo;

        inputParameters.device = Pa_GetDefaultInputDevice();
        if (inputParameters.device == paNoDevice)
        {
            cerr << "No default input device." << endl;
            return 1;
        }

        deviceInfo = Pa_GetDeviceInfo(inputParameters.device);

        // Check and set the sample rate based on device capability
        uint32_t desiredSampleRate = 48000;
        uint32_t actualSampleRate = min(deviceInfo->defaultSampleRate, static_cast<double>(desiredSampleRate));
        thread audioProcessingThread(ProcessAudioThread, actualSampleRate);
        // cout << "desiredSampleRate: " << desiredSampleRate << endl;
        // cout << "deviceInfo->defaultSampleRate: " << deviceInfo->defaultSampleRate << endl;
        inputParameters.channelCount = 1;         // Mono input
        inputParameters.sampleFormat = paFloat32; // 32-bit floating point input
        inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;

        err = Pa_OpenStream(
            &stream,
            &inputParameters,
            nullptr,          // No output parameters for recording only
            actualSampleRate, // Sample rate
            256,              // Frames per buffer
            paClipOff,        // We won't output out-of-range samples so don't bother clipping them
            nullptr,          // No callback, use blocking API
            nullptr);         // No data for the callback since we're not using one
        if (err != paNoError)
        {
            cerr << "PortAudio error: open stream: " << Pa_GetErrorText(err) << endl;
            return 1;
        }

        const int numSamples = actualSampleRate * 5; // 5 seconds of audio

        // Constants for Hanning window and frame processing
        const int windowSize = 256;
        const int hopSize = windowSize / 2;
        vector<float> window(windowSize);

        // Hanning window
        for (int i = 0; i < windowSize; ++i)
        {
            window[i] = 0.5 * (1 - cos(2 * M_PI * i / (windowSize - 1)));
        }

        int index = 0;
        WAVHeader header;

        // Set these values based on your actual audio data
        header.numChannels = 1;               // Mono = 1, Stereo = 2
        header.sampleRate = actualSampleRate; // e.g., 44100 or 48000
        header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8;
        header.blockAlign = header.numChannels * header.bitsPerSample / 8;
        header.subchunk2Size = numSamples * header.numChannels * header.bitsPerSample / 8;
        header.chunkSize = 36 + header.subchunk2Size;

        err = Pa_StartStream(stream);
        if (err != paNoError)
        {
            cerr << "PortAudio error: start stream: " << Pa_GetErrorText(err) << endl;
            return 1;
        }

        while (index < numSamples)
        {
            vector<float> chunk(256); // Temporary buffer to hold the audio chunk
            err = Pa_ReadStream(stream, chunk.data(), 256);
            if (err)
            {
                cerr << "PortAudio error: read stream: " << Pa_GetErrorText(err) << endl;
                return 1;
            }
            else
            {
                // Lock and add the chunk to the queue
                unique_lock<mutex> lock(mtx);
                dataQueue.push(move(chunk));
                lock.unlock();
                cv.notify_one(); // Signal the processing thread that new data is available
            }
            index += 256;
        }

        // Signal that recording is finished
        finishedRecording = true;
        cv.notify_one();              // Notify the processing thread
        audioProcessingThread.join(); // Wait for the audio processing thread to finish

        err = Pa_CloseStream(stream);
        if (err != paNoError)
        {
            cerr << "PortAudio error: close stream: " << Pa_GetErrorText(err) << endl;
            return 1;
        }

        // Now we are sure that all audio data has been processed
        ofstream outFile("recorded_audio.wav", ios::binary);
        if (!outFile.is_open())
        {
            cerr << "Failed to open file for writing." << endl;
            return 1;
        }

        WriteWAVHeader(outFile, header);

        // Write the processed data to the file
        while (!processedDataQueue.empty())
        {
            const auto &processedChunk = processedDataQueue.front();
            for (auto sample : processedChunk)
            {
                int16_t intSample = static_cast<int16_t>(sample * 32767.0f);
                outFile.write(reinterpret_cast<const char *>(&intSample), sizeof(intSample));
            }
            processedDataQueue.pop();
        }

        outFile.close();
        Pa_Terminate();
    }

    return 0;
}