# Developer documentation

## Downloading the source

The GitHub repository of JSBSim is reachable at this link: [github.com/JSBSim-Team/jsbsim](https://github.com/JSBSim-Team/jsbsim).

### What you need to download the source

You need to have the [Git software](https://git-scm.com/) installed. Git is a *version control software*, a system that records changes to a file or set of files over time so that you can recall specific versions later. The JSBSim software source code files are being version controlled by Git.

To install Git [go to its download site](https://git-scm.com/downloads) and grab the version for your platform. You can choose to use Git locally on your computer in two ways: via one of the [GUI clients](https://git-scm.com/downloads/guis), or through a command shell (e.g. a *Bash shell* on Linux or Windows).

Once you have installed Git, assuming you are going to use Git from the command shell, the JSBSim source code public repository can be *cloned* from [GitHub](https://github.com/JSBSim-Team/jsbsim)

The Git command to clone the repo is (HTTPS mode)

```bash
> git clone https://github.com/JSBSim-Team/jsbsim.git jsbsim-code
```

or (SSH mode)

```bash
> git clone git@github.com:JSBSim-Team/jsbsim.git jsbsim-code
```

## Building JSBSim

JSBSim can either be built with [CMake](https://cmake.org/) or [Microsoft Visual Studio](https://www.visualstudio.com/free-developer-offers/). If you are using a Mac OSX or a Linux platform, you must use CMake. If you are a Windows user you can use either one.

JSBSim is coded in standard C++17 and has no dependencies, so all you need is a C/C++ compiler installed on your platform.

### Building with CMake

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

#### Options passed to CMake

CMake can use a number of parameters to tune the build of JSBSim. Different options are presented below. You can use them independently or any combination thereof depending on your needs.

##### Passing parameters to the compiler

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

##### Building Expat or using the system library

JSBSim uses the [Expat library](https://libexpat.github.io/) to read XML files. The Expat source code is provided with JSBSim source code and is compiled along with JSBSim during its build. However, if Expat is already installed on your platform you might prefer to use your system Expat library in order to avoid duplication. In that case, you should pass the `SYSTEM_EXPAT` flag to CMake:

```bash
> cmake -DSYSTEM_EXPAT=ON ..
> make
```

##### Building shared libraries

Most of JSBSim code can be built as a shared library, so that the executable `JSBSim` and the Python module can share the same library which reduce the memory and disk space consumption.
The option `BUILD_SHARED_LIBS` must then be passed to CMake

```bash
> cmake -DBUILD_SHARED_LIBS=ON ..
> make
```

#### Building the Python module of JSBSim

A [Python](https://www.python.org/) module of JSBSim can also be built by CMake. For that, you need [Cython](http://cython.org/) installed on your platform. CMake will automatically detect Cython and build the Python module.

### Building with Microsoft Visual Studio

From Visual Studio, you can open the project file `JSBSim.vcxproj` to open a project for JSBSim. The project file will setup Visual Studio for building the JSBSim executable.

**Note 1:** JSBSim official build tool is CMake. Visual Studio project files are provided as a convenience and are not guaranteed to be up to date with the code.

**Note 2:** Since Visual Studio 2017, Microsoft has included CMake so you can build JSBSim on VS2017 directly from the CMake file.

For more detailed instructions on using Visual Studio project files and CMake via Visual Studio to build JSBSim take a look at the following documentation link - [Building using Visual Studio](https://jsbsim-team.github.io/jsbsim-reference-manual/mypages/quickstart-building-using-visualstudio/).

## Testing JSBSim

JSBSim comes with a test suite to automatically check that the build is correct. This test suite is located in the `tests` directory and is coded in Python so you need to [build the Python module of JSBSim](#building-the-python-module-of-jsbsim) first.

The tests are also using `numpy`, `pandas` and `scipy` so you need these Python modules to be installed on your system.

Make sure that the JSBSim shared library is in the path of the dynamic linker before running the tests (see [the procedure to add the JSBSim library to the dynamic linker path](#when-i-try-to-run-ctest-most-of-the-tests-fail) if you get multiple failures of the tests).

The test suite can then be run using `ctest` in the `build` directory. Tests can also be run in parallel on several cores (4 in the example below) using the option `-j`

```bash
> ctest -j4
```

### C++ code coverage

Tests ran with `ctest` include C++ unit tests that are written with the [CxxTest](https://cxxtest.com) test framework.

A code coverage report is automatically generated and is available at: <https://jsbsim-team.github.io/jsbsim/coverage/>

## Installing JSBSim

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

### Installing the Python module

#### Installation with CMake

If you plan to install the Python module of JSBSim in addition to the C++ headers and library, then you must pass the flag `INSTALL_JSBSIM_PYTHON_MODULE` to CMake. This is the procedure you should follow if you plan to package JSBSim with CPack.

```bash
> cmake -DINSTALL_JSBSIM_PYTHON_MODULE=ON ..
> make
> make install
```

**Note:** `make install` will attempt to override [Python virtual environments](https://docs.python.org/3/tutorial/venv.html) in order to install the Python module platform wide (i.e. in a directory such as `/usr/lib/python`). If you want the Python module installation process to comply with your virtual environment, you should use the Python `distutils` as described below.

#### Installation of the Python module

Alternatively, you can install the Python module manually using the Python `build` package and `pip`. It is recommended to perform this installation inside a Python virtual environment to avoid affecting your system-wide Python packages.

From the `build` directory, run the following commands:

```bash
> cd python
> pip install build
> python -m build --wheel
> pip install jsbsim --no-index -f dist
```

Here's what these commands do:

- `cd python`: Navigate to the Python module directory inside your build folder.
- `pip install build`: Ensure the Python `build` package is installed.
- `python -m build --wheel`: Build a wheel (binary package) for the JSBSim Python module. This will only compile and link the `_jsbsim.cxx` file generated by Cython, since the main library has already been built.
- `pip install jsbsim --no-index -f dist`: Installs the JSBSim wheel from the local `dist` directory. The `--no-index` flag prevents `pip` from searching the Python Package Index (PyPI) and only considers package sources specified by the `-f dist` flag, which points to the local `dist` folder.

After running these commands, the JSBSim Python module will be installed in your current Python environment.

## Packaging JSBSim for releases

JSBSim can also be packaged for releases. This is done automatically by Travis CI for the Ubuntu 14.04 LTS platform and the resulting Debian packages are available for download on the [JSBSim GitHub project page](https://github.com/JSBSim-Team/jsbsim/releases).

The packaging can be done by passing the option `CPACK_GENERATOR` to CMake then invoking CPack.

*At the moment, only RPM and Debian packages are supported by JSBSim.*

```bash
> cmake -DCPACK_GENERATOR=DEB .. # or RPM
> make
> cpack
```

The following packages are then built (with the extension `.rpm` if you selected the RPM generator)

- `JSBSim_[version].[platform].[architecture].deb` which contains the executables `JSBSim` and `aeromatic` (and shared libraries if `BUILD_SHARED_LIBS` was set to `ON`)
- `JSBSim-devel_[version].[platform].[architecture].deb` which contains the files for C++ development headers (and the static library if `BUILD_SHARED_LIBS` was **not** set to `ON`)
- `python[2-3]-JSBSim_[version].[platform].[architecture].deb` which contains the JSBSim Python module if `INSTALL_JSBSIM_PYTHON_MODULE` was set to `ON`

## C++ API documentation

The JSBSim C++ API documentation is built from the source code with [Doxygen](http://www.stack.nl/~dimitri/doxygen/) and is automatically published on GitHub each time a commit is pushed to the JSBSim GitHub project. It can be viewed here:

<https://jsbsim-team.github.io/jsbsim/>.

If you modify the documentation, you might need to generate the documentation locally in which case you should run the following command after `cmake` has been executed

```bash
> make doc
```

The HTML documentation will then be available in the directory `build/documentation/html`. Note that you need [Doxygen](www.doxygen.org) and [Graphviz](www.graphviz.org) to be installed.

## Frequently Asked Questions

### How can I get more details about the failure of a test ran by `ctest` ?

**Q:** I ran `ctest` from my build directory and it reports one or several tests failures. The problem is that `ctest` does not seem to give any details about the reason why the tests failed.

**A:** All the output issued by tests run by `ctest` are logged in the file `Testing/Temporary/LastTestsFailed.log`.

### How can I select the tests ran by `ctest` ?

**Q:** One of the tests ran by `ctest` is failing and I am trying to debug it. But `ctest` always executes all the tests and it takes much time. To iterate faster, I would like to focus only on the test that fails. How do I tell `ctest` to skip all the tests but the test that fails ?

**A:** You can filter the tests executed by `ctest` with the options `-R`, `-E` and `-I`. Say you want to run exclusively `TestDensityAltitude.py` then you can use the following command

```bash
> ctest -R TestDensityAltitude
```

If you need to run all the tests which name contains `Altitude`, you can use

```bash
> ctest -R Altitude
```

If you need to run all the tests but thoses which name contains `Altitude`, you can use

```bash
> ctest -E Altitude
```

If you need to run tests #12 to #14, you can use

```bash
> ctest -I 12,14
```

You can find more informations about `ctest` from its [manual page](https://cmake.org/cmake/help/v3.0/manual/ctest.1.html)

### When I try to run `ctest`, most of the tests fail

**Q:** Before running `make install`, I want to execute `ctest` but most of the tests fail. And when I check in the file `Testing/Temporary/LastTestsFailed.log` it reports many Python `ImportError` such as

```text
31: Traceback (most recent call last):
31:   File "TestTurbine.py", line 23, in <module>
31:     from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest, append_xml
31:   File "/xxx/build/tests/JSBSim_utils.py", line 24, in <module>
31:     import jsbsim
31: ImportError: libJSBSim.so.1: cannot open shared object file: No such file or directory
```

**A:** This error means that the dynamic linker `ld.so` cannot find the JSBSim shared library (`libJSBSim.so.1`) so you must tell it where it is located. For that purpose, you can use the environment variable `LD_LIBRARY_PATH` like below

```bash
> LD_LIBRARY_PATH=$PWD/src ctest
```

The command above will succeed if you execute it from your build directory in which case the environment variable `PWD` will contain the path to your build directory.

To avoid prepending every `ctest` command with `LD_LIBRARY_PATH=$PWD/src` you can execute the command below to store the modification to the `LD_LIBRARY_PATH` environment variable. This command must be executed from your `build` directory in order to store the appropriate path.

```bash
> export LD_LIBRARY_PATH=$PWD/src:$LD_LIBRARY_PATH
```

Please note that as soon as your shell session will be terminated, the modification to `LD_LIBRARY_PATH` will be lost.
