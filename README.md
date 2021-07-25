[![C/C++ build](https://github.com/JSBSim-Team/jsbsim/workflows/C/C++%20build/badge.svg?branch=master&event=push)](https://github.com/bcoconni/jsbsim/actions?query=workflow%3A%22C%2FC%2B%2B+build%22)
[![PyPI](https://img.shields.io/pypi/v/jsbsim)](https://pypi.org/project/JSBSim)
[![PyPI Downloads](https://static.pepy.tech/personalized-badge/jsbsim?period=total&units=international_system&left_color=grey&right_color=blue&left_text=pypi%20downloads)](https://pepy.tech/project/jsbsim)
[![Conda (channel only)](https://img.shields.io/conda/vn/conda-forge/jsbsim)](https://anaconda.org/conda-forge/jsbsim)

<p align="center">
<img width="250" heigth="250" src="https://github.com/JSBSim-Team/jsbsim-logo/blob/master/logo_JSBSIM_globe.png">
</p>

**[User Guide](#user-guide)**               |
**[Developer Docs](doc/DevelopersDocs.md)** |
**[FAQ](#frequently-asked-questions)**      |
**[Contact](#contact)**                     |
**[Legal notice](#legal-notice)**
---

# Introduction
JSBSim is a multi-platform, general purpose object-oriented Flight Dynamics Model (FDM) written in C++. The FDM is essentially the physics & math model that defines the movement of an aircraft, rocket, etc., under the forces and moments applied to it using the various control mechanisms and from the forces of nature. JSBSim can be run in a standalone batch mode flight simulator (no graphical displays) for testing and study, or integrated with [FlightGear](https://www.flightgear.org/) or other flight simulator.
 
Features include:

* Nonlinear 6 DoF (Degree of Freedom)
* Fully configurable flight control system, aerodynamics, propulsion, landing gear arrangement, etc. through XML-based text file format.
* Accurate Earth model including:
  * Rotational effects on the equations of motion (Coriolis and centrifugal acceleration modeled).
  * Oblate spherical shape and geodetic coordinates according to the [WGS84 geodetic system](https://en.wikipedia.org/wiki/World_Geodetic_System).
  * Atmosphere modeled according to the [International Standard Atmosphere (1976)](https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19770009539.pdf).
* Configurable data output formats to screen, file, socket, or any combination of those.
* A [Python](https://www.python.org) module which provides the exact same features than the C++ library
* A [Matlab](https://www.mathworks.com/products/matlab.html) S-Function to interface JSBSim with [Simulink](https://fr.mathworks.com/products/simulink.html).

In 2015, [the NASA performed some verification check cases on 7 flight dynamics software **including JSBSim**](https://nescacademy.nasa.gov/flightsim) (the other 6 being NASA in-house software). The results showed that the 7 simulation tools *"were good enough to indicate agreement between a majority of simulation tools for all cases published. Most of the remaining differences are explained and could be reduced with further effort."*

## Applications and Usages
JSBSim is used in a range of projects among which:

* Flight simulation: [FlightGear](http://www.flightgear.org), [OutTerra](https://www.outerra.com/wfeatures.html), [Skybolt Engine](https://github.com/Piraxus/Skybolt)
* SITL (Software In The Loop) Drone Autopilot testing : [ArduPilot](https://ardupilot.org/dev/docs/sitl-with-jsbsim.html), [PX4 Autopilot](https://dev.px4.io/master/en/simulation/jsbsim.html), [Paparazzi](https://wiki.paparazziuav.org/wiki/Simulation)
* Machine Learning Aircraft control: [gym-jsbsim](https://github.com/galleon/gym-jsbsim)
* [DARPA Virtual Air Combat Competition](https://www.darpa.mil/news-events/2019-10-21) where one of the AI went undefeated in five rounds of mock air combat against an Air Force fighter (see the [video on YouTube](https://www.youtube.com/watch?v=IOJhgC1ksNU)).

JSBSim is also used in academic and industry research ([more than 600 citations referenced by Google Scholar](https://scholar.google.com/scholar?&q=jsbsim) as of November 2020).

# User Guide
## Installation
### Windows
A Windows installer `JSBSim-1.1.8-setup.exe` is available in the [release section](https://github.com/JSBSim-Team/jsbsim/releases/tag/v1.1.8). It installs the 2 executables along with aircraft data and some example scripts:
* `JSBSim.exe` which runs FDM simulations.
* `aeromatic.exe` which builds aircraft definitions from Question/Answer interface

Both executables are console line command.
### Ubuntu Linux
Debian packages for Ubuntu Linux "Xenial" 16.04 LTS, "Bionic" 18.04 LTS and "Focal" 20.04 LTS for 64 bits platform are also available in the [JSBSim project release section](https://github.com/JSBSim-Team/jsbsim/releases/tag/v1.1.8). There are 3 packages:
* `JSBSim_1.1.8-588.amd64.deb` which installs the executables `JSBSim` and `aeromatic`
* `JSBSim-devel_1.1.8-588.amd64.deb` which installs the development resources (headers and libraries)
* `python3-JSBSim_1.1.8-588.amd64.deb` which installs the Python 3.6 module of JSBSim
### Python module
JSBSim provides binary packages for its Python module on Windows, Mac OSX and Linux platforms for several Python versions (3.6, 3.7, 3.8 and 3.9). These can be installed using either `pip` or `conda`.
#### Installation with `pip`
Binary packages a.k.a. known as [wheel packages](https://www.python.org/dev/peps/pep-0427) are available from the [Python Package Index (PyPI)](https://pypi.org), a repository of software for the Python programming language. 

Installing `jsbsim` using `pip` can be achieved with:
```bash
> pip install jsbsim
```
Check the [pip documentation](https://packaging.python.org/tutorials/installing-packages) for more details.

Note that wheel packages for Linux meet the [ManyLinux packages requirements](https://www.python.org/dev/peps/pep-0513) and as such are compatible with all the major Linux distributions.
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
At the moment, JSBSim do not provide binaries for platforms other than Windows 64 bits and Ubuntu 64 bits. If you fall in this category you should follow the instructions in the [developer docs](doc/DevelopersDocs.md) to build JSBSim on your platform.
### Aircraft data and example scripts
You can get aircraft data and example scripts by downloading either the [zip package](https://github.com/JSBSim-Team/jsbsim/archive/v1.1.8.zip) or the [tar.gz package](https://github.com/JSBSim-Team/jsbsim/archive/v1.1.8.tar.gz).
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
A first place to look at for JSBSim documentation resources is [http://jsbsim.sourceforge.net/documentation.html](http://jsbsim.sourceforge.net/documentation.html). This link points to the official [JSBSim Reference Manual](http://jsbsim.sourceforge.net/JSBSimReferenceManual.pdf), a PDF which is the best source of information for users and developers.

However, due to the nature of the development of the project (JSBSim sources are updated often, sometimes even daily), several new features that are available in the software are not yet documented in the reference manual. Starting from March 2018 a new effort is underway to deliver an up-to-date documentation web site. You can browse the new *JSBSim Online Reference Manual* by going to: [https://jsbsim-team.github.io/jsbsim-reference-manual](https://jsbsim-team.github.io/jsbsim-reference-manual). The online manual is under construction and as a first milestone it will incorporate all the non-outdated material contained in the original PDF Reference Manual. The online manual web site is based on the GitHub Pages technology and its sources are available [here](https://github.com/JSBSim-Team/jsbsim-reference-manual). Eventually, the PDF Reference Manual will be superseded by the online manual, which is designed to be updated collaboratively as well as in efficient and timely fashion.

## Interfacing JSBSim with your application
### Using the C++ API
JSBSim can be interfaced or integrated to your application via a C++ API. The  following  code  illustrates  how  JSBSim  could  be  called  by  a  small  program, with execution being controlled by a script: 
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
JSBSim can also be used as a Python module. The following code provides a simple example of how to interface with JSBSim using the Python programming language:
```python
import jsbsim

fdm = jsbsim.FGFDMExec('.', None)
fdm.load_script('scripts/c1721.xml')
fdm.run_ic()

while fdm.run():
  pass
```
A more elaborate example is [JSBSim.py](https://github.com/JSBSim-Team/jsbsim/blob/master/python/JSBSim.py), the Python version of `JSBSim.exe`.

# Frequently Asked Questions

### How can I interface JSBSim with FlightGear ?
**Q:** I would like to visualize the aircraft model in [FlightGear](http://flightgear.org) while running the FDM with the JSBSim executable. How do I proceed ?

**A:** We are assuming that FlightGear is installed on your platform. If it is not, please follow the instructions on the [FlightGear website](http://home.flightgear.org/download/main-program).

You will need to launch separately FlightGear and JSBSim from a console. In the example below, you will execute the script [ShortS23_2.xml](scripts/Short_S23_2.xml) so you might need to install the [Short Empire aircraft](http://wiki.flightgear.org/Short_Empire). This is optional however and any aircraft can be used for visualization even if it does not correspond to the FDM.

First, run FlightGear and tell it that the flight dynamics will be provided thru a socket by an external program. It is assumed that the executable of FlightGear is `fgfs` (see the FlightGear docs for [details on the parameters of `--native-fdm`](http://wiki.flightgear.org/Property_Tree/Sockets#Native_Socket) argument).
```bash
> fgfs --fdm=null --native-fdm=socket,in,60,,5550,udp --aircraft=Short_Empire --airport=SP01
```
Once FlightGear is launched, you will see the aircraft standing still.

![FlightGear is launched](doc/JSB2FG_interface_1.png)

Now we will run JSBSim and tell it that it must send the flight dynamics data to a socket with the FlightGear protocol
```bash
> JSBSim scripts/Short_S23_2.xml data_output/flightgear.xml --realtime --nice
```
The parameters describing the protocol are detailed in [`data_output/flightgear.xml`](data_output/flightgear.xml). The flag `--realtime` requests JSBSim to execute in real time. Otherwise JSBSim will run as fast as it can and the flight will look like it is played fast forward. The flag `--nice` tells JSBSim to use as few CPU power as possible. This is an optional flag but since we requested `--realtime`, JSBSim will spend a considerable amount of time idling, waiting for the next time step.

At the this stage, the two executables are interacting and FlightGear produces the visualization of the flight dynamics simulated by JSBSim.

![FlightGear/JSBSim interface running](doc/JSB2FG_interface_2.png)

# Contact
For more information on JSBSim, you can contact the development team on the mailing list jsbsim-devel@lists.sourceforge.net or submit tickets on https://github.com/JSBSim-Team/jsbsim/issues

We are also on Facebook: https://www.facebook.com/jsbsim/

# Legal Notice
JSBSim is open source and is licensed under the LGPL 2.1 license. The license is included in the source code in the file [COPYING](https://github.com/JSBSim-Team/jsbsim/blob/master/COPYING)

No proprietary code is included. All code included within JSBSim has been developed on a volunteer basis using publicly available information, and is often directly linked to a particular textbook, for educational reference. In some cases, code of a generic nature has been donated back to the project.

Likewise, the aircraft models included in this project and distribution do not include any proprietary, sensitive, or classified data. All data is derived from textbooks (such as Stevens and Lewis "Aircraft Control and Simulation" and Sutton's "Rocket Propulsion Elements"), freely available technical reports (see: http://ntrs.nasa.gov and http://www.aiaa.org), or other public data (such as the FAA web site). Aircraft models included in the JSBSim distribution and with names corresponding to existing commercial or military aircraft are approximations crafted using publicly available information, and are for educational or entertainment uses only. 
