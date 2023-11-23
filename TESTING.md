# Testing Guide for EchoGrapherQT

This document provides a detailed guide on how to build and run the test cases for the EchoGrapherQT project.

## Prerequisites

Before running the tests, make sure you have the following installed:

- Qt 6.6.0 with the corresponding GCC 64-bit compiler.
- Make sure `qmake` is available in your system's PATH.

## Running Tests

Follow these steps to run the test cases:

### Step 1: Navigate to the Tests Directory

Open a terminal and navigate to the `tests` directory inside the EchoGrapherQT project folder.

```sh
cd path/to/EchoGrapherQT/tests
```

Replace `path/to/EchoGrapherQT` with the actual path to your EchoGrapherQT project directory.

### Step 2: Generate Makefile

Generate the `Makefile` using `qmake`.

```sh
qmake tests.pro
```

This command will create a `Makefile` based on the `tests.pro` project file.

### Step 3: Build the Test Cases

Compile the test cases using the `make` command.

```sh
make
```

This will build all the test cases defined in the project.

### Step 4: Run the Test Executable

Once the build process is complete, you can run the tests by executing the `tests` binary.

```sh
./tests
```

This will execute all the test cases and display the results in the terminal.

### Test Results
![image](https://github.com/AliTahir-101/EchoGrapher/assets/76158157/be1d3903-c627-4a05-b3c6-ac296c006f56)

### Troubleshooting

If you encounter any issues, please refer to the following troubleshooting steps:

- Ensure that qmake is correctly installed and configured.
- Check that all the dependencies of the project are correctly installed.
- Make sure that the Qt version matches the one specified in the project.
