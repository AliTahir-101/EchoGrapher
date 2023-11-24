## About This Project

This project was developed as part of a technical interview process for [Supplyz.eu](https://supplyz.eu). The task was to create a cross-platform audio analysis tool capable of recording and visualizing audio spectrograms in real time.

# 🎵 Audio Spectrogram Visualizer - EchoGrapher 🎛️

## Overview 🌐

EchoGrapher is a cross-platform desktop tool designed for audio analysis and visualization. It captures audio through your system's microphone and renders a real-time log mel spectrogram a sophisticated visual representation of sound frequency and amplitude over time.

https://github.com/AliTahir-101/EchoGrapher/assets/76158157/eb8ef245-bee4-44bf-8fd5-a6c23922b468

## Features 🚀

- 🔊 Real-time audio recording and visualization
- 📊 Log mel spectrogram display with adjustable parameters
- 🛠️ Customizable window size, overlap, and number of mel bands
- 🔍 Zoom in/out and reset capabilities for thorough analysis

## Getting Started 🏁

### Project Structure 📂

The `EchoGrapherQT` project folder encompasses:

- 📝 Source files: `audioprocessor.cpp`, `main.cpp`, `mainwindow.cpp`
- 🗂️ Header files: `audioprocessor.h`, `mainwindow.h`
- 🖼️ UI file: `mainwindow.ui`
- 🔧 Project file: `EchoGrapherQT.pro`
- 🚫 User-specific settings: `EchoGrapherQT.pro.user` (should not be versioned)

Upon build, a directory named `build-EchoGrapherQT-Desktop_Qt_6_6_0_GCC_64bit-Debug` is generated, housing all compiled and intermediate files needed to run the application.

## Building and Running the Application 🛠️

### Prerequisites ✅

Make sure to have the following installed:

- 🌟 Qt 6.6.0 or later
- 🖥️ GCC (for Linux/macOS) or MSVC (for Windows) with C++17 support
- 📚 Dependencies: PortAudio, FFTW3

### Installation Steps 📌

#### General Dependencies 📦

Install the following prerequisites before proceeding:

##### PortAudio

- 🐧 Linux: `sudo apt-get install portaudio19-dev`
- 🍎 macOS: `brew install portaudio`
- 🪟 Windows: Download from [PortAudio&#39;s website](http://www.portaudio.com/download.html).

##### FFTW3

- 🐧 Linux: `sudo apt-get install libfftw3-dev`
- 🍎 macOS: `brew install fftw`
- 🪟 Windows: Download from [FFTW&#39;s website](http://www.fftw.org/install/windows.html).

### Compiling the Application 🏗️

#### On Ubuntu Linux 🐧

1. **Install Dependencies**
   ```bash
   sudo apt-get update
   sudo apt-get install qtbase5-dev libportaudio19-dev libfftw3-dev
   ```

### Using Qt Creator 🛠️ (Recommended)

1. Launch Qt Creator and choose `Open Project`.
2. Navigate to your project's directory and select the .pro file.
3. Select the appropriate kit for your OS and configure your project.
4. Hit `Build` to compile.
5. Once built, click `Run` in Qt Creator to start the app.
6. Launch Qt Creator and choose `Open Project`.
7. Navigate to your project's directory and select the .pro file.
8. Select the appropriate kit for your OS and configure your project.
9. Hit `Build` to compile.
10. Once built, click `Run` in Qt Creator to start the app.

### Using the Command Line 💻

1. Clone the repository and enter the project directory:

```bash
git clone git@github.com:AliTahir-101/EchoGrapher.git
cd EchoGrapherQT
```

2. Prepare a build directory and navigate into it:

```bash
mkdir build && cd build
```

3. Generate the Makefile using `qmake`:

```bash
qmake ..
```

4. Compile the project with `make` (on Windows, use `nmake` or `jom`):

```bash
make
```

5. The EchoGrapherQT executable (or EchoGrapherQT.exe on Windows) will be in the build folder.

### Running the Application 🚀

Execute the EchoGrapherQT binary `./EchoGrapherQT` to launch the app or In Windows open the .exe file. In Qt Creator, you can run the app with a simple click of the 'Run' button.

### Usage 🔧

- Start the application as per the installation instructions.
- Interact with the UI to begin recording and visualizing the spectrogram.
- Modify spectrogram parameters to fit your analysis needs.

### Testing 🧪

Unit and integration tests are located within the tests directory. See [TESTING.md](https://github.com/AliTahir-101/EchoGrapher/blob/main/TESTING.md) for execution instructions.

### Version Control 🔄

Git is employed for version control. For the complete commit history, visit the [repository](https://github.com/AliTahir-101/EchoGrapher/commits/main).

## Authors 👥

Ali Tahir - Initial work - [GitHub](https://github.com/AliTahir-101)

### Troubleshooting 🛠️

Encountering issues? Please consult [TROUBLESHOOTING.md](https://github.com/AliTahir-101/EchoGrapher/blob/main/TROUBLESHOOTING.md) or raise an issue in the repository.
