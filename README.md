| Travis CI (Linux) | AppVeyor CI (Windows) |
|:-----------------:|:---------------------:|
| [![Travis CI](https://travis-ci.org/JSBSim-Team/jsbsim.svg?branch=master)](https://travis-ci.org/JSBSim-Team/jsbsim) | [![Build status](https://ci.appveyor.com/api/projects/status/89wkiqja63kc6h2v/branch/master?svg=true)](https://ci.appveyor.com/project/agodemar/jsbsim/branch/master) |

# Introduction
JSBSim is a multi-platform, general purpose object-oriented Flight Dynamics Model (FDM) written in C++. The FDM is essentially the physics & math model that defines the movement of an aircraft, rocket, etc., under the forces and moments applied to it using the various control mechanisms and from the forces of nature. JSBSim can be run in a standalone batch mode flight simulator (no graphical displays) for testing and study, or integrated with [FlightGear](http://home.flightgear.org/) or other flight simulator.

More information on JSBSim can be found at the JSBSim home page here:

http://www.jsbsim.org

# Getting the source

The GitHub repository of JSBSim is reachable at this link: [github.com/JSBSim-Team/jsbsim](https://github.com/JSBSim-Team/jsbsim). This repository mirrors the original one on SourceForge: [sourceforge.net/projects/jsbsim](https://sourceforge.net/projects/jsbsim).

## What you need to download the source

You need to have the [Git software](https://git-scm.com/) installed. Git is a *version control software*, a system that records changes to a file or set of files over time so that you can recall specific versions later. The JSBSim software source code files are being version controlled by Git.

To install Git [go to its download site](https://git-scm.com/downloads) and grab the version for your platform. You can choose to use Git locally on your computer in two ways: via one of the [GUI clients](https://git-scm.com/downloads/guis), or through a command shell (e.g. a *Bash shell* on Linux or Windows).

Once you have installed Git, assuming you are going to use Git from the command shell, the JSBSim source code public repository can be *cloned* from one of the two following locations.

## Downloading from [SourceForge](https://sourceforge.net/p/jsbsim/code/ci/master/tree/)

In such case the Git command to clone the repo is (HTTPS mode)

```bash
> git clone https://git.code.sf.net/p/jsbsim/code jsbsim-code
```

or (SSH mode)

```bash
> git clone git://git.code.sf.net/p/jsbsim/code jsbsim-code
```

## Downloading from [GitHub](https://github.com/JSBSim-Team/jsbsim)

in such case the Git command to clone the repo is (HTTPS mode)

```bash
> git clone https://github.com/JSBSim-Team/jsbsim.git jsbsim-code
```

or (SSH mode)

```bash
> git clone git@github.com:JSBSim-Team/jsbsim.git jsbsim-code
```

# Building JSBSim
JSBSim can either be built with [CMake](https://cmake.org/) or [Microsoft Visual Studio](https://www.visualstudio.com/free-developer-offers/). If you are using a Mac OSX or a Linux platform, you must use CMake. If you are a Windows user you can use either one.

JSBSim is coded in standard C++98/C99 and has no dependencies, so all you need is a C/C++ compiler installed on your platform.

## Building with CMake
CMake is a multiplatform tool to build and test software. It can produce files to build JSBSim with GNU make or Microsoft Visual Studio. To keep the build files separated from the source code, it is preferable to build JSBSim in a separate directory.
```bash
> cd jsbsim-code
> mkdir build
> cd build
```
CMake *does not build* software, it produces files *for* a multitude of build tools. The following commands are assuming that you are using GNU make to build JSBSim.

First, you should invoke CMake and then execute `make`
```bash
> cmake ..
> make
```

This will compile the various classes, and build the JSBSim application which will be located in `build/src`

### Options passed to CMake
CMake can use a number of parameters to tune the build of JSBSim. Different options are presented below. You can use them independently or any combination thereof depending on your needs.
#### Passing parameters to the compiler
If you want to set compiler options, you can pass flags to CMake to build a `Debug` version of JSBSim. JSBSim also uses C for some code, you can set options for both the C++ and the C compiler. 
```bash
> cmake -DCMAKE_CXX_FLAGS_DEBUG="-g -Wall" -DCMAKE_C_FLAGS_DEBUG="-g -Wall" -DCMAKE_BUILD_TYPE=Debug ..
> make
```
Or alternatively you can build a `Release` version of JSBSim and request GNU Make to use 4 cores to build the executable faster.
```bash
> cmake -DCMAKE_CXX_FLAGS_RELEASE="-O3 -march=native -mtune=native" -DCMAKE_C_FLAGS_RELEASE="-O3 -march=native -mtune=native" -DCMAKE_BUILD_TYPE=Release ..
> make -j4
```
#### Building Expat or using the system library
JSBSim uses the [Expat library](https://libexpat.github.io/) to read XML files. The Expat source code is provided with JSBSim source code and is compiled along with JSBSim during its build. However, if Expat is already installed on your platform you might prefer to use your system Expat library in order to avoid duplication. In that case, you should pass the `SYSTEM_EXPAT` flag to CMake:
```bash
> cmake -DSYSTEM_EXPAT=ON ..
> make
```

#### Building shared libraries
Most of JSBSim code can be built as a shared library, so that the executable `JSBSim` and the Python module can share the same library which reduce the memory and disk space consumption.
The option `BUILD_SHARED_LIBS` must then be passed to CMake
```bash
> cmake -DBUILD_SHARED_LIBS=ON ..
> make
```

### Building the [Python](https://www.python.org/) module of JSBSim
A Python module of JSBSim can also be built by CMake. For that, you need [Cython](http://cython.org/) installed on your platform. CMake will automatically detect Cython and build the Python module.

## Building with Microsoft Visual Studio
From Visual Studio, you can open the project file `JSBSim.vcxproj` to open a project for JSBSim. The project file will setup Visual Studio for building the JSBSim executable.

**Note 1:** JSBSim official build tool is CMake. Visual Studio project files are provided as a convenience and are not guaranteed to be up to date with the code.

**Note 2:** Since Visual Studio 2017, Microsoft has included CMake so you can build JSBSim on VS2017 directly from the CMake file.

For more detailed instructions on using Visual Studio project files and CMake via Visual Studio to build JSBSim take a look at the following documentation link - [Building using Visual Studio](https://jsbsim-team.github.io/jsbsim-reference-manual/mypages/building-using-visualstudio/).

# Testing JSBSim
JSBSim comes with a test suite to automatically check that the build is correct. This test suite is located in the `tests` directory and is coded in Python so you need the [Python module for JSBSim to be built].

The tests are also using `numpy`, `pandas` and `scipy` so you need these Python modules to be installed on your system.

The test suite can then be run using `ctest` in the `build` directory. Tests can also be run in parallel on several cores (4 in the example below) using the option `-j`
```bash
> ctest -j4
```

# Installing JSBSim
Once JSBSim is built and tested, you can install the C++ headers and library. For that, you can invoke GNU make from the `build` directory
```bash
> make install
```

By default, CMake copies the files to a location where the headers and library are available platform wide (typically `/usr/include`, `/usr/lib` or `/usr/local/include`, `/usr/local/lib` for *nix OSes). If you want to install the files in another location you can pass the flag `CMAKE_INSTALL_PREFIX` to cmake.
```bash
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..
make
make install
```
## Installing the Python module
If you plan to install the Python module of JSBSim in addition to the C++ headers and library, then you must pass the flag `INSTALL_PYTHON_MODULE` to CMake
```bash
> cmake -DINSTALL_PYTHON_MODULE=ON ..
> make
> make install
```

Alternatively, the Python module can be installed manually by invoking the following command from the `build` directory
```bash
> python tests/setup.py install
```

# Documentation

A first place to look at for JSBSim documentation resouces is [http://jsbsim.sourceforge.net/documentation.html](http://jsbsim.sourceforge.net/documentation.html). This link points to the official [JSBSim Reference Manual](http://jsbsim.sourceforge.net/JSBSimReferenceManual.pdf), a PDF which is the best source of information for users and developers.

However, due to the nature of the development of the project (JSBSim sources are updated often, sometimes even daily), several new features that are available in the software are not yet documented in the reference manual. Starting from March 2018 a new effort is underway to deliver an up-to-date documentation web site. You can browse the new *JSBSim Online Reference Manual* by going to: [https://jsbsim-team.github.io/jsbsim-reference-manual](https://jsbsim-team.github.io/jsbsim-reference-manual). The online manual is under construction and as a first milestone it will incorporate all the non-outdated material contained in the original PDF Reference Manual. The online manual web site is based on the GitHub Pages technology and its sources are available [here](https://github.com/JSBSim-Team/jsbsim-reference-manual). Eventually, the PDF Reference Manual will be superseded by the online manual, which is designed to be updated collaboratively as well as in efficient and timely fashion.

The JSBSim API documentation can be viewed [here](http://jsbsim.sourceforge.net/JSBSim/).

# Contact
For more information on JSBSim, you can contact the development team on the mailing list jsbsim-devel@lists.sourceforge.net or submit tickets on https://github.com/JSBSim-Team/jsbsim/issues
