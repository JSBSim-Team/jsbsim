# JSBSim interface with MATLAB

JSBSim provides an S-Function to interface your flight dynamics model with MATLAB and Simulink.

Below are the instructions to build the S-Function from JSBSim Windows installer.

## Build instructions for MacOS

### Download and build JSBSim from source

Download the JSBSim source from [JSBSim release section](https://github.com/JSBSim-Team/jsbsim/releases/tag/v1.2.3).

Alternatively, you can clone the source using git:

```bash
> git clone https://github.com/JSBSim-Team/jsbsim/tree/v1.2.3
```

Build the source following the instructions in the [JSBSim Manual](https://jsbsim-team.github.io/jsbsim-reference-manual/mypages/quickstart-building-the-program/).
Since we are using MacOS, we must build with CMake.

### Preparation for MATLAB

In order to work with the JSBSim MATLAB S-Function block, we must move some
files into the `build/` directory. From the command line, make sure your
current directory is the root of the `jsbsim/` source code.

```bash
> pwd
/path/to/jsbsim
```

Then, we must copy some files into the build. These are necessary for the
S-Function to operate.

```bash
> cp -r aircraft build/src/aircraft
> cp -r engine build/src/engine
> cp -r scripts build/src/scripts
```

### Compile the S-Function and Move Compiled Files

In `jsbsim/matlab` open `JSBSimSimulinkCompiler.m` and entire that the line
below `For MacOS:` is the only one that is not commented out. In the MATLAB
command line, run the script. Then, from the root of the source code,
copy the following files into the build source:

```bash
> cp matlab/JSBSim_SFunction.mexmaci64 build/src/JSBSim_SFunction.mexmaci64
> cp matlab/ex737cruise.slx build/src/ex737cruise.slx
> cp matlab/TestJSBSim.m build/src/TestJSBSim.m
> cp matlab/clearSF.m build/src/clearSF.m
```

You can now follow the test instructions below in order to ensure it is in
working condition.

Once you have the S-Function compiled, you just need `aircraft/` and `engine/`
alongside the compiled `JSBSim_SFunction.mexmaci64` in order for it to work.
You also nede to be sure that the script in the S-Function parameter is in
the correct location.

## Build instructions for the Windows platform

### Download and install JSBSim

A Windows installer `JSBSim-1.2.3-setup.exe` is available from [JSBSim release section](https://github.com/JSBSim-Team/jsbsim/releases/tag/v1.2.3). Download the installer and set up JSBSim in the following local directory tree

```bash
<JSBSim root>/
   aeromatic++/
   aircraft/
   data_output/
   engine/
   include/
   lib/
   matlab/
   scripts/
   systems/
   COPYING
   JSBSim.exe
   msvcp140.dll
```

The exact location `<JSBSim root>/` on your system depends on where you choose to install JSBSim.

> **NOTE:** The JSBSim library file located at `lib\JSBSim.lib` has been compiled with Microsoft Visual Studio Enterprise 2022 (build 17.13.35825.156)

### Prepare MATLAB

The following steps are tested for *Matlab v2020b*.

To compile the file `<JSBSim root>/matlab/JSBSim_SFunction.cpp` and get the MEX-File `JSBSim_SFunction.mexw64` on Windows, you need a working C++ compiler. Among the available options, we suggest using [Visual Studio Community Edition](https://visualstudio.microsoft.com/).

Once you have a working C++ development environment, you need to tell the MEX utility what is the default C++ compiler. This can be achieved by executing the following command from the Matlab command line:

`[Matlab prompt]>> mex -setup CPP`

MEX will detect your C++ compiler and will be configured to build your `*.mexw64` files.

### Run the JSBSim script for MATLAB

The MATLAB script `<JSBSim root>/matlab/JSBSimSimulinkCompile.m` default setting is to compile the S-Function from the folder `<JSBSim root>/matlab`. If needed, you can customize the script to your own needs.

Once done, change the current *Matlab Working Directory* to `<JSBSim root>/matlab` and run `JSBSimSimulinkCompile.m` either from the Matlab editor or from the command line:

`[Matlab prompt]>> JSBSimSimulinkCompile`

After the completion of the compilation you should have the following MEX-file `<JSBSim root>/matlab/JSBSim_SFunction.mexw64`.

### Test the S-Function

To verify that the MEX file has been built successfully, we will now run the example script provided with JSBSim.

First of all, copy the script `<JSBSim root>/matlab/TestJSBSim.m` to the root directory `<JSBSim root>`. This makes sense, because we want a Matlab EXecutable (MEX) that lives besides `<JSBSim root>/JSBSim.exe`, and that must be able to reach all input files in the directories `aircraft`, `engine`, `scripts`, and `systems`.

Proceed with the Simulink example file `ex737cruise.slx` and the MEX file `JSBSim_SFunction.mexw64` and copy them both from the folder `<JSBSim root>/matlab/` to the root directory `<JSBSim root>/`.

You should now have a script `<JSBSim root>/TestJSBSim.m` that contains the following instructions:

```matlab
disp('Run 737 example');
fprintf('Current directory: %s', pwd)
sim('ex737cruise');
clear functions;
clear all;
disp('JSBSim S-Function Reset');
```

and this is a Simulink screenshot once the file `<JSBSim root>/ex737cruise.slx` is open:

![image](https://user-images.githubusercontent.com/819499/128168297-7fdbd0c4-e7c7-40ee-bb53-156221989c8f.png)

In particular, this is how the Simulink block named `JSBSim_SFunction` appears when you double-click on it:

![image](https://user-images.githubusercontent.com/819499/128169185-b334bbe0-27f9-4426-91f9-3e4393da6e07.png)

When you run the test script you'll get the foloowing output in the Matlab command window:

```matlab
>> TestJSBSim
Run 737 example
Current directory: C:\Users\agodemar\JSBSim
Warning: 'Output Port 3' of 'ex737cruise/S-Function' is not connected.
> In TestJSBSim (line 3)


     JSBSim Flight Dynamics Model v1.1.8 [GitHub build 588/commit c943f83deeb3e14bed7939ac65dfac789a7a0181] Jul 24 2021 16:18:42
            [JSBSim-ML v2.0]

JSBSim startup beginning ...

Simulation dt set to 0.008333
Script input: scripts/737_cruise
Reset file: 'cruise_init' .

JSBSim S-Function is initializing...

Note: For Aircraft with integrators in the FCS, please type 'clearSF' to completely reset S-Function.

	Setting up JSBSim with standard 'aircraft', 'engine', and 'system' paths.
	Loading aircraft '737' ...
Reading Aircraft Configuration File: 737
                            Version: 2.0


This aircraft model is a BETA release!!!

This aircraft model probably will not fly as expected.

Use this model for development purposes ONLY!!!

  Description:   Models a Boeing 737.
  Model Author:  Dave Culp
  Creation Date: 2006-01-04
  Version:       $Revision: 1.43 $

  Aircraft Metrics:
    WingArea: 1171
    WingSpan: 94.7
    Incidence: 0
    Chord: 12.31
    H. Tail Area: 348
    H. Tail Arm: 48.04
    V. Tail Area: 297
    V. Tail Arm: 44.5
    Eyepoint (x, y, z): 80 , -30 , 70
    Ref Pt (x, y, z): 625 , 0 , 24
    Visual Ref Pt (x, y, z): 0 , 0 , 0

  Mass and Balance:
    baseIxx: 562000 slug-ft2
    baseIyy: 1.473e+06 slug-ft2
    baseIzz: 1.894e+06 slug-ft2
    baseIxy: -0 slug-ft2
    baseIxz: 8000 slug-ft2
    baseIyz: -0 slug-ft2
    Empty Weight: 83000 lbm
    CG (x, y, z): 639 , 0 , -40

  Ground Reactions:
    BOGEY Nose Gear
      Location: 158 , 0 , -84
      Spring Constant:  90000
      Damping Constant: 4000 (linear)
      Rebound Damping Constant: 8000 (linear)
      Dynamic Friction: 0.5
      Static Friction:  0.8
      Rolling Friction: 0.02
      Steering Type:    STEERABLE
      Grouping:         NONE
      Max Steer Angle:  35
      Retractable:      1
    BOGEY Left Main Gear
      Location: 648 , -100 , -84
      Spring Constant:  120000
      Damping Constant: 10000 (linear)
      Rebound Damping Constant: 20000 (linear)
      Dynamic Friction: 0.5
      Static Friction:  0.8
      Rolling Friction: 0.02
      Steering Type:    FIXED
      Grouping:         LEFT
      Max Steer Angle:  0
      Retractable:      1
    BOGEY Right Main Gear
      Location: 648 , 100 , -84
      Spring Constant:  120000
      Damping Constant: 10000 (linear)
      Rebound Damping Constant: 20000 (linear)
      Dynamic Friction: 0.5
      Static Friction:  0.8
      Rolling Friction: 0.02
      Steering Type:    FIXED
      Grouping:         RIGHT
      Max Steer Angle:  0
      Retractable:      1

  Propulsion:
      FUEL tank holds 10200 lbs. FUEL
      currently at 98.0392% of maximum capacity
      Tank location (X, Y, Z): 520, -80, -18
      Effective radius: 0 inches
      Initial temperature: -9999 Fahrenheit
      Priority: 1
      FUEL tank holds 10200 lbs. FUEL
      currently at 98.0392% of maximum capacity
      Tank location (X, Y, Z): 520, 80, -18
      Effective radius: 0 inches
      Initial temperature: -9999 Fahrenheit
      Priority: 1
      FUEL tank holds 15000 lbs. FUEL
      currently at 26.6667% of maximum capacity
      Tank location (X, Y, Z): 480, 0, -18
      Effective radius: 0 inches
      Initial temperature: -9999 Fahrenheit
      Priority: 1
    2 dimensional table with 6 rows, 8 columns.
		-10000.0000	0.0000	10000.0000	20000.0000	30000.0000	40000.0000	50000.0000	60000.0000
	0.0000	0.0420	0.0436	0.0528	0.0694	0.0899	0.1183	0.1467	0.0000
	0.2000	0.0500	0.0501	0.0335	0.0544	0.0797	0.1049	0.1342	0.0000
	0.4000	0.0040	0.0047	0.0020	0.0272	0.0595	0.0891	0.1203	0.0000
	0.6000	0.0000	0.0000	0.0000	0.0000	0.0276	0.0718	0.1073	0.0000
	0.8000	0.0000	0.0000	0.0000	0.0000	0.0174	0.0468	0.0900	0.0000
	1.0000	0.0000	0.0000	0.0000	0.0000	0.0000	0.0422	0.0700	0.0000
    Function: propulsion/engine[0]/IdleThrust
    2 dimensional table with 7 rows, 8 columns.
		-10000.0000	0.0000	10000.0000	20000.0000	30000.0000	40000.0000	50000.0000	60000.0000
	0.0000	1.2600	1.0000	0.7400	0.5340	0.3720	0.2410	0.1490	0.0000
	0.2000	1.1710	0.9340	0.6970	0.5060	0.3550	0.2310	0.1430	0.0000
	0.4000	1.1500	0.9210	0.6920	0.5060	0.3570	0.2330	0.1450	0.0000
	0.6000	1.1810	0.9510	0.7210	0.5320	0.3780	0.2480	0.1540	0.0000
	0.8000	1.2580	1.0200	0.7820	0.5820	0.4170	0.2750	0.1700	0.0000
	1.0000	1.3690	1.1200	0.8710	0.6510	0.4750	0.3150	0.1950	0.0000
	1.2000	0.0000	0.0000	0.0000	0.0000	0.0000	0.0000	0.0000	0.0000
    Function: propulsion/engine[0]/MilThrust
      X = 540.0000
      Y = -193.0000
      Z = -40.0000
      Pitch = 0.0000 degrees
      Yaw = 0.0000 degrees
    2 dimensional table with 6 rows, 8 columns.
		-10000.0000	0.0000	10000.0000	20000.0000	30000.0000	40000.0000	50000.0000	60000.0000
	0.0000	0.0420	0.0436	0.0528	0.0694	0.0899	0.1183	0.1467	0.0000
	0.2000	0.0500	0.0501	0.0335	0.0544	0.0797	0.1049	0.1342	0.0000
	0.4000	0.0040	0.0047	0.0020	0.0272	0.0595	0.0891	0.1203	0.0000
	0.6000	0.0000	0.0000	0.0000	0.0000	0.0276	0.0718	0.1073	0.0000
	0.8000	0.0000	0.0000	0.0000	0.0000	0.0174	0.0468	0.0900	0.0000
	1.0000	0.0000	0.0000	0.0000	0.0000	0.0000	0.0422	0.0700	0.0000
    Function: propulsion/engine[1]/IdleThrust
    2 dimensional table with 7 rows, 8 columns.
		-10000.0000	0.0000	10000.0000	20000.0000	30000.0000	40000.0000	50000.0000	60000.0000
	0.0000	1.2600	1.0000	0.7400	0.5340	0.3720	0.2410	0.1490	0.0000
	0.2000	1.1710	0.9340	0.6970	0.5060	0.3550	0.2310	0.1430	0.0000
	0.4000	1.1500	0.9210	0.6920	0.5060	0.3570	0.2330	0.1450	0.0000
	0.6000	1.1810	0.9510	0.7210	0.5320	0.3780	0.2480	0.1540	0.0000
	0.8000	1.2580	1.0200	0.7820	0.5820	0.4170	0.2750	0.1700	0.0000
	1.0000	1.3690	1.1200	0.8710	0.6510	0.4750	0.3150	0.1950	0.0000
	1.2000	0.0000	0.0000	0.0000	0.0000	0.0000	0.0000	0.0000	0.0000
    Function: propulsion/engine[1]/MilThrust
      X = 540.0000
      Y = 193.0000
      Z = -40.0000
      Pitch = 0.0000 degrees
      Yaw = 0.0000 degrees

  FCS: FCS: 737

    Channel Pitch

    Loading Component "Pitch Trim Sum" of type: SUMMER
      Minimum limit: -1.000000
      Maximum limit: 1.000000
      INPUTS:
       elevator-cmd-norm
       pitch-trim-cmd-norm
      OUTPUT: pitch-trim-sum

    Loading Component "Elevator Control" of type: AEROSURFACE_SCALE
      INPUT: pitch-trim-sum
      GAIN: constant value 1.000000
      OUTPUT: elevator-pos-rad
      OUTPUT: elevator-control
      In/Out Mapping:
        Input MIN: -1.0000
        Input MAX: 1.0000
        Output MIN: -0.3000
        Output MAX: 0.3000

    Loading Component "Elevator Normalized" of type: AEROSURFACE_SCALE
      INPUT: elevator-pos-rad
      GAIN: constant value 1.000000
      OUTPUT: elevator-pos-norm
      OUTPUT: elevator-normalized
      In/Out Mapping:
        Input MIN: -0.3000
        Input MAX: 0.3000
        Output MIN: -1.0000
        Output MAX: 1.0000

    Channel Roll

    Loading Component "Roll Trim Sum" of type: SUMMER
      Minimum limit: -1.000000
      Maximum limit: 1.000000
      INPUTS:
       aileron-cmd-norm
       roll-trim-cmd-norm
      OUTPUT: roll-trim-sum

    Loading Component "Left Aileron Control" of type: AEROSURFACE_SCALE
      INPUT: roll-trim-sum
      GAIN: constant value 1.000000
      OUTPUT: left-aileron-pos-rad
      OUTPUT: left-aileron-control
      In/Out Mapping:
        Input MIN: -1.0000
        Input MAX: 1.0000
        Output MIN: -0.3500
        Output MAX: 0.3500

    Loading Component "Right Aileron Control" of type: AEROSURFACE_SCALE
      INPUT: -roll-trim-sum
      GAIN: constant value 1.000000
      OUTPUT: right-aileron-pos-rad
      OUTPUT: right-aileron-control
      In/Out Mapping:
        Input MIN: -1.0000
        Input MAX: 1.0000
        Output MIN: -0.3500
        Output MAX: 0.3500

    Loading Component "Left aileron Normalized" of type: AEROSURFACE_SCALE
      INPUT: left-aileron-pos-rad
      GAIN: constant value 1.000000
      OUTPUT: left-aileron-pos-norm
      OUTPUT: left-aileron-normalized
      In/Out Mapping:
        Input MIN: -0.3500
        Input MAX: 0.3500
        Output MIN: -1.0000
        Output MAX: 1.0000

    Loading Component "Right aileron Normalized" of type: AEROSURFACE_SCALE
      INPUT: right-aileron-pos-rad
      GAIN: constant value 1.000000
      OUTPUT: right-aileron-pos-norm
      OUTPUT: right-aileron-normalized
      In/Out Mapping:
        Input MIN: -0.3500
        Input MAX: 0.3500
        Output MIN: -1.0000
        Output MAX: 1.0000

    Channel Yaw

    Loading Component "Rudder Command Sum" of type: SUMMER
      Minimum limit: -1.000000
      Maximum limit: 1.000000
      INPUTS:
       rudder-cmd-norm
       yaw-trim-cmd-norm
      OUTPUT: rudder-command-sum

    Loading Component "Yaw Damper" of type: SCHEDULED_GAIN
    1 dimensional table with 3 rows.
	0.0000	0.0000
	0.1000	0.0000
	0.1100	1.0000
      INPUT: r-aero-rad_sec
      GAIN: constant value 1.000000
      OUTPUT: yaw-damper
      Scheduled by table:
    1 dimensional table with 3 rows.
	0.0000	0.0000
	0.1000	0.0000
	0.1100	1.0000

    Loading Component "Yaw Damper Final" of type: SCHEDULED_GAIN
    1 dimensional table with 3 rows.
	0.0000	0.0000
	0.1000	0.0000
	0.1100	1.0000
      INPUT: yaw-damper
      GAIN: constant value 1.000000
      OUTPUT: yaw-damper-final
      Scheduled by table:
    1 dimensional table with 3 rows.
	0.0000	0.0000
	0.1000	0.0000
	0.1100	1.0000

    Loading Component "Rudder Sum" of type: SUMMER
      Minimum limit: -1.000000
      Maximum limit: 1.000000
      INPUTS:
       rudder-command-sum
       yaw-damper-final
      OUTPUT: rudder-sum

    Loading Component "Rudder Control" of type: AEROSURFACE_SCALE
      INPUT: rudder-sum
      GAIN: constant value 1.000000
      OUTPUT: rudder-pos-rad
      OUTPUT: rudder-control
      In/Out Mapping:
        Input MIN: -1.0000
        Input MAX: 1.0000
        Output MIN: -0.3500
        Output MAX: 0.3500

    Loading Component "Rudder Normalized" of type: AEROSURFACE_SCALE
      INPUT: rudder-pos-rad
      GAIN: constant value 1.000000
      OUTPUT: rudder-pos-norm
      OUTPUT: rudder-normalized
      In/Out Mapping:
        Input MIN: -0.3500
        Input MAX: 0.3500
        Output MIN: -1.0000
        Output MAX: 1.0000

    Channel Flaps

    Loading Component "Flaps Control" of type: KINEMATIC
      INPUT: flap-cmd-norm
      DETENTS: 9
        0.0000 0.0000
        0.1250 5.0000
        0.2500 4.0000
        0.3750 3.0000
        0.5000 2.0000
        0.6250 2.0000
        0.7500 2.0000
        0.8750 2.0000
        1.0000 2.0000
      OUTPUT: flap-pos-norm
      OUTPUT: flaps-control

    Channel Landing Gear

    Loading Component "Gear Control" of type: KINEMATIC
      INPUT: gear-cmd-norm
      DETENTS: 2
        0.0000 0.0000
        1.0000 5.0000
      OUTPUT: gear-pos-norm
      OUTPUT: gear-control

    Channel Flight Spoilers

    Loading Component "Flight Spoilers Control" of type: KINEMATIC
      INPUT: speedbrake-cmd-norm
      DETENTS: 2
        0.0000 0.0000
        1.0000 0.6000
      OUTPUT: speedbrake-pos-norm
      OUTPUT: flight-spoilers-control

    Channel Ground Spoilers

    Loading Component "Ground Spoilers Control" of type: KINEMATIC
      INPUT: spoiler-cmd-norm
      DETENTS: 2
        0.0000 0.0000
        1.0000 0.6000
      OUTPUT: spoiler-pos-norm
      OUTPUT: ground-spoilers-control
    1 dimensional table with 10 rows.
	0.0000	0.0480
	0.1000	0.5150
	0.1500	0.6290
	0.2000	0.7090
	0.3000	0.8150
	0.4000	0.8820
	0.5000	0.9280
	0.6000	0.9620
	0.7000	0.9880
	0.8000	1.0000
    Function: aero/function/kCDge
    1 dimensional table with 13 rows.
	0.0000	1.2030
	0.1000	1.1270
	0.1500	1.0900
	0.2000	1.0730
	0.3000	1.0460
	0.4000	1.0280
	0.5000	1.0190
	0.6000	1.0130
	0.7000	1.0080
	0.8000	1.0060
	0.9000	1.0030
	1.0000	1.0020
	1.1000	1.0000
    Function: aero/function/kCLge
    1 dimensional table with 2 rows.
	0.0000	1.0000
	0.1000	0.8500
    Function: aero/function/kCLsb
    1 dimensional table with 2 rows.
	0.0000	1.0000
	0.1000	0.6000
    Function: aero/function/kCLsp

  Aerodynamics (Lift|Side|Drag axes):

    1 dimensional table with 5 rows.
	-1.5700	1.5000
	-0.2600	0.0420
	0.0000	0.0210
	0.2600	0.0420
	1.5700	1.5000
    Function: aero/coefficient/CD0
    Function: aero/coefficient/CDi
    1 dimensional table with 4 rows.
	0.0000	0.0000
	0.7900	0.0000
	1.1000	0.0230
	1.8000	0.0150
    Function: aero/coefficient/CDmach
    Function: aero/coefficient/CDflap
    Function: aero/coefficient/CDgear
    Function: aero/coefficient/CDsb
    Function: aero/coefficient/CDsp
    1 dimensional table with 5 rows.
	-1.5700	1.2300
	-0.2600	0.0500
	0.0000	0.0000
	0.2600	0.0500
	1.5700	1.2300
    Function: aero/coefficient/CDbeta
    Function: aero/coefficient/CDde
    Function: aero/coefficient/CYb
    1 dimensional table with 4 rows.
	-0.2000	-0.6800
	0.0000	0.2000
	0.2300	1.2000
	0.4600	0.2000
    Function: aero/coefficient/CLalpha
    Function: aero/coefficient/dCLflap
    Function: aero/coefficient/CLde
    Function: aero/coefficient/Clb
    Function: aero/coefficient/Clp
    Function: aero/coefficient/Clr
    1 dimensional table with 2 rows.
	0.0000	0.1000
	2.0000	0.0330
    Function: aero/coefficient/Clda
    Function: aero/coefficient/Cldr
    Function: aero/coefficient/Cmalpha
    1 dimensional table with 2 rows.
	0.0000	-1.2000
	2.0000	-0.3000
    Function: aero/coefficient/Cmde
    Function: aero/coefficient/Cmq
    Function: aero/coefficient/Cmadot
    Function: aero/coefficient/Cnb
    Function: aero/coefficient/Cnr
    Function: aero/coefficient/Cndr

  Input data set: 0

  Input data set: 1

    Declared properties


In file aircraft/737/737.xml: line 911
      Property fcs/aileron-cmd-norm is already defined.

In file aircraft/737/737.xml: line 912
      Property fcs/elevator-cmd-norm is already defined.

In file aircraft/737/737.xml: line 913
      Property fcs/rudder-cmd-norm is already defined.

In file aircraft/737/737.xml: line 914
      Property fcs/throttle-cmd-norm[0] is already defined.

In file aircraft/737/737.xml: line 915
      Property fcs/throttle-cmd-norm[1] is already defined.

In file aircraft/737/737.xml: line 916
      Property simulation/terminate is already defined.
	Model 737 loaded.
'737' Aircraft File has been successfully loaded!
Winsock DLL loaded ...
Creating input TCP socket on port 5137
Successfully bound to TCP input socket on port 5137

Winsock DLL loaded ...
Creating input UDP socket on port 5139
Successfully bound to UDP input socket on port 5139


  Mass Properties Report (English units: lbf, in, slug-ft^2)
                                      Weight    CG-X    CG-Y    CG-Z         Ixx         Iyy         Izz         Ixy         Ixz         Iyz
    Base Vehicle                     83000.0   639.0     0.0   -40.0    562000.0   1473000.0   1894000.0        -0.0      8000.0        -0.0
0   Fuel                               10000     520     -80     -18           0           0           0
1   Fuel                               10000     520      80     -18           0           0           0
2   Fuel                                4000     480       0     -18           0           0           0

    Total:                          107000.0   610.8     0.0   -35.1    591572.3   1539552.7   1986235.4         0.0     19109.1         0.0

End of vehicle configuration loading.
-------------------------------------------------------------------------------

Simulation completed.
Remember to reset the program by typing clearSF in the matlab command window!
JSBSim S-Function Reset
```

and in Simulink you'll have the following outputs:

![image](https://user-images.githubusercontent.com/819499/128168677-d753fc97-710c-4b35-b962-b4643baaade1.png)

So you just have to try it out and adapt this workflow to your needs.

## License

The JSBSim interface with MATLAB including the S-Function is open source and is licensed under the [BSD license](https://opensource.org/licenses/bsd-license.php). The license is included in the source code file [LICENSE.txt](https://github.com/JSBSim-Team/jsbsim/blob/master/matlab/LICENSE.txt).
