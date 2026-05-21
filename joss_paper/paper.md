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
 - name: Independent Developer, United States
   index: 1
 - name: Independent Developer, France
   index: 2
 - name: Professor of Flight Mechanics, University of Naples Federico II, Italy
   index: 3
 - name: Independent Developer, South Africa
   index: 4
date: XY april 2026
bibliography: paper.bib
---

# Summary

JSBSim is an open-source, platform-independent, data-driven flight dynamics software library for aerospace research, simulation development, and education. It provides high-fidelity modeling of aerospace vehicle dynamics, and includes propulsion, control systems, aerodynamics, and other subsystem models, as well as environmental models such as atmosphere, gravity, and geodesy.

JSBSim can be invoked from the command line — running faster than real time for scripted flight scenarios, Monte Carlo studies, or AI training — or it can be integrated into an interactive simulator codebase such as FlightGear, Unreal Engine, or Outerra and run in real time.

Features include:

* Rigid body dynamics with support for 6-degrees-of-freedom (6-DoF) simulations.
* Quaternion-based computation of the aircraft attitude to avoid the gimbal lock of Euler angles.
* Accurate environment models (geodesy, atmosphere, rotational planet effects).
* Fully configurable input model characteristics and output logging.
* Developed in standard-compliant C++17, with bindings for Python, and MATLAB (includes a Simulink S-Function).

JSBSim has been in active development for almost 30 years.

# Statement of Need

JSBSim emerged from the need for an open, modern, clean, extensible flight dynamics model (FDM). Aerospace researchers, instructors, and engineers often need an FDM that is scientifically credible and openly accessible. Existing FDMs are either proprietary, tightly integrated with specific simulators, or lack extensibility for custom modeling, automated testing, or integration into research pipelines. Others have been in development for so long that they have become difficult to adapt or even to understand. JSBSim fills this gap by offering a standalone, open-source FDM with a clear architecture, straightforward and predictable behavior, and a long history in academic, government, and open-source projects.

The JSBSim XML-based model definitions support validation, and scriptable running supports testing and reproducibility.

# State of the Field

In the research community, several tools are currently used to model flight dynamics, but they are often unavailable for public use or present significant limitations for automated research or high-fidelity academic studies. The [Trick simulation environment](https://github.com/nasa/trick), developed at NASA Johnson Space Center, was released as open source software in 2015. [POST2](https://www.nasa.gov/post2/) - successor to POST, Program to Optimize Simulated Trajectories, developed over 50 years ago — is still used internally within NASA to this day, as a trusted and well-verified tool.
Within the open-source and free-to-use landscape, [YASim](https://wiki.flightgear.org/YASim) (also used in [FlightGear](https://www.flightgear.org)) offers a quick-solving approach where the FDM is automatically generated from geometry and performance points using Blade Element Theory. While efficient for rapid prototyping, it generates plausible but low-fidelity models.
[X-Plane](https://www.x-plane.com/) also uses a fast, low-fidelity aerodynamic model, but it cannot run natively in a headless mode. Its closed-source nature makes it difficult to extend or integrate deeply into custom research pipelines without complex, third-party interfaces.
Engineering-focused tools like the [MATLAB Aerospace Toolbox](https://www.mathworks.com/products/aerospace-toolbox.html) provide high-fidelity components but are proprietary and require the MATLAB/Simulink ecosystem.

# Unique Scholarly Contribution

JSBSim’s unique contribution lies in providing a standalone traditional coefficient-based, “architecture-light,” FDM that bridges the gap between the alternatives mentioned above. Unlike YASim and X-Plane, JSBSim primarily uses stability derivatives to model vehicle aerodynamics. Stability derivatives can be constant or functionally dependent on other state and input variables. However, the use of stability derivatives is not the only option available to FDM authors. The XML-based metalanguage provided by JSBSim allows for the use of general, nonlinear, and multidimensional lookup tables to model any type of behavior within the FDM.

By providing a complete, turnkey solution, JSBSim allows researchers to bypass the complexity of building a simulation engine from scratch and focus entirely on their specific area of study. Furthermore, for research topics that require modeling beyond the standard stability derivative approach, JSBSim’s `external_reactions` feature offers the flexibility to interface with any external aerodynamic or propulsive simulation (or co-simulation) method, ensuring the library remains extensible for even the most unconventional aerospace designs.

# Software Design

JSBSim was designed from the ground up with several features in mind [@Berndt:2004:JSBSim]. One was to make the codebase easily comprehensible and expandable, and another was to completely separate the characteristics of a specific vehicle from a completely generic codebase. This was done in part to keep possibly proprietary information out of the codebase. With all specific model characteristics contained in data files, there is no need to recompile the code to model a different vehicle, or changes to the vehicle characteristics. This is a key design feature of JSBSim, which allows users to define an entire FDM using XML files—unlike, for example, [LaRCSim](https://ntrs.nasa.gov/citations/19950023906), where modifying aircraft parameters requires writing and re-compiling C code.

To illustrate, here’s a “Hello World” view of a minimal JSBSim invocation that models a ball in low Earth orbit. First, here is the format of a simplified XML file containing the characteristics of the “vehicle” — here, just a ball — in the file named `minimal_ball.xml`:
```xml
<?xml version="1.0"?>
<fdm_config name="Ball" version="2.0">
  <metrics> ... </metrics>
  <mass_balance> ... </mass_balance>
  ...
  <external_reactions> ... </external_reactions>
  ...
  <flight_control> ... </flight_control>
  ...
  <aerodynamics> ... </aerodynamics>
  <output name="BallOut.csv" type="CSV" rate="1"> ... </output>
</fdm_config>
```
The user also has to tell JSBSim, at time zero, where to place the ball in space, and how fast it is going, in which direction, etc. That is contained in the file, `reset00_v2.xml`:
```xml
<?xml version="1.0"?>
<initialize name="reset00" version="2.0">
  <position frame="ECEF"> ... </position>
  <orientation unit="DEG" frame="LOCAL"> ... </orientation>
  <velocity unit="FT/SEC" frame="BODY"> ... </velocity>
  <attitude_rate unit="DEG/SEC" frame="ECI"> ... </attitude_rate>
</initialize>
```
And here is how the batch version of JSBSim can be invoked from the command line:
```sh
./JSBSim --end=5400 --aircraft=minimal_ball --initfile=reset00_v2
```
where the directives `--aircraft` and `--initfile` instruct the program to read the above inputs. The execution results in the ball characteristics being read, initialized to the state specified in the `reset00_v2.xml` file, and in a state propagation being run from time zero to 5400 simulated seconds. The instantaneous position of the ball is logged at 1 Hz in a file named `BallOut.csv` (see the `<output>` block in the file `minimal_ball.xml`). The output file can be quickly read and examined using a wide range of commonly available scientific visualization tools.

While this is a minimal example, JSBSim scales to highly complex aerospace vehicles — even rockets with GNC systems and large aerodynamic databases derived from wind tunnel testing — all specified through data files alone. In fact, the input metalanguage offers great flexibility in terms of defining properties within a specific FDM, with the availability of a large number of mathematical operators, interpolating functions from data arranged in tabular form, and access to the aircraft's metrics and state via the property system.
In the following example the pitch moment due to elevator (linearly dependent on its deflection $\delta_{\mathrm{e}}$) is defined in terms of a control power coefficient $C_{m_{\delta_\mathrm{e}}}$, function of Mach number $M_{\infty}$ (see also \autoref{fig:Cm_delta:e}):
```xml
<function name="aero/PitchMoment_elevator">
    <description>Pitch moment due to elevator</description>
    <product>
        <property>aero/qbar-psf</property>
        <property>metrics/Sw-sqft</property>
        <property>metrics/cbarw-ft</property>
        <property>fcs/elevator-pos-rad</property>
        <table> <!-- 1D tabular function -->
            <independentVar>velocities/mach</independentVar>
            <tableData> <!-- lookup table - Cmde coefficient -->
                0.0 -1.20
                1.0 -1.00
                2.0 -0.30
            </tableData>
        </table>
    </product>
</function>
```
![Pitching control power coefficient as a tabulated function of Mach number. See `<table>` element in the above snippet.\label{fig:Cm_delta:e}](assets/PitchMoment_elevator.png)

The more common execution from the command line involves running from a script, which interacts with the simulation by modifying properties based on conditional logic. While JSBSim handles the continuous physics of flight, the script acts as a state-machine-driven mission controller, providing the discrete logical transitions required to navigate complex flight scenarios. JSBSim scripts are coded as XML-based input files, with their specific metalanguage.

# A Selected Example of Use in Research: Flight Load Assessment for Light Aircraft Flying near Wind Turbine Wake

JSBSim has been used by @Varriale:DeMarco:2018:Flight:Load:Assessment to assess the flight loads on light aircraft flying through or nearby wind turbine wakes. For this research, a framework of software applications has been developed for generating and controlling a population of flight simulation scenarios in the presence of CFD calculated wind and turbulence fields. JSBSim's autopilot system has been used to simulate realistic pilot behavior during navigation. 
\autoref{fig:wake:crossing} shows the top view of a selected flight scenario (left). The figure also reports the time histories of the normal relative-wind component ($V_{\mathrm{w},z_\mathrm{B}}$, along z-body axis) induced by the turbine wake, and the normal load factor $n_{z_\mathrm{B}}$. Based on these results, preliminary guidelines and recommendations on safe encounter distances have been provided for general aviation aircraft when flying in the proximity of wind farms.

![Normal relative-wind component and aircraft normal load factor. Results adapted from @Varriale:DeMarco:2018:Flight:Load:Assessment.\label{fig:wake:crossing}](assets/turbine_wake_crossing_scheme_plots.png)

# Implementation and Engineering Practices

A key requirement of an FDM is accuracy. The JSBSim equations of motion were verified through comparison with other similar flight simulation applications. The [NASA Engineering Safety Center](https://www.nasa.gov/nesc) undertook an effort in 2015 to develop a set of check cases that could serve as a basis for comparing time-history data across simulations. JSBSim was included in this effort as the only non-NASA simulation software [@Murri:2015:Check:Cases].

The library also leverages the broader open-source ecosystem by integrating mature and specialized components. For XML parsing, JSBSim relies on [Expat](https://libexpat.github.io/), a foundational and industry-standard library in the open-source community. Additionally, complex geodesic calculations required for high-fidelity trajectory modeling on the WGS84 oblate spheroid are performed using Charles Karney’s [GeographicLib](https://geographiclib.sourceforge.io/), ensuring precision in geospatial positioning and navigation [@Karney:2025:GeographicLib].

JSBSim adheres to modern open-source Quality Assurance (QA) standards through an extensive Continuous Integration and Continuous Deployment. Every commit and Pull Request undergoes automated builds and testing across all major supported platforms. This pipeline also tracks code coverage to monitor testing depth. Furthermore, the CD workflow automates the release process.

# Research Impact Statement

JSBSim is used across a broad range of aerospace applications, including flight control development, UAV research, aircraft design studies, and simulation-based testing. It’s use in academic and industry research has resulted in over 1000 citations as per Google Scholar, and it has been integrated into several popular flight simulators and research platforms. In the existing scientific literature, the key works on JSBSim are those by @Berndt:2004:JSBSim, @DeMarco:2007:General:Solution:Trim, @Berndt:DeMarco:2009:Progress:JSBSim, @Murri:2015:Check:Cases.

Examples of use cases include:

- Modeling flight dynamics within a full-featured flight simulator, such as [FlightGear](https://www.flightgear.org), [MIXR (Mixed Reality Simulation Platform)](https://www.mixr.dev) (formerly known as OpenEaagles), the [Outerra world simulator](https://outerra.com), or [Epic Games' Unreal Engine 5](https://www.unrealengine.com/unreal-engine-5).

- Control system design. See the articles by @Vogeltanz:2018:Development:Control:System:Designer and @Vogeltanz:2020:Control:System:Designer.

- Reinforcement learning research, where JSBSim is used as the environment in which an agent learns to control an aircraft. One example being it's use in the [DARPA Virtual Air Combat Competition](https://www.darpa.mil/news/2019/virtual-air-combat-competition). See also the works by @DeMarco:2023:DRL:Hight:Performance:Aircraft, @Pope:2023:Hierarchical:RL:DARPA:Trials, @Chen:2026:Physics:Informed:Target:Aiming.

- SITL (Software In The Loop) Drone autopilot testing: [ArduPilot](https://ardupilot.org/dev/docs/sitl-with-jsbsim.html), [PX4 Autopilot](https://docs.px4.io/main/en/sim_jsbsim/), [Paparazzi](https://wiki.paparazziuav.org/wiki/Simulation).

- UAV modeling. See @Kamal:2016:Modeling:Flight:Simulation:UAV, @Cereceda:2019:Giant:BigStik.

- Rocket trajectory simulations. See @Gomez:2003:Active:Guidance, @Braun:2006:Design:ARES, @Kenney:2011:Flight:Simulation:ARES.

- Sensor assessment and Human Factor. See @Zhang:2010:Mathematical:Models:Pilot, @McAnanama:2018:OpenSource:FDM:IMU.

- Simulation integration. See @Park:2008:Experimental:Evaluation:UAV:TMO, @Nicolosi:DeMarco:2018:Roll:Performance:Assessment, @Saber:2025:Integration:JSBSim:Unreal.

- CPU performance benchmarking. JSBSim has been included in the [SPEC CPU 2026](https://www.spec.org/cpu2026/) [@Madhav:2026:SPEC:CPU:Next:Generation], a widely recognized benchmark suite designed to measure the performance of a computer's processor, memory, and compiler efficiency using compute-intensive workloads.

JSBSim’s versatility is further expanded by its blossoming Python ecosystem, which has seen over one million cumulative downloads across PyPI and Conda. While its established C++ integration remains fundamental, there is a significant growth in users leveraging Python’s ubiquity in the research, engineering, and AI communities. This trend is reflected in a growing number of applications that utilize interactive notebooks to provide a didactic interface, effectively lowering the barrier to entry. Moreover, Python enables a more dynamic approach to simulation, allowing users to complement traditional XML scripting with complex programmatic scenarios and to develop sophisticated aerodynamic or propulsion models. A great potential of the JSBSim Python API is confirmed, for instance, by its straightforward usability within applications based on [PathSim](https://pathsim.org/) [@Rother:2025:PathSim:JOSS] and its companion [PathView](https://view.pathsim.org/); the latter being a Python native environment with a visual toolbox capable of modeling simulation workflows, as noncommercial alternative to MATLAB/Simulink or Modelica.

# Acknowledgements

JSBSim is currently being maintained and developed by Bertrand Coconnier, Sean McLeod, and Agostino De Marco, along with contributions from the broader community. Initial architecture and development was done by Jon Berndt, with major contributions from Tony Peden, David Megginson, and David Culp. Initial integration into the FlightGear open source flight simulator was assisted by Curt Olson.

# AI Usage Disclosure 

During the preparation of this manuscript, AI‑based tools were used basically to support clarity, language refinement, and the management of bibliographic information. All outputs were critically reviewed and edited by the authors. The authors remain fully responsible for the content of the manuscript, including its accuracy, validity, and compliance with ethical and scholarly standards.
In terms of coding, data generation and scientific claims—due to the fact that JSBSim largely predates the advent of AI—most of the software development has been made by humans and all the contributions are reviewed by humans.
 
# References
