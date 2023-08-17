# JSBSim for Unreal - Unix build instructions

This document describes how to build the JSBSim library as well as the Unreal Engine plugin for supported Unix operating systems.

## Support
| Operating System | Status |
|------------------|--------|
| Linux            | Supported |
| Macos            | Supported |
| Android          | Work in progress |

## Building (Easy way)
Run `JSBSimForUnrealMac.sh` on Macos or `JSBSimForUnrealLinux.sh` on Linux

## Building (Hard way)
The first step is to build JSBSim on your target platform using cmake. Open an terminal and type the following commands

### For Macos

```bash
mkdir build
cd build
cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++" ..
make -j4
```

### For Linux
Make sure to install the following package `clang libc++-dev libc++1-x libc++abi1-x libc++abi-x-dev`. Where (x) is the version number of your distribution.
On Ubuntu 23.04 it was `libc++ 1.15, libc++abi1-15`. 

```bash
mkdir build
cd build
cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++" ..
make -j4
```

### Copy headers and the libJSBSim library
```bash
# From the build folder
cd ..

# Copy headers
rsync -avm --include='*.h' --include='*.hpp' --include='*.hxx' -f 'hide,! */' src/ UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Source/ThirdParty/JSBSim/Include/

# Copy the JSBSim library (Macos)
mkdir -p UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Source/ThirdParty/JSBSim/Lib/Mac/
cp -Rf build/src/libJSBSim.1.2.0.dev1.dylib UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Source/ThirdParty/JSBSim/Lib/Mac/libJSBSim.dylib

# Copy the JSBSim library (Linux)
mkdir -p UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Source/ThirdParty/JSBSim/Lib/Linux/
cp -Rf build/src/libJSBSim.so UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Source/ThirdParty/JSBSim/Lib/Linux/

# Copy the JSBSim library (Android)
mkdir -p UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Source/ThirdParty/JSBSim/Lib/Android/
cp -Rf build/src/libJSBSim.so UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Source/ThirdParty/JSBSim/Lib/Android/

# Copy the resource files
mkdir -p UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Resources/JSBSim
cp -Rf aircraft UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Resources/JSBSim
cp -Rf engine UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Resources/JSBSim
cp -Rf systems UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Resources/JSBSim
```

### Compile the Unreal Engine project

Run the following command to compile and run JSBSim for Unreal

You can generate a makefile or an Xcode workspace using the following command

```bash
$PATH_TO_UNREAL/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh $PATH_TO_JSBSIM/UnrealEngine/UEReferenceApp.uproject -game
```

```bash
$PATH_TO_UNREAL/Engine/Binaries/Linux/UnrealEditor $PATH_TO_JSBSIM/UnrealEngine/UEReferenceApp.uproject
```