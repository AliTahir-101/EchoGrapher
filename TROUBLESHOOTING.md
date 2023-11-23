# Troubleshooting Guide

This document provides a set of steps to troubleshoot common issues when setting up and running the project on Ubuntu.

## Preliminary Setup

Before proceeding with the installation of project dependencies, ensure your package lists are updated and your system is upgraded:

```bash
sudo apt update
sudo apt upgrade
```

## Installing Qt

To work with EchoGrapherQT, you need to have Qt installed on your system. Here's how you can install Qt:

### For Linux Systems:

1. **Download the Qt Online Installer:**

   - Visit the [Qt Open Source download page](https://www.qt.io/download-open-source).
   - Download the Qt online installer for your system, which is typically a file named like `qt-unified-linux-x64-<version>-online.run`.

2. **Make the Installer Executable:**

   - Open a terminal and navigate to the directory where you downloaded the installer.
   - Change the permissions to make it executable using the following command:

   ```bash
   chmod +x qt-unified-linux-x64-<version>-online.run
   ```

   - Replace `<version>` with the actual version number of the downloaded file.

3. **Run the Installer:**

   - Execute the installer with the following command:

   ```bash
   ./qt-unified-linux-x64-<version>-online.run
   ```

   - Follow the on-screen instructions to select the components you wish to install.

4. **Complete the Installation:**

   - Once the installation is complete, you can launch Qt Creator from your applications menu or via the command line.

## Dependency Installation

### PortAudio

To install PortAudio on Linux:

```bash
sudo apt-get install portaudio19-dev
```

### FFTW3

For the installation of FFTW3 and the float version:

```bash
sudo apt-get install libfftw3-dev libfftw3-single3
```

When using FFTW3 in your source files, include the header:

```c
#include <fftw3.h>
```

For your project file, link against the float version of the library:

```bash
LIBS += -lportaudio
LIBS += -lfftw3f
```

### ALSA and Jack Components (Optional)

If you encounter issues with ALSA or Jack, ensure they are installed and up to date:

```bash
sudo apt install alsa-base alsa-utils jackd2
sudo alsa force-reload
```

### QMake Command Not Found

If the `qmake` command is not found, you may need to locate it and add it to your PATH:

Search for all instances of `qmake`:

```bash
find / -name 'qmake' 2>/dev/null
```

Add the path to `qmake` to your `.bashrc` file:

```bash
echo 'export PATH=/home/ali/Qt/6.6.0/gcc_64/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

After setting the PATH, you should be able to run qmake on your project:

```bash
qmake EchoGrapherQT.pro
make
```
