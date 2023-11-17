#include <iostream>
#include <portaudio.h> // PortAudio is flexible and can be used for both simple and complex audio tasks, including system audio recording.
#include <fstream>
#include <vector>
#include <algorithm>
#include <fftw3.h>
#include <cmath>

using namespace std;

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
        const int windowSize = 1024;
        const int hopSize = windowSize / 2;
        vector<float> window(windowSize);

        // Hanning window
        for (int i = 0; i < windowSize; ++i)
        {
            window[i] = 0.5 * (1 - cos(2 * M_PI * i / (windowSize - 1)));
        }

        // Variables for FFTW
        fftwf_complex *in, *out;
        fftwf_plan plan;

        in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * windowSize);
        out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * windowSize);

        vector<float> recordedSamples(numSamples);
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
            err = Pa_ReadStream(stream, &recordedSamples[index], 256);
            if (err)
            {
                cerr << "PortAudio error: read stream: " << Pa_GetErrorText(err) << endl;
                return 1;
            }
            else
            {
                // Process in frames
                for (int i = 0; i + windowSize <= numSamples; i += hopSize)
                {
                    // Apply Hanning window and store in FFTW input
                    for (int j = 0; j < windowSize; ++j)
                    {
                        in[j][0] = recordedSamples[i + j] * window[j];
                        in[j][1] = 0.0;
                    }

                    // Create FFT plan and execute
                    plan = fftwf_plan_dft_1d(windowSize, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
                    fftwf_execute(plan);

                    // Process the FFT output (out) here

                    fftwf_destroy_plan(plan);
                }
            }
            index += 256;
        }

        err = Pa_CloseStream(stream);
        if (err != paNoError)
        {
            cerr << "PortAudio error: close stream: " << Pa_GetErrorText(err) << endl;
            return 1;
        }

        // After recording is done
        ofstream outFile("recorded_audio.wav", ios::binary);
        if (!outFile.is_open())
        {
            cerr << "Failed to open file for writing." << endl;
            return 1;
        }

        WriteWAVHeader(outFile, header);

        // Convert float samples to 16-bit PCM and write to file
        for (auto sample : recordedSamples)
        {
            int16_t intSample = static_cast<int16_t>(sample * 32767.0f);
            outFile.write(reinterpret_cast<const char *>(&intSample), sizeof(intSample));
        }

        outFile.close();

        // Clean up
        fftwf_free(in);
        fftwf_free(out);
        fftwf_cleanup();

        Pa_Terminate();
    }

    return 0;
}