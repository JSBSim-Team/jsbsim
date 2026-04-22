---
title: 'JSBSim, An Open Source Flight Dynamics Software Library'
tags:
  - Flight simulation
  - Flight dynamics modeling
  - Aerodynamic modeling
  - Propulsion modeling
  - C++
  - Python
  - Julia
  - MATLAB/Simulink
  - Pathsim
  - Pathview
authors:
  - name: Jon S. Berndt
    orcid: 0009-0009-9701-2182
    affiliation: 1 
  - name: Bertrand Coconnier
    orcid: 0009-0000-0065-3704
    affiliation: 2 
  - name: Agostino De Marco
    orcid: 0000-0001-5985-9950
    affiliation: 3 
  - name: Sean McLeod
    orcid: 0009-0003-1039-8307
    affiliation: 4 
affiliations:
 - name: Jon's affiliation
   index: 1
 - name: Bertrand's affiliation
   index: 2
 - name: University of Naples Federico II, Italy
   index: 3
 - name: Professional Software Developer
   index: 4
date: XY april 2026
bibliography: paper.bib 
---

# Summary

JSBSim is an open-source, platform-independent, data-driven flight dynamics software library for aerospace research, simulation development, and education. It provides a high-fidelity, fully scriptable environment for modeling aircraft dynamics, propulsion, control systems, and flight conditions. 

JSBSim can be used in batch mode running faster than real-time for flight analysis or AI training or run within a flight simulator environment like FlightGear in real-time.

Features include:

- Rigid body dynamics with support for 6 DoF.
- Fully configurable aerodynamics, flight control system, propulsion, landing gear arrangement, etc. through XML-based text file format.
- Accurate Earth model including:
   - Rotational effects on the equations of motion (Coriolis and centrifugal acceleration modeled).
   - Oblate spherical shape and geodetic coordinates according to the WGS84 geodetic system.
   - Atmosphere modeled according to the International Standard Atmosphere (1976).
- Configurable data output formats to screen, file, socket, or any combination of those.

Developed in standard-compliant C++17, JSBSim also includes the following bindings and interfaces:

- Python module (compatible with Python 3.10+)
- MATLAB S-Function that interfaces JSBSim with MATLAB Simulink
- Julia bindings
- Unreal Engine plugin

# Statement of Need

Aerospace researchers, instructors, and engineers often need a flight dynamics model (FDM) that is scientifically credible and openly accessible. Existing FDMs are either  proprietary, tightly integrated with specific simulators, or lack extensibility for custom modeling, automated testing, or integration into research pipelines. Others have been in development for so long that they have become difficult to adapt or even to understand. JSBSim fills this gap by offering a standalone, open-source FDM with a clear architecture, straightforward and predictable behavior, and a long history in academic, government, and open-source projects. 

JSBSim has been in development since 1996; in 2018, its codebase was moved to GitHub under the organization JSBSim-Team.

The JSBSim XML-based model definitions support validation, and scriptable running supports reproducibility.


# Early Motivation

While electro-mechanical flight *trainers** (such as Link’s Blue Box) have been around for almost 100 years, flight simulation codebases have been around since the mid-70s, beginning with Bruce Artwick’s first foray into computer-based flight simulation, as part of his engineering thesis - fifty years ago! Some of the earliest codebases were written in Fortran and evolved over the years into very capable and trusted tools. However, over the years, additions to those codebases by various contributors resulted in code that was less cohesive, brittle, and hard to read.

The C++ programming language emerged in the mid-1980’s and began to see widespread use due in part to its support for object-oriented design concepts. In the mid-1990’s after having worked for ten years on flight simulation tasks involving older, hard to read and use legacy code, the original JSBSim developer thought, “there’s got to be a better way,” and began experimenting with flight simulation code in C++, which seemed to be a very well-suited language for flight simulation. 

Almost 30 years later, and with the participation of many contributors and collaborators, JSBSim is a great example of what the Open Source paradigm can achieve. 

# Development and Design Choices

JSBSim was designed from the ground up with several features in mind. One was to make the codebase easily comprehensible and expandable, and another was to completely separate the characteristics of a specific vehicle from a completely generic codebase. This was done in part to keep possibly proprietary information out of the codebase. 

In a nutshell, the flow of the code can be illustrated as follows:

[diagram or explanation of the architecture of the code and how it is instantiated]

JSBSim is data-driven, with all specific model characteristics contained in data files. Here’s a “Hello World” view of a minimal JSBSim invocation, modeling a ball in low Earth orbit. First, here is the XML file containing the characteristics of the “vehicle” - here, just a ball - in the file named minimal_ball.xml:

```xml
<?xml version="1.0"?>
<fdm_config name="Ball" version="2.0">

  <metrics>
    <wingarea unit="FT2"> 1.0 </wingarea>
    <wingspan unit="FT"> 1.0 </wingspan>
    <chord unit="FT"> 1.0 </chord>
    <location name="AERORP" unit="IN">
      <x> 0.0 </x> <y> 0.0 </y> <z> 0.0 </z>
    </location>
  </metrics>

  <mass_balance>
    <ixx unit="SLUG*FT2"> 1.0 </ixx>
    <iyy unit="SLUG*FT2"> 1.0 </iyy>
    <izz unit="SLUG*FT2"> 1.0 </izz>
    <emptywt unit="LBS"> 10.0 </emptywt>
  </mass_balance>

  <output name="BallOut.csv" type="CSV" rate="1">
    <position> ON </position>
  </output>

</fdm_config>
```

We also have to tell JSBSim, at time zero, where to place the ball in space, and how fast it is going, in which direction, etc. That is contained in the file, reset00_v2.xml:

```xml
<?xml version="1.0"?>
<initialize name="reset00" version="2.0">

  <!-- This file sets up the spacecraft to start off
       at altitude and orbital velocity. -->

  <position frame="ECEF">
    <altitudeMSL unit="FT"> 800000.0  </altitudeMSL>
  </position>

  <orientation unit="DEG" frame="LOCAL">
    <yaw>   90.0  </yaw>
  </orientation>

  <velocity unit="FT/SEC" frame="BODY">
    <x> 23889.145167 </x>
  </velocity>

  <attitude_rate unit="DEG/SEC" frame="ECI">
    <x> 0.0 </x>
    <y> 0.0 </y>
    <z> 0.0 </z>
  </attitude_rate>

</initialize>
```

And here is how we invoke the batch version of JSBSim from the command line, which reads the above inputs:

```sh
./JSBSim --end=5400 --aircraft=minimal_ball --initfile=reset00_v2
```

Running the above command results in the ball characteristics being read in, placed at the state specified in the  reset00_v2.xml file, and running for 5400 seconds. The position of the ball is logged at 1 Hz in a file named BallOut.csv.

[further explanation of JSBSim]

# Use Cases and Research Applications

JSBSim is used across a broad range of aerospace applications, including flight control development, UAV research, aircraft design studies, and simulation-based testing. It's use in academic and industry research has resulted in over 1000 citations as per Google Scholar, and it has been integrated into several popular flight simulators and research platforms.

Examples of use cases include:

- Modeling flight dynamics within a full-featured flight simulator, such as FlightGear, MIXR (formerly known as OpenEaagles), the Outerra world simulator, or Epic Games’ Unreal Engine 5. 

- Reinforcement learning research, where JSBSim is used as the environment in which an agent learns to control an aircraft. One example being it's use in the [DARPA Virtual Air Combat Competition](https://www.darpa.mil/news/2019/virtual-air-combat-competition).

- SITL (Software In The Loop) Drone autopilot testing:  [ArduPilot](https://ardupilot.org/dev/docs/sitl-with-jsbsim.html), [PX4 Autopilot](https://docs.px4.io/main/en/sim_jsbsim/), [Paparazzi](https://wiki.paparazziuav.org/wiki/Simulation)

- [additional examples here, look at some of the Google Scholar citations for different/interesting use cases]


# Implementation and Engineering Practices

A key requirement of an FDM is accuracy, as would be expected. That is, the underlying math model of rigid body motion needs to be implemented properly. But how can one verify this? One way is through comparison with other similar flight simulation applications. To this end, the NASA Engineering Safety Center undertook an effort in 2015 to develop a set of check cases that could serve as a basis for comparing time-history data across simulations. JSBSim was included in this effort as the only non-NASA simulation. 

[description of JSBSim development and version management, etc.]

# Acknowledgements

JSBSim is currently being maintained and developed by Bertrand Coconnier, Sean McLeod, and Agostino De Marco, along with contributions from the broader community. Initial architecture and development was done by Jon Berndt, with major contributions from Tony Peden, David Megginson, and David Culp. Initial integration into the FlightGear open source flight simulator was assisted by Curt Olson. 

# Authors

Jon S. Berndt

[Add other names as applicable]

Bertrand Coconnier

Agostino De Marco

Sean McLeod

# References

Jon Berndt. 
["JSBSim: An Open Source Flight Dynamics Model in C++,"](https://arc.aiaa.org/doi/10.2514/6.2004-4923) AIAA 2004-4923. AIAA Modeling and Simulation Technologies Conference and Exhibit. August 2004

Jon Berndt and Agostino De Marco. ["Progress On and Usage of the Open Source Flight Dynamics Model Software Library, JSBSim,"](https://arc.aiaa.org/doi/10.2514/6.2009-5699) AIAA 2009-5699. AIAA Modeling and Simulation Technologies Conference. August 2009

Daniel G. Murri, E. Bruce Jackson, Robert O. Shelton. ["Check-Cases for Verification of 6-Degree-of-Freedom Flight Vehicle Simulations,"](https://ntrs.nasa.gov/api/citations/20150001263/downloads/20150001263.pdf) NASA/TM-2015-218675/Volume I
