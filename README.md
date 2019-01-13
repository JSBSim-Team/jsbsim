| Travis CI (Linux) | AppVeyor CI (Windows) |
|:-----------------:|:---------------------:|
| [![Travis CI](https://travis-ci.org/JSBSim-Team/jsbsim.svg?branch=master)](https://travis-ci.org/JSBSim-Team/jsbsim) | [![Build status](https://ci.appveyor.com/api/projects/status/89wkiqja63kc6h2v/branch/master?svg=true)](https://ci.appveyor.com/project/agodemar/jsbsim/branch/master) |

<p align="center">
<img width="250" heigth="250" src="https://github.com/JSBSim-Team/jsbsim-logo/blob/master/logo_JSBSIM_globe.png">
</p>

**[User Guide](#user-guide)**           |
**[Developer Docs](DevelopersDocs.md)** |
**[FAQ](#frequently-asked-questions)**  |
**[Contact](#contact)**                 |
**[Legal notice](#legal-notice)**
---

# Introduction
JSBSim is a multi-platform, general purpose object-oriented Flight Dynamics Model (FDM) written in C++. The FDM is essentially the physics & math model that defines the movement of an aircraft, rocket, etc., under the forces and moments applied to it using the various control mechanisms and from the forces of nature. JSBSim can be run in a standalone batch mode flight simulator (no graphical displays) for testing and study, or integrated with [FlightGear](http://home.flightgear.org/) or other flight simulator.
 
Features include:

* Fully configurable flight control system, aerodynamics, propulsion, landing gear arrangement, etc. through XML-based text file format.
* Rotational earth effects on the equations of motion (Coriolis and centrifugal acceleration modeled).
* The earth atmosphere is modeled according to the [International Standard Atmosphere (1976)](https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19770009539.pdf)
* Configurable data output formats to screen, file, socket, or any combination of those.

More information on JSBSim can be found at the JSBSim home page here:

http://www.jsbsim.org

# User Guide
**[Installation](#installation)** |
**[Quick Start](#quick-start)**   |
**[User Docs](#user-documentation)** |
**[Interfacing JSBSim with your app](#interfacing-jsbsim-with-your-application)**
---
## Installation
### Windows
Win64 executables for JSBSim are available in the [JSBSim project release section](https://github.com/JSBSim-Team/jsbsim/releases). There are 2 executables:
* `JSBSim.exe` which runs FDM simulations.
* `aeromatic.exe` which builds aircraft definitions from Question/Answer interface

Both executables should be used from the console.
### Ubuntu Linux
Debian packages for Ubuntu Linux "Trusty" 14.04 on 64 bits platform are also available in the [JSBSim project release section](https://github.com/JSBSim-Team/jsbsim/releases). There are 3 packages:
* `JSBSim_1.0.0-xxx.trusty.amd64.deb` which installs the executables `JSBSim` and `aeromatic`
* `JSBSim-devel_1.0.0-xxx.trusty.amd64.deb` which installs the development resources (headers and libraries)
* `pythonX-JSBSim_1.0.0-xxx.trusty.amd64.deb` which installs the Python module of JSBSim (for python 2.7 if the package name starts with **python2** or 3.4 if it starts with **python3**)
### Other platforms
At the moment, JSBSim do not provide binaries for platforms other than Windows 64 bits and Ubuntu 14.04 64 bits. If you fall in this category you should follow the instructions in the [developer docs](#developer-documentation) to build JSBSim on your platform.
### Aircraft data and example scripts
You can get aircraft data and example scripts by downloading either the [zip package](https://github.com/JSBSim-Team/jsbsim/archive/JSBSim-trusty-v2018a.zip) or the [tar.gz package](https://github.com/JSBSim-Team/jsbsim/archive/JSBSim-trusty-v2018a.tar.gz).
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
JSBSim can be interfaced or integrated to your application via a C++ API. The  following  code  illustrates  how  JSBSim  could  be  called  by  a  small  program, with execution being controlled by a script: 
```c++
#include <FGFDMExec.h> 

int main(int argc, char **argv) 
{ 
  JSBSim::FGFDMExec FDMExec; 
  bool result = true; 
  FDMExec.LoadScript(argv[1]); 
  while (result) result = FDMExec.Run();
}
 ```
The API is described in more details in the [C++ API documentation](#c-api-documentation)

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
