[![C/C++ build](https://github.com/JSBSim-Team/jsbsim/workflows/C/C++%20build/badge.svg?branch=master&event=push)](https://github.com/bcoconni/jsbsim/actions?query=workflow%3A%22C%2FC%2B%2B+build%22)
[![PyPI](https://img.shields.io/pypi/v/jsbsim)](https://pypi.org/project/JSBSim)
[![PyPI Downloads](https://static.pepy.tech/personalized-badge/jsbsim?period=total&units=international_system&left_color=grey&right_color=blue&left_text=pypi%20downloads)](https://pepy.tech/project/jsbsim)
[![Conda (channel only)](https://img.shields.io/conda/vn/conda-forge/jsbsim)](https://anaconda.org/conda-forge/jsbsim)

<p align="center">
<img width="250" heigth="250" src="https://github.com/JSBSim-Team/jsbsim-logo/blob/master/logo_JSBSIM_globe.png">
</p>

# Introduction

JSBSim is a multi-platform, general purpose object-oriented Flight Dynamics Model (FDM) written in C++. The FDM is essentially the physics & math model that defines the movement of an aircraft, rocket, etc., under the forces and moments applied to it using the various control mechanisms and from the forces of nature. JSBSim can be run in a standalone batch mode flight simulator (no graphical displays a.k.a. console mode) for testing and study, or integrated with the [Unreal engine](https://www.unrealengine.com), [FlightGear](https://www.flightgear.org/) and many other simulation environments.

Features include:

* Nonlinear 6 DoF (Degree of Freedom)
* Fully configurable flight control system, aerodynamics, propulsion, landing gear arrangement, etc. through XML-based text file format.
* Accurate Earth model including:
  * Rotational effects on the equations of motion (Coriolis and centrifugal acceleration modeled).
  * Oblate spherical shape and geodetic coordinates according to the [WGS84 geodetic system](https://en.wikipedia.org/wiki/World_Geodetic_System).
  * Atmosphere modeled according to the [International Standard Atmosphere (1976)](https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19770009539.pdf).
* Configurable data output formats to screen, file, socket, or any combination of those.

JSBSim also includes the following bindings:

* A [Python](https://www.python.org) module which provides the exact same features than the C++ library
* A [Matlab S-Function](https://github.com/JSBSim-Team/jsbsim/blob/master/matlab) that interfaces JSBSim with [MATLAB Simulink](https://fr.mathworks.com/products/simulink.html).
* An [Unreal Engine plugin](https://github.com/JSBSim-Team/jsbsim/blob/master/UnrealEngine/) to build a connection between the flight dynamics model from JSBSim and the entire virtual environment provided by [Unreal engine](https://www.unrealengine.com).

<p align="center">
<img src="UnrealEngine/UEReferenceApp.png">
</p>

In 2015, [NASA performed some verification check cases on 7 flight dynamics software **including JSBSim**](https://nescacademy.nasa.gov/flightsim) (the other 6 being NASA in-house software). The results showed that the 7 simulation tools *"were good enough to indicate agreement between a majority of simulation tools for all cases published. Most of the remaining differences are explained and could be reduced with further effort."*

## Applications and Usages

JSBSim is used in a range of projects among which:

* Unreal Engine's [Antoinette Project](https://www.unrealengine.com/en-US/blog/antoinette-project-tools-to-create-the-next-generation-of-flight-simulators): tools to create the next generation of flight simulators.
* Flight simulation: [FlightGear](http://www.flightgear.org), [OutTerra](https://www.outerra.com/wfeatures.html), [Skybolt Engine](https://github.com/Piraxus/Skybolt)
* SITL (Software In The Loop) Drone Autopilot testing : [ArduPilot](https://ardupilot.org/dev/docs/sitl-with-jsbsim.html), [PX4 Autopilot](https://dev.px4.io/master/en/simulation/jsbsim.html), [Paparazzi](https://wiki.paparazziuav.org/wiki/Simulation)
* Machine Learning Aircraft control: [gym-jsbsim](https://github.com/galleon/gym-jsbsim)
* [DARPA Virtual Air Combat Competition](https://www.darpa.mil/news-events/2019-10-21) where one of the AI went undefeated in five rounds of mock air combat against an Air Force fighter (see the [video on YouTube](https://www.youtube.com/watch?v=IOJhgC1ksNU)).

## Academic and Industry Research

JSBSim is also used in academic and industry research ([more than 700 citations referenced by Google Scholar](https://scholar.google.com/scholar?&q=jsbsim) as of May 2023).

In 2023 JSBSim has been featured in the article ["A deep reinforcement learning control approach for high-performance aircraft"](https://link.springer.com/article/10.1007/s11071-023-08725-y) on _Nonlinear Dynamics_, an International Journal of Nonlinear Dynamics and Chaos in Engineering Systems by Springer. The open access article is available as a PDF here [https://link.springer.com/content/pdf/10.1007/s11071-023-08725-y.pdf](https://link.springer.com/content/pdf/10.1007/s11071-023-08725-y.pdf). The work demonstrates an application of Deep Reinforcement Learning (DRL) to flight control and guidance, leveraging the JSBSim interface to MATLAB/Simulink.

# User Guide

## Installation

### Windows

A Windows installer `JSBSim-1.1.13-setup.exe` is available in the [release section](https://github.com/JSBSim-Team/jsbsim/releases/tag/v1.1.13). It installs the 2 executables along with aircraft data and some example scripts:

* `JSBSim.exe` which runs FDM simulations.
* `aeromatic.exe` which builds aircraft definitions from Question/Answer interface

Both executables are console line commands.

The Windows installer also contains the files needed to build the JSBSim Matlab S-Function (see [our MATLAB README](matlab/README.md) for more details about using JSBSim in Matlab).

### Ubuntu Linux

Debian packages for Ubuntu Linux "Bionic" 18.04 LTS and "Focal" 20.04 LTS for 64 bits platforms are also available in the [JSBSim project release section](https://github.com/JSBSim-Team/jsbsim/releases/tag/v1.1.13). There are 3 packages for each platform:

* `JSBSim_1.1.13-986.amd64.deb` which installs the executables `JSBSim` and `aeromatic`
* `JSBSim-devel_1.1.13-986.amd64.deb` which installs the development resources (headers and libraries)
* `python3-JSBSim_1.1.13-986.amd64.deb` which installs the Python module of JSBSim

### Python module

JSBSim provides binary wheel packages for its Python module on Windows, Mac OSX and Linux platforms for several Python versions (3.7, 3.8, 3.9, 3.10 and 3.11). These can be installed using either `pip` or `conda`.

#### Installation with `pip`

Binary packages a.k.a. [wheel packages](https://www.python.org/dev/peps/pep-0427) are available from the [Python Package Index (PyPI)](https://pypi.org), a repository of software for the Python programming language.

Installing `jsbsim` using `pip` can be achieved with:

```bash
> pip install jsbsim
```

Check the [pip documentation](https://packaging.python.org/tutorials/installing-packages) for more details.

Note that wheel packages for Linux meet the [PEP600 ManyLinux packages requirements](https://www.python.org/dev/peps/pep-0600) and as such are compatible with a majority of Linux distributions.

#### Installation with `conda`

[Conda](https://docs.conda.io/projects/conda/en/latest/index.html) is an open-source package management system and environment management system that runs on Windows, macOS, and Linux. The JSBSim conda package is available from [`conda-forge`](https://conda-forge.org), a community led collection of recipes, build infrastructure and distributions for the conda package manager.

Installing `jsbsim` from the `conda-forge` channel can be achieved by adding `conda-forge` to your channels with:

```bash
> conda config --add channels conda-forge
```

Once the `conda-forge` channel has been enabled, `jsbsim` can be installed with:

```bash
> conda install jsbsim
```

It is possible to list all of the versions of `jsbsim` available on your platform with:

```bash
> conda search jsbsim --channel conda-forge
```

### Other platforms

At the moment, JSBSim does not provide binaries for platforms other than Windows 64 bits and Ubuntu 64 bits. Alternatively, you can use [JSBSim wheel packages](https://github.com/bcoconni/jsbsim#python-module) for Windows, Linux or MacOS. Otherwise you should follow the instructions in the [developer docs](doc/DevelopersDocs.md) to build JSBSim on your platform.

### Aircraft data and example scripts

JSBSim aircraft data and example scripts are automatically installed if you are using [Python wheel packages](https://github.com/bcoconni/jsbsim#python-module). Otherwise, you can get aircraft data and example scripts by downloading either the [zip package](https://github.com/JSBSim-Team/jsbsim/archive/v1.1.13.zip) or the [tar.gz package](https://github.com/JSBSim-Team/jsbsim/archive/v1.1.13.tar.gz).

## Quick start

Once you have downloaded (or built) the binaries and unzipped the [aircraft data](#aircraft-data-and-example-scripts). Go to the root of the data package and make sure the executable is accessible from there.

You can then run an FDM simulation with the following command:

```bash
> JSBSim.exe --script=scripts/c1721.xml
```

More options are available if you run:

```bash
> JSBSim.exe --help
```

## User documentation

A first place to look at for JSBSim documentation resources is <https://jsbsim.sourceforge.net/documentation.html>. This link points to the official [JSBSim Reference Manual](https://jsbsim.sourceforge.net/JSBSimReferenceManual.pdf), a PDF which is the best source of information for users and developers.

However, due to the nature of the development of the project (JSBSim sources are updated often, sometimes even daily), several new features that are available in the software are not yet documented in the reference manual. Starting from March 2018 a new effort is underway to deliver an up-to-date documentation web site. You can browse the new *JSBSim Online Reference Manual* by going to: [https://jsbsim-team.github.io/jsbsim-reference-manual](https://jsbsim-team.github.io/jsbsim-reference-manual). The online manual is under construction and as a first milestone it will incorporate all the non-outdated material contained in the original PDF Reference Manual. The online manual web site is based on the GitHub Pages technology and its sources are available [here](https://github.com/JSBSim-Team/jsbsim-reference-manual). Eventually, the PDF Reference Manual will be superseded by the online manual, which is designed to be updated collaboratively as well as in efficient and timely fashion.

## Interfacing JSBSim with your application

### Using the C++ API

JSBSim can be interfaced or integrated to your application via a C++ API. The following code illustrates how JSBSim can be called by a small program, with execution being controlled by a script:

```c++
#include <FGFDMExec.h>

int main(int argc, char **argv)
{
  JSBSim::FGFDMExec FDMExec;
  FDMExec.LoadScript(SGPath(argv[1]));
  FDMExec.RunIC();
  bool result = true;
  while (result) result = FDMExec.Run();
}
 ```

The API is described in more details in the [C++ API documentation](doc/DevelopersDocs.md#c-api-documentation)

### Using the Python module

JSBSim can also be used as a Python module. JSBSim Python wheels are provided with the proverbial "batteries included" i.e. with some default aircraft data and example scripts.

The following code provides a simple example of how to interface with JSBSim using the Python programming language:

```python
import jsbsim

fdm = jsbsim.FGFDMExec(None)  # Use JSBSim default aircraft data.
fdm.load_script('scripts/c1723.xml')
fdm.run_ic()

while fdm.run():
  pass
```

Providing `jsbsim.FGFDMExec` with the value `None` allows using the installed default aircraft data and scripts (in the example above we are using the script `scripts/c1723.xml`, one of the many scripts installed by default).

The default aircraft data is located in a directory which path can be retrieved with the function `get_default_root_dir()`:

```python
print(jsbsim.get_default_root_dir())
```

A more elaborate example of Python code is [JSBSim.py](https://github.com/JSBSim-Team/jsbsim/blob/master/python/JSBSim.py), the Python equivalent to `JSBSim.exe`.

# Contact

For more information about JSBSim, you can contact the development team on [GitHub discussion](https://github.com/JSBSim-Team/jsbsim/discussions) or submit tickets on <https://github.com/JSBSim-Team/jsbsim/issues>

We are also on Facebook: <https://www.facebook.com/jsbsim/>

# Legal Notice

The JSBSim library is open source and is licensed under the [LGPL 2.1 license](https://opensource.org/licenses/LGPL-2.1). The license is included in the source code file [COPYING](https://github.com/JSBSim-Team/jsbsim/blob/master/COPYING).

The Unreal Engine Reference Application for JSBSim is open source and is licensed under the [MIT license](https://opensource.org/licenses/MIT). The license is included in the source code file [UnrealEngine/LICENSE.txt](https://github.com/JSBSim-Team/jsbsim/blob/master/UnrealEngine/LICENSE.txt).

The JSBSim interface with MATLAB including the S-Function is open source and is licensed under the [BSD license](https://opensource.org/licenses/bsd-license.php). The license is included in the source code file [matlab/LICENSE.txt](https://github.com/JSBSim-Team/jsbsim/blob/master/matlab/LICENSE.txt).

The Python module of JSBSim is open source and is licensed under the [LGPL 2.1 license](https://opensource.org/licenses/LGPL-2.1). The license is included in the source code file [COPYING](https://github.com/JSBSim-Team/jsbsim/blob/master/COPYING).

No proprietary code is included. All code included within the JSBSim project has been developed on a volunteer basis using publicly available information, and is often directly linked to a particular textbook, for educational reference. In some cases, code of a generic nature has been donated back to the project.

Likewise, the aircraft models included in this project and distribution do not include any proprietary, sensitive, or classified data. All data is derived from textbooks (such as Stevens and Lewis "Aircraft Control and Simulation" and Sutton's "Rocket Propulsion Elements"), freely available technical reports (see: <https://ntrs.nasa.gov> and <https://www.aiaa.org>), or other public data (such as the FAA web site). Aircraft models included in the JSBSim distribution and with names corresponding to existing commercial or military aircraft are approximations crafted using publicly available information, and are for educational or entertainment uses only.
