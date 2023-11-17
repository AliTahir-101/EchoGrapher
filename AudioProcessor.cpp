#include <iostream>
#include <portaudio.h> // PortAudio is flexible and can be used for both simple and complex audio tasks, including system audio recording.

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

        Pa_Terminate();
    }

    return 0;
}