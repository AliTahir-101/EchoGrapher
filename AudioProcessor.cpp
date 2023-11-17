#include <iostream>
#include <portaudio.h> // PortAudio is flexible and can be used for both simple and complex audio tasks, including system audio recording.
#include <fstream>

using namespace std;

struct WAVHeader
{
    char chunkID[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunkSize; // Size of the entire file in bytes minus 8 bytes
    char format[4] = {'W', 'A', 'V', 'E'};
    char subchunk1ID[4] = {'f', 'm', 't', ' '};
    uint32_t subchunk1Size = 16; // PCM header size
    uint16_t audioFormat = 1;    // PCM = 1
    uint16_t numChannels = 1;    // Mono = 1, Stereo = 2, etc.
    uint32_t sampleRate = 44100;
    uint32_t byteRate;           // sampleRate * numChannels * bitsPerSample/8
    uint16_t blockAlign;         // numChannels * bitsPerSample/8
    uint16_t bitsPerSample = 16; // 8 bits = 8, 16 bits = 16, etc.
    char subchunk2ID[4] = {'d', 'a', 't', 'a'};
    uint32_t subchunk2Size; // numSamples * numChannels * bitsPerSample/8
};

void WriteWAVHeader(ofstream &file, int numSamples, int sampleRate, int numChannels, int bitsPerSample)
{
    WAVHeader header;
    header.sampleRate = sampleRate;
    header.numChannels = numChannels;
    header.bitsPerSample = bitsPerSample;
    header.byteRate = sampleRate * numChannels * bitsPerSample / 8;
    header.blockAlign = numChannels * bitsPerSample / 8;
    header.subchunk2Size = numSamples * numChannels * bitsPerSample / 8;
    header.chunkSize = 4 + (8 + header.subchunk1Size) + (8 + header.subchunk2Size);

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

        inputParameters.device = Pa_GetDefaultInputDevice();
        if (inputParameters.device == paNoDevice)
        {
            cerr << "No default input device." << endl;
            return 1;
        }

        inputParameters.channelCount = 1;         // Mono input
        inputParameters.sampleFormat = paFloat32; // 32-bit floating point input
        inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream(
            &stream,
            &inputParameters,
            NULL,      // No output parameters for recording only
            44100,     // Sample rate
            256,       // Frames per buffer
            paClipOff, // We won't output out-of-range samples so don't bother clipping them
            NULL,      // No callback, use blocking API
            NULL);     // No data for the callback since we're not using one
        if (err != paNoError)
        {
            cerr << "PortAudio error: open stream: " << Pa_GetErrorText(err) << endl;
            return 1;
        }

        const int numSamples = 44100 * 5; // 5 seconds of audio
        float *recordedSamples = new float[numSamples];
        int index = 0;

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

        WriteWAVHeader(outFile, numSamples, 44100, 1, 16);

        // Convert float samples to 16-bit PCM and write to file
        for (int i = 0; i < numSamples; ++i)
        {
            int16_t intSample = static_cast<int16_t>(recordedSamples[i] * 32767.0f);
            outFile.write(reinterpret_cast<const char *>(&intSample), sizeof(intSample));
        }

        outFile.close();

        delete[] recordedSamples;

        Pa_Terminate();
    }

    return 0;
}