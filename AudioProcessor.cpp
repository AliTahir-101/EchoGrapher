#include <iostream>
#include <portaudio.h> // PortAudio is flexible and can be used for both simple and complex audio tasks, including system audio recording.
#include <fstream>
#include <vector>
#include <algorithm>
#include <fftw3.h>

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
        vector<float> recordedSamples(numSamples);
        vector<fftwf_complex> out(numSamples / 2 + 1); // FFTW output array
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
            index += 256;
        }

        // FFTW Plan
        fftwf_plan plan = fftwf_plan_dft_r2c_1d(numSamples, recordedSamples.data(), out.data(), FFTW_ESTIMATE);

        // Execute FFTW Plan
        fftwf_execute(plan);

        // ... [Code to analyze/process FFTW output will come here]

        // Cleanup
        fftwf_destroy_plan(plan);
        fftwf_cleanup();

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

        Pa_Terminate();
    }

    return 0;
}