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
 - name: Professor of Flight Mechanics, University of Naples Federico II, Italy
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

The JSBSim XML-based model definitions support validation, and scriptable running supports reproducibility.


# Early Motivation

While electro-mechanical flight *trainers* (such as Link’s Blue Box) have been around for almost 100 years, flight simulation codebases have been around since the mid-70s, beginning with Bruce Artwick’s first foray into computer-based flight simulation, as part of his engineering thesis - fifty years ago! Some of the earliest codebases were written in Fortran and evolved over the years into very capable and trusted tools. However, over the years, additions to those codebases by various contributors resulted in code that was less cohesive, brittle, and hard to read.

The C++ programming language emerged in the mid-1980’s and began to see widespread use due in part to its support for object-oriented design concepts. In the mid-1990’s after having worked for ten years on flight simulation tasks involving older, hard to read and use legacy code, the original JSBSim developer thought, “there’s got to be a better way,” and began experimenting with flight simulation code in C++, which seemed to be a very well-suited language for flight simulation. 

Almost 30 years later, and with the participation of many contributors and collaborators, JSBSim is a great example of what the Open Source paradigm can achieve. 

# Development and Design Choices

JSBSim was designed from the ground up with several features in mind. One was to make the codebase easily comprehensible and expandable, and another was to completely separate the characteristics of a specific vehicle from a completely generic codebase. This was done in part to keep possibly proprietary information out of the codebase [@jsbsim-an-open-source-flight-dynamics-model-in-cpp-2004]. 

In a nutshell, the flow of the code can be illustrated as follows:

[diagram or explanation of the architecture of the code and how it is instantiated]

JSBSim is data-driven, with all specific model characteristics contained in data files, therefore there is no need to recompile the code to model a different vehicle, or changes to the vehicle characteristics. 

## JSBSim FDM Definition

A brief introduction to the various XML based components that make up a JSBSim FDM.

The FDM is defined in one or more XML files which define the mass configuration, ground reactions for the gear, propulsion, the flight control system and the forces and moments for the 6 axes.

### Mass

The `mass_balance` element is used to define the aircraft’s empty weight, the center of gravity at empty weight and the moments of inertia at empty weight.

```xml
<mass_balance negated_crossproduct_inertia="true">
    <ixx unit="SLUG*FT2"> 562000 </ixx>
    <iyy unit="SLUG*FT2"> 1.473e+06 </iyy>
    <izz unit="SLUG*FT2"> 1.894e+06 </izz>
    <ixy unit="SLUG*FT2"> 0 </ixy>
    <ixz unit="SLUG*FT2"> 8000 </ixz>
    <iyz unit="SLUG*FT2"> 0 </iyz>
    <emptywt unit="LBS"> 83000 </emptywt>
    <location name="CG" unit="IN">
        <x> 639 </x>
        <y> 0 </y>
        <z> -40 </z>
    </location>
</mass_balance>
```

Additional mass can be added via `tank` and `pointmass` elements.

```xml
<tank type="FUEL"><! -- Left wing tank -->
    <location unit="IN">
        <x> 520 </x>
        <y> -80 </y>
        <z> -18 </z>
    </location>
    <type>JET-A</type>
    <capacity unit="LBS"> 10200 </capacity>
    <contents unit="LBS"> 10000 </contents>
</tank>
```

A `pointmass` element can also be used to model external stores that can be released during flight.

```xml
<pointmass name="forward-cargo">
    <weight unit="LBS"> 5000 </weight>
    <location name="POINTMASS" unit="IN">
        <x> 157.6 </x>
        <y> 0 </y>
        <z> -39.4 </z>
    </location>
</pointmass>
```

During each simulation timestep JSBSim sums up the current mass of each of the tank elements taking into account any mass loss due to engine fuel burn plus the mass of the set of `pointmass` elements. In addition to calculating the current total mass of the aircraft, the center of gravity is also updated based on the physical location of the various masses and lastly the moments of inertia are also updated.

### Ground Reactions

The `contact` element is used to define either landing gear contacts or structural contacts. JSBSim uses their location, friction coefficients, spring and damping coefficients in order to calculate the forces and moments from their interaction with the ground.

Landing gear contacts (`BOGEY`) also define additional properties in terms of whether they can be used for steering, whether they include brakes and whether they’re retractable.

```xml
<contact name="Left Main Gear" type="BOGEY">
    <location unit="IN">
        <x> 648 </x>
        <y> -100 </y>
        <z> -84 </z>
    </location>
    <static_friction> 0.80 </static_friction>
    <dynamic_friction> 0.50 </dynamic_friction>
    <rolling_friction> 0.02 </rolling_friction>
    <spring_coeff unit="LBS/FT"> 120000 </spring_coeff>
    <damping_coeff unit="LBS/FT/SEC"> 10000 </damping_coeff>
    <damping_coeff_rebound unit="LBS/FT/SEC"> 20000 </damping_coeff_rebound>
    <max_steer unit="DEG"> 0.0 </max_steer>
    <brake_group> LEFT </brake_group>
    <retractable> 1 </retractable>
</contact>
```

A `STRUCTURE` contact can be defined for example to provide a contact point at the rear of the fuselage for performing a velocity minimum unstick $V_{MU}$ simulation flight test.

```xml
<contact type="STRUCTURE" name="TAIL_STRIKE">
    <location unit="IN">
        <x> 924.93864 </x>
        <y> 0 </y>
        <z> 3.41992 </z>
    </location>
    <static_friction> 0.5 </static_friction>
    <dynamic_friction> 0.4 </dynamic_friction>
    <spring_coeff unit="LBS/FT"> 100000 </spring_coeff>
    <damping_coeff unit="LBS/FT/SEC"> 20000 </damping_coeff>
    <brake_group> NONE </brake_group>
    <retractable> 0 </retractable>
</contact>
```

### Aerodynamic Force and Moments

All aerodynamic forces and moments have to specified within the FDM. JSBSim itself doesn’t define any forces or moments. The forces and moments can be specified in one of 3 reference frames, the body axes, stability axes or the wind axes.

The author of the FDM is free to define as many or as few forces and moments based on the level of fidelity they want to implement and based on the aerodynamic data that they have available to them for the aircraft type.

JSBSim provides a number of mathematical functions for use in calculating a force or moment. A lookup table element is also provided.

```xml
<function name="aero/coefficient/CLalpha">
    <description>Lift_due_to_alpha</description>
    <product>
        <property>aero/qbar-psf</property>
        <property>metrics/Sw-sqft</property>
        <table>
            <independentVar>aero/alpha-rad</independentVar>
            <tableData>
               -0.20 -0.68
                0.00  0.20
                0.23  1.20
                0.46  0.20
            </tableData>
        </table>
    </product>
</function>
```

JSBSim provides a number of pre-calculated properties, e.g. `aero/qbar-psf` is the dynamic pressure $\frac{1}{2} \rho V^2$ calculated based on the current air density of the aircraft within the atmosphere model and the aircraft’s true airspeed.

During each time step JSBSim evaluates each function defining a force for each axis and sums all the forces in order to calculate the net force per axis. 

The Moment Reference Center (MRC), named as `AERORP`, needs to be defined within the `metrics` element.

```xml
<metrics>
    <location name="AERORP" unit="IN">
        <x> 625 </x>
        <y> 0 </y>
        <z> 24 </z>
    </location>
</metrics>
```

The moments and forces can also reference properties that define control positions, e.g. `fcs/elevator-pos-rad` as shown below. The example below also shows how Mach effects may be modelled, in this case to change $C_{m_{\delta_e}}$ based on Mach.

```xml
<function name="aero/coefficient/Cmde">
    <description>Pitch_moment_due_to_elevator</description>
    <product>
        <property>aero/qbar-psf</property>
        <property>metrics/Sw-sqft</property>
        <property>metrics/cbarw-ft</property>
        <property>fcs/elevator-pos-rad</property>
        <table>
            <independentVar>velocities/mach</independentVar>
            <tableData>
                0.0 -1.20
                2.0 -0.30
            </tableData>
        </table>
    </product>
</function>
```

All the moment definitions are evaluated and summed for each axis. JSBSim then calculates an additional moment based on the current forces and the moment arm between the current cg and the MRC.

### Propulsion

JSBSim includes engine models covering piston, turbine, turboprop, rocket and electric engines. Configuration parameters are defined to specify the performance of specific engines.

A `propulsion` element is defined which specifies an engine file for the specific engine, it’s physical location and orientation on the aircraft.

```xml
<propulsion>
    <engine file="CFM56">
        <feed>0</feed>
        <feed>2</feed>
        <thruster file="direct">
            <location unit="IN">
                <x> 540 </x>
                <y> -193 </y>
                <z> -40 </z>
            </location>
            <orient unit="DEG">
                <roll> 0 </roll>
                <pitch> 0 </pitch>
                <yaw> 0 </yaw>
            </orient>
        </thruster>
    </engine>
</propulsion>
```

Below is an example of a specific turbine engine type.

```xml
<turbine_engine name="CFM56">
  <milthrust> 20000.0 </milthrust>
  <bypassratio>     5.9 </bypassratio>
  <tsfc>            0.657 </tsfc>
  <bleed>           0.04 </bleed>
  <idlen1>         30.0 </idlen1>
  <idlen2>         60.0 </idlen2>
  <maxn1>         100.0 </maxn1>
  <maxn2>         100.0 </maxn2>
  <augmented>         0 </augmented>
  <injected>          0 </injected>

  <function name="IdleThrust">
    <table>
      <independentVar lookup="row">velocities/mach</independentVar>
      <independentVar lookup="column">atmosphere/density-altitude</independentVar>
      <tableData>
                 -10000  0       10000   20000   30000   40000   50000   60000
            0.0  0.0420  0.0436  0.0528  0.0694  0.0899  0.1183  0.1467  0.0
            0.2  0.0500  0.0501  0.0335  0.0544  0.0797  0.1049  0.1342  0.0
            0.4  0.0040  0.0047  0.0020  0.0272  0.0595  0.0891  0.1203  0.0
            0.6  0.0     0.0     0.0     0.0     0.0276  0.0718  0.1073  0.0
            0.8  0.0     0.0     0.0     0.0     0.0174  0.0468  0.0900  0.0
            1.0  0.0     0.0     0.0     0.0     0.0     0.0422  0.0700  0.0
      </tableData>
    </table>
  </function>

  <function name="MilThrust">
    <table>
      <independentVar lookup="row">velocities/mach</independentVar>
      <independentVar lookup="column">atmosphere/density-altitude</independentVar>
      <tableData>
                  -10000  0       10000   20000   30000   40000   50000   60000
            0.0   1.2600  1.0000  0.7400  0.5340  0.3720  0.2410  0.1490  0.0
            0.2   1.1710  0.9340  0.6970  0.5060  0.3550  0.2310  0.1430  0.0
            0.4   1.1500  0.9210  0.6920  0.5060  0.3570  0.2330  0.1450  0.0
            0.6   1.1810  0.9510  0.7210  0.5320  0.3780  0.2480  0.1540  0.0
            0.8   1.2580  1.0200  0.7820  0.5820  0.4170  0.2750  0.1700  0.0
            1.0   1.3690  1.1200  0.8710  0.6510  0.4750  0.3150  0.1950  0.0
            1.2   0.0000  0.0000  0.0000  0.0000  0.0000  0.0000  0.0000  0.0
      </tableData>
    </table>
  </function>
</turbine_engine>
```

If JSBSim’s specific engine modelling doesn’t meet the FDM author’s requirements a propulsion force can be added as an `external_reaction` with the FDM author calcuating the magnitude of the force which JSBSim will then apply.

```xml
<external_reactions>
    <force name="pushback" frame="BODY">
        <location unit="IN">
            <x> -980.19 </x>
            <y> 0.00 </y>
            <z> -65.00 </z>
        </location>
        <direction>
            <x> 1 </x>
            <y> 0 </y>
            <z> 0 </z>
        </direction>
    </force>
</external_reactions>
```

### FCS

The Flight Control System can be as simple as modelling a direct physical connection mapping the pilot’s control input in the range from `[-1, +1]` linearly to an angular position for the relevant control position. Or a complete Fly-By-Wire (FBW) FCS can be implemented.

The control position property is then used by functions in the aerodynamic section for calculating forces and moments.

```xml
<channel name="Pitch">

    <summer name="Pitch Trim Sum">
        <input>fcs/elevator-cmd-norm</input>
        <input>fcs/pitch-trim-cmd-norm</input>
        <clipto>
            <min>-1</min>
            <max> 1 </max>
        </clipto>
    </summer>

    <aerosurface_scale name="Elevator Control">
        <input>fcs/pitch-trim-sum</input>
        <range>
            <min> -0.3 </min>
            <max> 0.3 </max>
        </range>
        <output>fcs/elevator-pos-rad</output>
    </aerosurface_scale>
```

A `pid` element is provided for use in defining an FCS that makes use of feedback control.

```xml
<! --
    - Calculate the difference between actual roll-rate and
    - commanded roll-rate.
-->

<summer name="fcs/roll-trim-error">
    <input> fcs/aileron-cmd-norm </input>
    <input> -fcs/roll-rate-norm </input>
</summer>

<pid name="fcs/roll-rate-pid">
    <trigger>fcs/aileron-pid-trigger</trigger>
    <input>fcs/roll-trim-error</input>
    <kp> 3.00000 </kp>
    <ki> 0.00050 </ki>
    <kd> -0.00125 </kd>
</pid>
```

XXXXXXXXXXX

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

[An example of script file would be appropriate]

JSBSim has been in development since 1996; in 2018, its codebase was moved to GitHub under the organization JSBSim-Team.

# Use Cases and Research Applications

JSBSim is used across a broad range of aerospace applications, including flight control development, UAV research, aircraft design studies, and simulation-based testing. It's use in academic and industry research has resulted in over 1000 citations as per Google Scholar, and it has been integrated into several popular flight simulators and research platforms.

Examples of use cases include:
  
- Modeling flight dynamics within a full-featured flight simulator, such as FlightGear, MIXR (formerly known as OpenEaagles), the Outerra world simulator, or Epic Games’ Unreal Engine 5. 

- Reinforcement learning research, where JSBSim is used as the environment in which an agent learns to control an aircraft. One example being it's use in the [DARPA Virtual Air Combat Competition](https://www.darpa.mil/news/2019/virtual-air-combat-competition).

- SITL (Software In The Loop) Drone autopilot testing:  [ArduPilot](https://ardupilot.org/dev/docs/sitl-with-jsbsim.html), [PX4 Autopilot](https://docs.px4.io/main/en/sim_jsbsim/), [Paparazzi](https://wiki.paparazziuav.org/wiki/Simulation)

- [additional examples here, look at some of the Google Scholar citations for different/interesting use cases]

See also the selection of use cases reported by the authors in 2009 [@progress-on-and-usage-of-the-open-source-flight-dynamics-2009].

JSBSim has been included in the [SPEC CPU](https://www.spec.org/) benchmark.

# Implementation and Engineering Practices

A key requirement of an FDM is accuracy, as would be expected. That is, the underlying math model of rigid body motion needs to be implemented properly. But how can one verify this? One way is through comparison with other similar flight simulation applications. To this end, the NASA Engineering Safety Center undertook an effort in 2015 to develop a set of check cases that could serve as a basis for comparing time-history data across simulations. JSBSim was included in this effort as the only non-NASA simulation [@open-aerospace-jsbsim-nasa-test-cases,open-aerospace-jsbsim-nasa-test-cases-case-01]. 

[description of JSBSim development and version management, etc.]

# Acknowledgements

JSBSim is currently being maintained and developed by Bertrand Coconnier, Sean McLeod, and Agostino De Marco, along with contributions from the broader community. Initial architecture and development was done by Jon Berndt, with major contributions from Tony Peden, David Megginson, and David Culp. Initial integration into the FlightGear open source flight simulator was assisted by Curt Olson. 

# References

 - CAVEAT: this section should be left blank. It will be populated by the automatic production workflow of the JOSS paper, based on the citations found in the running text.

Jon Berndt. 
["JSBSim: An Open Source Flight Dynamics Model in C++,"](https://arc.aiaa.org/doi/10.2514/6.2004-4923) AIAA 2004-4923. AIAA Modeling and Simulation Technologies Conference and Exhibit. August 2004

Jon Berndt and Agostino De Marco. ["Progress On and Usage of the Open Source Flight Dynamics Model Software Library, JSBSim,"](https://arc.aiaa.org/doi/10.2514/6.2009-5699) AIAA 2009-5699. AIAA Modeling and Simulation Technologies Conference. August 2009

Daniel G. Murri, E. Bruce Jackson, Robert O. Shelton. ["Check-Cases for Verification of 6-Degree-of-Freedom Flight Vehicle Simulations,"](https://ntrs.nasa.gov/api/citations/20150001263/downloads/20150001263.pdf) NASA/TM-2015-218675/Volume I
