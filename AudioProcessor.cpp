#include <iostream>
#include <portaudio.h> // PortAudio is flexible and can be used for both simple and complex audio tasks, including system audio recording.
#include <fstream>

using namespace std;

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
            std::cerr << "No default input device." << std::endl;
            return 1;
        }

        inputParameters.channelCount = 2;         // Stereo input
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
            std::cerr << "PortAudio error: open stream: " << Pa_GetErrorText(err) << std::endl;
            return 1;
        }

        const int numSamples = 44100 * 5; // 5 seconds of audio
        float *recordedSamples = new float[numSamples];
        int index = 0;

        err = Pa_StartStream(stream);
        if (err != paNoError)
        {
            std::cerr << "PortAudio error: start stream: " << Pa_GetErrorText(err) << std::endl;
            return 1;
        }

        while (index < numSamples)
        {
            err = Pa_ReadStream(stream, &recordedSamples[index], 256);
            if (err)
            {
                std::cerr << "PortAudio error: read stream: " << Pa_GetErrorText(err) << std::endl;
                return 1;
            }
            index += 256;
        }

        err = Pa_CloseStream(stream);
        if (err != paNoError)
        {
            std::cerr << "PortAudio error: close stream: " << Pa_GetErrorText(err) << std::endl;
            return 1;
        }

        Pa_Terminate();
    }

    return 0;
}